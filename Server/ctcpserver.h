#ifndef CTCPSERVER_H
#define CTCPSERVER_H
#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <sys/epoll.h>

const int EVENT_NUM = 1024;

class CTCPServer
{
public:
    CTCPServer();
    ~CTCPServer();

public:
    bool StartServer(std::string& strIP, int nPort);
    void StopServer();
    const char* GetErrorMsg()
    {
        return m_strErrorMsg.c_str();
    }

private:
    bool Init(std::string& ip, int nPort);
    void ErrorMsg(const char* arg, ...);
    bool InitEpoll();
    bool AddfdToEpoll(int nfd, bool et = false);
    void DoAccept();
    void DoSocket(epoll_event* env); // socket read or write
    void ClearAllClient();
private:
    std::string m_strErrorMsg;
    int         m_listensockfd;
    int         m_epollfd;
    std::thread m_thEpoll;
    int         m_clients[EVENT_NUM];

    std::mutex      m_mutexFree;
    std::queue<int> m_queueFree; // free client
};

#endif // CTCPSERVER_H
