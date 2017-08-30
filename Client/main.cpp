#include <iostream>
#include "ctcpclient.h"
#include <sys/time.h>
#include <sys/select.h>
#include <unistd.h>
#include <signal.h>
#include <thread>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>

using namespace std;

fd_set r_set, w_set;

void TermFunc(int nSigNo)
{
    std::cout << "done" << std::endl;
    write(STDERR_FILENO, '0', 1); // terminate thread
}

int main(char* argv[], int argc)
{
    if (argc < 3)
    {
        std::cout << "too few args, uses ";
        std::cout << argv[0] << " ip port" << std::endl;
        return 0;
    }

    // register siganl
    signal(SIGTERM, TermFunc);

    //set nonblock
    fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL, 0) | O_NONBLOCK);
    fcntl(STDERR_FILENO, F_SETFL, fcntl(STDERR_FILENO, F_GETFL, 0) | O_NONBLOCK);

    FD_ZERO(&r_set);
    FD_ZERO(&w_set);
    CTCPClient client;
    bool bConnectSuccess = false;
    bool bRet = client.Connect(argv[1], argv[2]);
    if (!bRet)
    {
        std::cout << client.GetErrorMsg() << std::endl;
        return 0;
    }
    // add sockfd to w_set, test if connect

    std::thread th = std::move([=](){
        while(1)
        {
            if (!bConnectSuccess)
            {
                FD_ZERO(&w_set);
                FD_SET(client.GetSockfd(), &w_set);
            }
            FD_ZERO(&r_set);
            FD_SET(client.GetSockfd(), &r_set);
            FD_SET(STDIN_FILENO, &r_set);
            FD_SET(STDERR_FILENO, &r_set); // terminate thread


            int nRet = select(client.GetSockfd() + 1, &r_set, &w_set, NULL, NULL);
            if (nRet < 0)
            {
                std::cout << "select error" << std::endl;
                return;
            }

            if (FD_ISSET(client.GetSockfd(), &w_set))
            { // connect success or not
                int nRet = -1;
                getsockopt(client.GetSockfd(), SOL_SOCKET, SO_ERROR, &nRet, sizeof(int));
                if (nRet != 0)
                {
                    std::cout << "connect error" << std::endl;
                    return;
                }
                bConnectSuccess = true; // not detect agin
            }
            if (FD_ISSET(client.GetSockfd(), &r_set))
            {
                char szBuff[1024] = { 0x00 };
                int nRet = recv(client.GetSockfd(), szBuff, sizeof(szBuff), 0);
                if (nRet <= 0) // socket close
                    return;
            }
            if (FD_ISSET(STDIN_FILENO, &r_set))
            {
                // send data
                char szBuff[1024] = { 0x00 };
                int nRet = read(STDIN_FILENO, szBuff, sizeof(szBuff));
                send(client.GetSockfd(), szBuff, nRet, 0);
            }
            if (FD_ISSET(STDERR_FILENO, &r_set))
            { // terminate thread;
                return;
            }
        }
    });

    th.join(); // wait terminate
    client.DisConnect();

    return 0;
}
