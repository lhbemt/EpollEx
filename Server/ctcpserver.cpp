#include "ctcpserver.h"
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdarg.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include <sys/epoll.h>

CTCPServer::CTCPServer()
{
    for (int i = 0; i < EVENT_NUM; ++i)
        m_queueFree.push(i);
}

CTCPServer::~CTCPServer()
{

}

void CTCPServer::ErrorMsg(const char *args, ...)
{
    char szErrorMsg[1024] = { 0x00 };
    va_list arg;
    va_start(arg, args);
    vsprintf(szErrorMsg, args, arg);
    m_strErrorMsg = szErrorMsg;
}

bool CTCPServer::InitEpoll()
{
    m_epollfd = epoll_create(5);
    if (m_epollfd < 0)
    {
        ErrorMsg("epoll_create error, errorcode: %d, errormsg: %s", errno, strerror(errno));
        return false;
    }
    return true;
}

bool CTCPServer::AddfdToEpoll(int nfd, bool et) // use lt default
{
    epoll_event env;
    env.data.fd = nfd;
    env.events = EPOLLIN;
    if (et)
        env.events |= EPOLLET; // with et mode

    int nRet = epoll_ctl(m_epollfd, EPOLL_CTL_ADD, nfd, &env);
    if (nRet < 0)
    {
        ErrorMsg("epoll_ctl error, errorcode: %d, errormsg: %s", errno, strerror(errno));
        return false;
    }
    return true;
}

bool CTCPServer::Init(std::string& strIP, int nPort)
{
    bool bRet = InitEpoll();
    if (!bRet)
        return;

    m_listensockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_listensockfd < 0)
    {
        ErrorMsg("socket error, errorcode: %d, errormsg: %s", errno, strerror(errno));
        return false;
    }

    // set nonblock
    fcntl(m_listensockfd, F_SETFL, fcntl(m_listensockfd, F_GETFL, 0) | O_NONBLOCK);
    // set reuse
    int nUse = 1;
    nRet = setsockopt(m_listensockfd, SOL_SOCKET, SO_REUSEADDR, &nUse, sizeof(nUse));
    if (nRet < 0)
    {
        ErrorMsg("setsockopt error, errorcode: %d, errormsg: %s", errno, strerror(errno));
        close(m_listensockfd);
        return false;
    }

    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    int nRet = inet_pton(AF_INET, strIP.c_str(), &serverAddr.sin_addr);
    if (nRet != 1)
    {
        ErrorMsg("inet_pton error, errorcode: %d, errormsg: %s", errno, strerror(errno));
        close(m_listensockfd);
        return false;
    }
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(nPort);

    // bind
    nRet = bind(m_listensockfd, (sockaddr*)&serverAddr, sizeof(sockaddr));
    if (nRet < 0)
    {
        ErrorMsg("bind error, errorcode: %d, errormsg: %s", errno, strerror(errno));
        close(m_listensockfd);
        return false;
    }

    //listen
    nRet = listen(m_listensockfd, SOMAXCONN);
    if (nRet < 0)
    {
        ErrorMsg("listen error, errorcode: %d, errormsg: %s", errno, strerror(errno));
        close(m_listensockfd);
        return false;
    }

    // add to epoll
    bRet = AddfdToEpoll(m_listensockfd, true);
    if (!bRet)
    {
        close(m_listensockfd);
        return false;
    }

    return true;
}

void CTCPServer::DoAccept()
{
    while(1)
    {
        int nConnfd = accept(m_listensockfd, NULL, NULL);
        if (nConnfd == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return;
            else // accept error terminate thread
            {
                write(STDERR_FILENO, 'c', 1);
                return;
            }
        }
        else // find free client
        {
            m_mutexFree.lock();
            if (m_queueFree.empty()) // no free client
            {
                close(nConnfd);
                m_mutexFree.unlock();
            }
            else
            {
                int index = m_queueFree.front();
                m_clients[index] = nConnfd;
                AddfdToEpoll(nConnfd, true);
                m_queueFree.pop();
                m_mutexFree.unlock();
            }
        }
    }
}

void CTCPServer::DoSocket(epoll_event *env)
{
    if (!env)
        return;
    int nRet = 0;
    char szBuff[1024] = { 0x00 };
    if (env->events & EPOLLIN) // read
    {
        while(1) // read until no data et mode
        {
            memset(szBuff, 0, sizeof(szBuff));
            nRet = recv(env->data.fd, szBuff, sizeof(szBuff), 0);
            if (nRet == 0) // socket close
            {
                close(env->data);
                epoll_ctl(m_epollfd, EPOLL_CTL_DEL, env->data.fd, NULL); // remove
                m_mutexFree.lock();
                for (int i = 0; i < EVENT_NUM; ++i)
                {
                    if (m_clients[i] == env->data)
                    {
                        m_queueFree.push(i); //free
                        m_clients[i] = -1;
                        break;
                    }
                }
                m_mutexFree.unlock();
                return;
            }
            else if (nRet == -1)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    return; // read all
                else // recv error
                    return;
            }
            else // read success
                send(env->data.fd, szBuff, nRet, 0);
        }
    }
}

void CTCPServer::ClearAllClient()
{
    for (int i = 0; i < EVENT_NUM; ++i)
    {
        if (m_clients[i] != -1)
        {
            epoll_ctl(m_epollfd, EPOLL_CTL_DEL, m_clients[i], NULL);
            close(m_clients[i]);
        }
    }
}

bool CTCPServer::StartServer(std::string &strIP, int nPort)
{

    bool bRet = Init(strIP, nPort);
    if (!bRet)
        return false;
    fcntl(STDERR_FILENO, F_SETFL, fcntl(STDERR_FILENO, F_GETFL, 0) | O_NONBLOCK);
    bRet = AddfdToEpoll(STDERR_FILENO, true); // terminate thread
    if (!bRet)
        return false;
    // start thread

    epoll_event envs[EVENT_NUM];
    int nRet = 0;
    m_thEpoll = std::move(std::thread([=](){
        while(1)
        {
            nRet = epoll_wait(m_epollfd, envs, EVENT_NUM, -1);
            if (nRet < 0)
            {
                ErrorMsg("epoll_wait error, errorcode: %d, errormsg: %s", errno, strerror(errno));
                return;
            }
            if (nRet == 0)
                continue;
            for (int i = 0; i < nRet; ++i)
            {
                nRet = envs[i].data.fd;
                if (nRet == m_listensockfd)
                {
                    DoAccept();
                }
                else if (nRet == STDERR_FILENO) // ternimate thread
                    return;
                else
                    DoSocket(&envs[i]);
            }
        }
    }));

    m_thEpoll.join(); // wait terminate
    ClearAllClient(); // clear

    return true;
}

void CTCPServer::StopServer() // stop
{
    write(STDERR_FILENO, 'c', 1);
}
