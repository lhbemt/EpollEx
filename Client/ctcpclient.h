#ifndef CTCPCLIENT_H
#define CTCPCLIENT_H
#include <iostream>

class CTCPClient
{
public:
    CTCPClient();
    ~CTCPClient();
public:
    const char* GetErrorMsg()
    {
        return m_strErrorMsg.c_str();
    }

    bool Connect(std::string strIP, int nPort);
    void DisConnect();
    int  GetSockfd() const
    {
        return m_sockfd;
    }

private:
    bool Init();
    void ErrorMsg(const char* arg, ...);
private:
    std::string m_strErrorMsg;
    int         m_sockfd;
};

#endif // CTCPCLIENT_H
