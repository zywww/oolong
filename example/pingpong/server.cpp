#include <string>
#include <iostream>

#include <oolong/net/EndPoint.h>
#include <oolong/net/EventLoop.h>
#include <oolong/net/TcpServer.h>
#include <oolong/net/TcpConnection.h>

using namespace oolong;
using namespace std;

void onMessage(TcpConnectionPtr conn, Buffer* buffer)
{
    conn->send(*buffer);
}

int main(int argc, char* argv[])
{
    if (argc < 4)
    {
        cout << "usage: ./a.out ip port threadNum" << endl;
    }
    else
    {
        std::string ip(argv[1]);
        uint16_t port = std::stoi(std::string(argv[2]));
        EndPoint listenAddr(ip, port);
        int threadNum = std::stoi(std::string(argv[3]));
        
        EventLoop loop;
        TcpServer server(&loop, listenAddr);

        server.setMessageCallback(onMessage);

        if (threadNum > 1)
            server.setThreadNum(threadNum);

        server.start();

        loop.loop();
    }
}