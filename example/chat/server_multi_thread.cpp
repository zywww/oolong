#include <oolong/base/Logger.h>
#include <oolong/net/Buffer.h>
#include <oolong/net/Callbacks.h>
#include <oolong/net/EndPoint.h>
#include <oolong/net/EventLoop.h>
#include <oolong/net/TcpServer.h>
#include <unordered_set>
#include <mutex>
#include "codec.h"

using namespace oolong;
using namespace std;

class ChatServer
{
public:
    ChatServer(EventLoop* loop, const EndPoint& serverAddr) :
        server_(loop, serverAddr, true),
        codec_(std::bind(&ChatServer::onMessage, this, _1, _2))
    {
        server_.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1, _2));
        server_.setMessageCallback(std::bind(&Codec::onMessage, &codec_, _1, _2));
    }

    void start()
    {
        server_.start();
    }

    void setThreadNum(int num)
    {
        server_.setThreadNum(num);
    }

private:
    void onConnection(const TcpConnectionPtr& conn, bool up)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (up)
        {
            conns_.insert(conn);
            LogInfo << "new client connect";
        }
        else 
        {
            conns_.erase(conn);
            LogInfo << "client disconnect";
        }
    }

    void onMessage(const TcpConnectionPtr& msgConn, const std::string& msg)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const TcpConnectionPtr& conn : conns_)
        {
            if (conn != msgConn)
            {
                conn->send(codec_.encodeMessage(msg));
            }
        }
    }

    TcpServer server_;
    Codec codec_;
    std::mutex mutex_;
    std::unordered_set<TcpConnectionPtr> conns_; 
};

int main(int argc, char* argv[])
{
    if (argc >= 2)
    {
        uint16_t port = std::stoi(argv[1]);
        EndPoint serverAddr(port);
        EventLoop loop;
        ChatServer server(&loop, serverAddr);
        if (argc >= 3)
        {
            int threadNum = std::stoi(argv[2]);
            server.setThreadNum(threadNum);
        }
        server.start();
        loop.loop();
    }
    else 
        LogInfo << "usage: a.out port [thread_num]";

    return 0;
}