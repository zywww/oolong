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
        server_(loop, serverAddr, true)
    {
        server_.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1, _2));
        server_.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2));
    }

    void setThreadNum(int num)
    {
        server_.setThreadNum(num);
    }

    void start()
    {
        server_.start();
    }
private:
    void onConnection(const TcpConnectionPtr& conn, bool up)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (up)
        {
            conns_.insert(conn);
            LogInfo << "new client connect";
            //LogInfo << "conns_.size()=" << conns_.size();
        }
        else 
        {
            conns_.erase(conn);
            LogInfo << "client disconnect";
        }
    }

    void onMessage(const TcpConnectionPtr& msgConn, Buffer* buf)
    {
        std::string msg;
        while (codec_.decodeMessage(buf, msg))
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (const TcpConnectionPtr& conn : conns_)
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
    if (argc >= 3)
    {
        uint16_t port = std::stoi(argv[1]);
        int threads = std::stoi(argv[2]);
        EndPoint serverAddr(port);
        EventLoop loop;
        ChatServer server(&loop, serverAddr);
        server.setThreadNum(threads);
        server.start();
        loop.loop();
    }
    else 
        LogInfo << "usage: a.out port threads";

    return 0;
}