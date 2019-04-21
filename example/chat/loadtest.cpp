#include <oolong/base/Logger.h>
#include <oolong/base/Timestamp.h>
#include <oolong/net/TcpConnection.h>
#include <oolong/net/EventLoopThreadPool.h>
#include <oolong/net/EndPoint.h>
#include <oolong/net/EventLoop.h>
#include <oolong/net/TcpClient.h>
#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>
#include <stdio.h>
#include "codec.h"

using namespace oolong;
using namespace std;

class Stat
{
public:
    Stat(int numSession) :
        session_(numSession)
    {
    }

    void dumpStat()
    {
        vector<double> seconds;
        seconds.reserve(session_);
        for (auto t : recivedTimes_)
            seconds.push_back(Timestamp::timeDifference(t, startTime_));
        
        std::sort(seconds.begin(), seconds.end());
        for (size_t i = 0; i < seconds.size(); i += std::max(static_cast<size_t>(1), seconds.size()/20))
        {
            printf("%6d%% %.6f\n", i*100/seconds.size(), seconds[i]);
        }
        printf("%6d%% %.6f\n", 100, seconds.back());
    }

    //void incSended() { ++sended_; }
    //int getSended() const { return sended_; }
    int getSession() const { return session_; }
    int incConnected() { ++connected_; }
    int getConnected() const { return connected_; }
    void setStartTime(Timestamp t) { startTime_ = t; }
    void addRecivedTime(Timestamp t) 
    { 
        std::lock_guard<std::mutex> lock(mutex_);
        recivedTimes_.push_back(t);
        if (recivedTimes_.size() == session_)
            dumpStat();
    }


private:
    const int session_;
    //atomic<int> sended_{0};
    atomic<int> connected_{0};
    Timestamp startTime_;
    std::mutex mutex_;
    vector<Timestamp> recivedTimes_;
};

class Session
{
public:
    Session(EventLoop* loop, Stat* stat, const EndPoint& serverAddr) :
        loop_(loop),
        client_(loop, serverAddr),
        stat_(stat)
    {
        client_.setConnectionCallback(std::bind(&Session::onConnection, this, _1, _2));
        client_.setMessageCallback(std::bind(&Session::onMessage, this, _1, _2));
    }

    void start()
    {
        // client_.enableRetry();
        client_.connect();
    }

    void send(const TcpConnectionPtr& conn, const std::string& msg)
    {
        conn->send(codec_.encodeMessage(msg));
        stat_->setStartTime(Timestamp::now());
        LogInfo << "sended";
    }

private:
    void onConnection(const TcpConnectionPtr& conn, bool up)
    {
        // todo 这个是在另外线程调用吧
        if (up)
        {
            stat_->incConnected();
            if (stat_->getConnected() == stat_->getSession())
            {
                LogInfo << "all " << stat_->getConnected() << " clients connected";
                loop_->runAfter(10, std::bind(&Session::send, this, conn, "hello"));
            }
            // if (conn->send(codec_.encodeMessage("xx")) == false) 
            //     LogInfo << "send false";
        }
        else 
            LogInfo << "disconnected";
    }

    void onMessage(const TcpConnectionPtr& conn, Buffer* buf)
    {
        stat_->addRecivedTime(Timestamp::now());
    }

    EventLoop* loop_;
    TcpClient client_;
    Codec codec_;
    Stat* stat_;
};

class ChatClientManager
{
public:
    ChatClientManager(EventLoop* loop, const EndPoint& serverAddr, int numSession, int numThread) : 
        pool_(loop),
        stat_(numSession)
    {
        if (numThread >= 0)
            pool_.setThreadNum(numThread);
        pool_.start();

        for (int i = 0; i < numSession; ++i)
        {
            auto up = std::make_unique<Session>(pool_.getNextLoop(), &stat_, serverAddr);
            up->start();
            usleep(200);
            sessions_.emplace_back(std::move(up));
        }
    }


private:
    EventLoopThreadPool pool_;
    vector<unique_ptr<Session>> sessions_;
    Stat stat_;
};


// fixme:不加usleep会出现服务端和客户端连接数不一致
int main(int argc, char* argv[])
{
    if (argc >= 3)
    {
        int port = std::stoi(argv[1]);
        int sessions = std::stoi(argv[2]);
        int threads = std::stoi(argv[3]);
        EventLoop loop;
        EndPoint serverAddr(port);
        ChatClientManager mgr(&loop, serverAddr, sessions, threads);
        loop.loop();
    }
    else 
        cout << "usage: a.out port sessions threads" << endl;
}