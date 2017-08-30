#include "ctcpserver.h"
#include <unistd.h>
#include <signal.h>
#include <sstream>

CTCPServer server;

void TermServer(int sig)
{
    server.StopServer();
}

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        std::cout << "too few args, uses ";
        std::cout << argv[0] << " ip port" << std::endl;
        return 0;
    }

    signal(SIGINT, TermServer);

    std::string strIp = argv[1];
    std::stringstream is;
    is << argv[2];
    int nPort = 0;
    is >> nPort;

    bool bRet = server.StartServer(strIp, nPort);
    if (!bRet)
        std::cout << "start server error: " << server.GetErrorMsg() << std::endl;
    return 0;

}
