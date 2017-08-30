#include "ctcpclient.h"
#include <stdarg.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

CTCPClient::CTCPClient()
{

}

CTCPClient::~CTCPClient()
{}

void CTCPClient::ErrorMsg(const char *args, ...)
{
    char szErrorMsg[1024] = { 0x00 };
    va_list arg;
    va_start(arg, args);
    vsprintf(szErrorMsg, args, arg);
    m_strErrorMsg = szErrorMsg;
}

bool CTCPClient::Init()
{
    m_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_sockfd < 0)
    {
        ErrorMsg("socket error: errorcode: %d, errormsg: %s", errno, strerror(errno));
        return false;
    }

    // set nonblocked
    int nRet = fcntl(m_sockfd, F_GETFL, 0);
    nRet |= O_NONBLOCK;
    fcntl(m_sockfd, F_SETFL, nRet);

    return true;
}

bool CTCPClient::Connect(std::string strIP, int nPort)
{
    bool bRet = Init();
    if (!bRet)
        return bRet;
    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    int nRet = inet_pton(AF_INET, strIP.c_str(), &serverAddr.sin_addr);
    if (nRet != 1)
    {
        ErrorMsg("inet_pton error, errorcode: %d, errorMsg: %s", errno, strerror(errno));
        return false;
    }
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(nPort);

    nRet = connect(m_sockfd, (sockaddr*)&serverAddr, sizeof(serverAddr));
    if (nRet != EINPROGRESS) // not einprrogress is error
    {
        ErrorMsg("connect error, errorcode: %d, errormsg: %s", errno, strerror(errno));
        return false;
    }
    return true;
}

void CTCPClient::DisConnect()
{
    close(m_sockfd);
}
