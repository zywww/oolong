#include <unistd.h>
#include <iostream>
#include <memory>
#include <string>
#include <atomic>
#include <vector>

#include <oolong/base/Logger.h>
#include <oolong/base/CurrentThread.h>
#include <oolong/net/TcpClient.h>
#include <oolong/net/EventLoop.h>
#include <oolong/net/EventLoopThreadPool.h>
#include <oolong/net/EndPoint.h>
#include <oolong/net/TcpConnection.h>

using namespace std;
using namespace oolong;


class Client;

class Session
{
public:
    Session(Client* owner, 
        EventLoop* loop,
        const EndPoint& serverAddr) :
        owner_(owner),
        client_(loop, serverAddr)
    {
        client_.setConnectionCallback(std::bind(&Session::onConnection, this, _1, _2));
        client_.setMessageCallback(std::bind(&Session::onMessage, this, _1, _2));
    }

    void start()
    {
        client_.connect();
    }

    void stop()
    {
        client_.disconnect();
    }

    uint64_t bytesRead() const { return bytesRead_; }
    uint64_t messagesRead() const { return messagesRead_; }

private:
    void onConnection(const TcpConnectionPtr& conn, bool up);

    void onMessage(const TcpConnectionPtr conn, Buffer* buffer)
    {
        ++messagesRead_;
        bytesRead_ += buffer->readableBytes();
        bytesWritten_ += buffer->readableBytes();
        conn->send(*buffer);
    }

    Client* owner_;
    TcpClient client_;
    uint64_t bytesRead_ = 0;
    uint64_t bytesWritten_ = 0;
    uint64_t messagesRead_ = 0;
};

class Client
{
public:
    Client(EventLoop* loop, 
        EndPoint serverAddr, 
        int threadCount,
        int blocksize,
        int sessionCount,
        int timeout) :
        loop_(loop),
        threadPool_(loop),
        timeout_(timeout),
        sessionCount_(sessionCount)
    {
        loop_->runAfter(timeout, std::bind(&Client::handleTimeout, this));
        if (threadCount > 1)
            threadPool_.setThreadNum(threadCount);
        threadPool_.start();

        for (int i = 0; i < blocksize; ++i)
            message_.push_back(static_cast<char>(i%128));
            // message_ += static_cast<char>(i%128);
        LogDebug << "message size:" << message_.size();
        
        for (int i = 0; i < sessionCount; ++i)
        {
            auto session = std::make_unique<Session>(this, threadPool_.getNextLoop(), serverAddr);
            session->start();
            sessions_.emplace_back(std::move(session));
        }
    }

    void start()
    {
        for (auto& session : sessions_)
            session->start();
    }

    void onConnect()
    {
        if (++numConnected_ == sessionCount_)
            LogInfo << "all client connected";
    }

    void onDisconnect(const TcpConnectionPtr conn)
    {
        if (--numConnected_ == 0)
        {
            LogInfo << "all client disconnected";
            uint64_t totalBytesRead = 0;
            uint64_t totalMessageRead = 0;
            for (const auto& session : sessions_)
            {
                totalBytesRead += session->bytesRead();
                totalMessageRead += session->messagesRead();
            }            
            LogInfo << totalBytesRead << " total bytes read";
            LogInfo << totalMessageRead << " total message read";
            LogInfo << static_cast<double>(totalBytesRead) / (timeout_*1024*1024) << " MiB/s throughput";
            conn->getLoop()->queueInLoop(std::bind(&Client::quit, this));
        }
    }

    const string& message() const 
    {
        LogDebug << "get message size:" << message_.size();
        return message_;
    }

private:
    void quit()
    {
        loop_->queueInLoop(std::bind(&EventLoop::quit, loop_));
    }

    void handleTimeout()
    {
        LogInfo << "timeout";
        for (auto& session : sessions_)
            session->stop();
    }

    EventLoop* loop_;
    EventLoopThreadPool threadPool_;
    int timeout_;
    string message_;
    int sessionCount_;
    std::vector<std::unique_ptr<Session>> sessions_;
    std::atomic<uint32_t> numConnected_{0};
};

void Session::onConnection(const TcpConnectionPtr& conn, bool up)
{
    if (up)
    {
        conn->send(owner_->message());
        owner_->onConnect();
    }
    else 
    {
        owner_->onDisconnect(conn);
    }
}

int main(int argc, char* argv[])
{
    if (argc != 7)
    {
        cout << "usage: ./a.out server_ip port threads blocksize sessions time" << endl;
    }
    else
    {
        LogInfo << "pid = " << getpid() << ", tid = " << currentThreadTid();
        const char* ip = argv[1];
        uint16_t port = stoi(string(argv[2]));
        int threadCount = stoi(string(argv[3]));
        int blocksize = stoi(string(argv[4]));
        int sessionCount = stoi(string(argv[5]));
        int timeout = atoi(argv[6]);
        
        EventLoop loop;
        EndPoint serverAddr(ip, port);
        Client client(&loop, serverAddr, threadCount, blocksize, sessionCount, timeout);
        loop.loop();
    }
    
}