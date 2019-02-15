#pragma once
#include <functional>

#include <oolong/base/noncopyable.h>
#include <oolong/net/EndPoint.h>
#include <oolong/net/Channel.h>
#include <oolong/net/TcpSocket.h>

namespace oolong
{
    class EventLoop;

    // 内部使用
    // 接受新链接
    class Acceptor : noncopyable
    {
    public:
        using NewConnectionCallback = std::function<void(int sockfd, const EndPoint&)>;
        Acceptor(EventLoop* loop, const EndPoint& endpoint, bool reusePort);
        ~Acceptor();

        bool listening() const { return listening_; }
        void listen(); // loop 线程调用
        void stopListening();

        void setNewConnectionCallback(NewConnectionCallback cb) { newConnectionCallback_ = std::move(cb); }

    private:
        void handleRead();

        bool listening_ = false;
        EventLoop* loop_;
        const EndPoint endpoint_;
        TcpSocket acceptSocket_;
        Channel acceptChannel_;
        NewConnectionCallback newConnectionCallback_;

        int idleFd_;
    };
} // namespace oolong