#include <oolong/base/Logger.h>
#include <oolong/net/EndPoint.h>
#include <oolong/net/EventLoopThread.h>
#include <oolong/net/TcpClient.h>
#include <oolong/net/EventLoop.h>
#include <oolong/net/Callbacks.h>
#include <oolong/net/TcpConnection.h>
#include <oolong/net/Buffer.h>
#include "codec.h"
#include <string>
#include <mutex>
#include <functional>
#include <iostream>

using namespace oolong;
using namespace std;

class ChatClient
{
public:
    ChatClient(EventLoop* loop, const EndPoint& serverAddr) 
        : loop_(loop),
          codec_(std::bind(&ChatClient::onMessage, this, _1, _2)),
          client_(loop, serverAddr)
        {
            client_.setMessageCallback(std::bind(&Codec::onMessage, &codec_, _1, _2));
            client_.setConnectionCallback(std::bind(&ChatClient::onConnection, this, _1, _2));
        }

    void connect()
    {
        client_.connect();
    }

    void disconnect()
    {
        client_.disconnect();
    }

    void sendMessage(const std::string& msg)
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (conn_)
        {
            std::string toSend = codec_.encodeMessage(msg);// 相比muduo多了一次复制,但是codec的责任更单纯
            conn_->send(toSend);
        }
    }

private:
    void onConnection(const TcpConnectionPtr& conn, bool up)
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (up)
        {
            conn_ = conn;
            LogInfo << "connect to server";
        }
        else 
        {
            conn_.reset();
            LogInfo << "disconnected";
        }
    }

    void onMessage(const TcpConnectionPtr& conn, const std::string& msg)
    {
        cout << ">>" << msg << endl;
    }

    EventLoop* loop_;
    Codec codec_;
    TcpClient client_;
    std::mutex mutex_;
    TcpConnectionPtr conn_;
};

int main(int argc, char* argv[])
{
    if (argc >= 2)
    {
        uint16_t port = std::stoi(argv[1]);
        EndPoint serverAddr(port);
        EventLoopThread thread;
        ChatClient client(thread.startLoop(), serverAddr);
        client.connect();

        string line;
        while (cin >> line)
        {
            client.sendMessage(line);
        }
        client.disconnect();
    }
    else 
        LogInfo << "usage: a.out port";
        
}