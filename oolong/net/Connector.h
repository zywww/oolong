#pragma once
#include <functional>

#include <oolong/net/EndPoint.h>
#include <memory>

namespace oolong
{
    class Channel;
    class EventLoop;
    class Connector :   noncopyable,
                        public std::enable_shared_from_this<Connector>
    {
    public:
        using NewConnectionCallback = std::function<void (int sockfd)>;

        Connector(EventLoop* loop, const EndPoint& peer);
        ~Connector();

        void start(); // 线程安全
        void stop(); // 线程安全
        void restart(); 

        void setNewConnectionCallback(NewConnectionCallback cb) { newConnectionCallback_ = std::move(cb); }

    private:
        enum class State { Disconnected, Connecting, Connected };
        static const int kInitRetryDelayMs = 500;
        static const int kMaxRetryDelayMs = 30*1000;

        void startInLoop();
        void stopInLoop();
        void setState(State state) { state_ = state; }
        void connect();
        void connecting(int sockfd);
        void retry(int sockfd);
        void handleWrite();
        void handleError();
        

        EventLoop* loop_;
        EndPoint peer_;
        bool connect_ = false; // 表示是否要链接
        State state_ = State::Disconnected;
        std::unique_ptr<Channel> channel_; // connect时用的,若channel可读则链接成功, 这里的channel不管理fd资源
        NewConnectionCallback newConnectionCallback_;
        int retryDelayMs_ = kInitRetryDelayMs;
    };
} // namespace oolong