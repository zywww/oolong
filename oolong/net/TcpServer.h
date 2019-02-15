#pragma once

#include <functional>
#include <memory>
#include <atomic>
#include <unordered_set>

#include <oolong/net/Acceptor.h>
#include <oolong/net/EndPoint.h>
#include <oolong/net/Callbacks.h>
#include <oolong/net/EventLoopThreadPool.h>

namespace oolong
{
    class TcpConnection;
    class Buffer;

    class TcpServer //: noncopyable
    {
    public:
        using ThreadInitCallback = std::function<void(EventLoop*)>;
        TcpServer(EventLoop* loop, const EndPoint& endpoint, bool reusePort = true);
        ~TcpServer();

        void setMessageCallback(MessageCallback cb) { messageCallback_ = std::move(cb); }
        void setConnectionCallback(ConnectionCallback cb) { connectionCallback_ = std::move(cb); }
        void setWriteCompleteCallback(WriteCompleteCallback cb) { writeCompleteCallback_ = std::move(cb); }

        // 必须在调用 start 之前设置线程数
        void setThreadNum(size_t num);
        void setThreadInitCallback(const ThreadInitCallback& cb) { threadInitCallback_ = cb; }

        void start(); // 线程安全
        void stop(); // todo 线程安全

        void removeConnection(TcpConnectionPtr conn);
        
    private:
        void newConnection(int sockfd, const EndPoint&);
        void removeConnectionInLoop(TcpConnectionPtr conn);
        
        std::atomic<bool> started_{false};
        EventLoop* loop_;
        Acceptor acceptor_;
        EventLoopThreadPool threadPool_;
        MessageCallback messageCallback_;
        ConnectionCallback connectionCallback_;
        WriteCompleteCallback writeCompleteCallback_;
        std::unordered_set<TcpConnectionPtr> connections_;
        ThreadInitCallback threadInitCallback_;
    };
} // namespace oolong