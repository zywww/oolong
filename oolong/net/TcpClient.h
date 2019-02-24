#pragma once
#include <mutex>

#include <oolong/base/noncopyable.h>
#include <oolong/net/EndPoint.h>
#include <oolong/net/Callbacks.h>

namespace oolong
{
    class EventLoop;
    class Connector;
    class TcpClient : noncopyable
    {
    public:
        using ConnectorPtr = std::shared_ptr<Connector>;
        TcpClient(EventLoop* loop, const EndPoint& peer);
        ~TcpClient();

        void connect(); // 线程安全
        void disconnect(); // 线程安全
        void stop(); // 线程安全

        bool retry() const { return retry_; }
        void enableRetry() { retry_ = true; }
        
        void setConnectionCallback(ConnectionCallback cb) { connectionCallback_ = std::move(cb); }
        void setMessageCallback(MessageCallback cb) { messageCallback_ = std::move(cb); }
        void setWriteCompleteCallback(WriteCompleteCallback cb) { writeCompleteCallback_ = std::move(cb); }

    private:
        void newConnection(int sockfd);
        void removeConnectionByPeer(const TcpConnectionPtr& conn);
        void removeConnection(const TcpConnectionPtr& conn);

        EventLoop* loop_;

        ConnectorPtr connector_;
        bool retry_ = false;
        bool connect_ = true;
        ConnectionCallback connectionCallback_;
        MessageCallback messageCallback_;
        WriteCompleteCallback writeCompleteCallback_;

        mutable std::mutex mutex_;
        TcpConnectionPtr connection_;
    };
} // namespace oolong