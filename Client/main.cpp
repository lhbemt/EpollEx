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
#include <sstream>

using namespace std;

fd_set r_set, w_set;
int pipefd[2]; // ternimate thread

void TermFunc(int nSigNo)
{
    std::cout << "done" << std::endl;
	int nRet = write(pipefd[1], "0", 1); // terminate thread
	return;
}

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        std::cout << "too few args, uses ";
        std::cout << argv[0] << " ip port" << std::endl;
        return 0;
    }

    // register siganl
    signal(SIGINT, TermFunc);
	int pipeerr = pipe(pipefd);
	if (pipeerr == -1)
	{
		std::cout << "pipe error" << std::endl;
		return 0;
	}
	fcntl(pipefd[0], F_SETFL, fcntl(pipefd[0], F_GETFL, 0) | O_NONBLOCK);

    //set nonblock
    fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL, 0) | O_NONBLOCK);

    FD_ZERO(&r_set);
    FD_ZERO(&w_set);
    CTCPClient client;
    bool bConnectSuccess = false;
	std::string strIP = argv[1];
	std::stringstream is;
	int nPort = 0;
	is << argv[2];
	is >> nPort;
    bool bRet = client.Connect(strIP, nPort);
    if (!bRet)
    {
        std::cout << client.GetErrorMsg() << std::endl;
        return 0;
    }
    // add sockfd to w_set, test if connect
	
	int maxfd = (pipefd[0] > client.GetSockfd()) ? pipefd[0] : client.GetSockfd();	

    std::thread th = std::move(std::thread([=, &bConnectSuccess](){
        while(1)
        {
            if (!bConnectSuccess)
            {
                FD_ZERO(&w_set);
                FD_SET(client.GetSockfd(), &w_set);
            }
			else
			{
				FD_ZERO(&w_set);
				FD_ZERO(&r_set);
				FD_SET(client.GetSockfd(), &r_set);
				FD_SET(STDIN_FILENO, &r_set);
				FD_SET(client.GetSockfd(), &r_set); //read from tcp
				FD_SET(pipefd[0], &r_set); // terminate thread
			}


            int nRet = select(maxfd + 1, &r_set, &w_set, NULL, NULL);
            if (nRet < 0)
            {
                std::cout << "select error" << std::endl;
                return;
            }

            if (FD_ISSET(client.GetSockfd(), &w_set))
            { // connect success or not
                int nRet = -1;
				socklen_t nLen = sizeof(int);
                getsockopt(client.GetSockfd(), SOL_SOCKET, SO_ERROR, &nRet, &nLen);
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
				else
				  std::cout << szBuff;
            }
            if (FD_ISSET(STDIN_FILENO, &r_set))
            {
                // send data
                char szBuff[1024] = { 0x00 };
                int nRet = read(STDIN_FILENO, szBuff, sizeof(szBuff));
				if (nRet < 0)
					return;
                nRet = send(client.GetSockfd(), szBuff, nRet, 0);
				if (nRet < 0)
				{
					if (errno != EAGAIN || errno != EWOULDBLOCK)
					{
						std::cout << "send error, error code: " << errno << std::endl;
						return;
					}
				}
            }
		   if (FD_ISSET(pipefd[0], &r_set))
		   {
			   char szBuff[20] = { 0x00 };
			   int nRet = read(pipefd[0], szBuff, sizeof(szBuff));
			   if (nRet < 0)
			   {
				   if (errno == EAGAIN) // not ternimate
					 continue;
				   else
					 std::cout << "terminate read error, errorcode: " << errno <<std::endl;
			   }
			   return;
		   }
        }
    }));

    th.join(); // wait terminate
    client.DisConnect();
	close(pipefd[0]);
	close(pipefd[1]);

    return 0;
}
