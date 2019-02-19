#include <oolong/net/TcpServer.h>
#include <oolong/net/TcpConnection.h>
#include <oolong/net/EventLoop.h>
#include <oolong/net/EndPoint.h>

using namespace oolong;
using namespace std;

class EchoServer
{
public:
    EchoServer(EventLoop* loop, const EndPoint listenAddr) :
        loop_(loop),
        server_(loop, listenAddr)
    {
        server_.setMessageCallback(std::bind(&EchoServer::onMessage, this, _1, _2));
    }

    void start()
    {
        server_.start();
    }

private:
    void onMessage(const TcpConnectionPtr conn, Buffer* buf)
    {
        conn->send(*buf);
    }
    
    EventLoop* loop_;
    TcpServer server_;
};

int main()
{
    EventLoop loop;
    EndPoint listenAddr(10010);
    EchoServer server(&loop, listenAddr);
    server.start();
    loop.loop();
}