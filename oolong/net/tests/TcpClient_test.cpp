#include <oolong/base/Logger.h>
#include <oolong/net/Buffer.h>
#include <oolong/net/TcpConnection.h>
#include <oolong/net/TcpClient.h>
#include <oolong/net/EndPoint.h>
#include <oolong/net/EventLoop.h>

using namespace oolong;

class EchoClient
{
public:
    EchoClient(EventLoop* loop, const EndPoint& peer) :
        client_(loop, peer),
        loop_(loop)
    {
        client_.setConnectionCallback(std::bind(&EchoClient::connectionCallback, this, _1, _2));
        client_.setMessageCallback(std::bind(&EchoClient::messageCallback, this, _1, _2));
    }

    void start()
    {
        client_.connect();
    }

private:
    void connectionCallback(TcpConnectionPtr conn, bool up)
    {
        LogInfo << "connection is " << (up?"up":"down");
        conn->send("hello");
    }
    
    void messageCallback(TcpConnectionPtr conn, Buffer* msg)
    {
        LogInfo << "receive msg: " << msg->retrieveAllAsString();
        loop_->quit();
    }

    TcpClient client_;
    EventLoop* loop_;
};

int main()
{
    EventLoop loop;
    EndPoint peer(10010);
    EchoClient client(&loop, peer);
    client.start();
    loop.loop();

    return 0;
}