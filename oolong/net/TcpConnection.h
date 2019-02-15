#pragma once
#include <string>

#include <oolong/base/noncopyable.h>
#include <oolong/net/Channel.h>
#include <oolong/net/EndPoint.h>
#include <oolong/net/Callbacks.h>
#include <oolong/net/Buffer.h>

namespace oolong
{
    class EventLoop;
    class TcpSocket;

    // 表示一条连接,一旦断连,本对象无用
    // 如果在有数据未写完时关链接,先发送数据再关闭
    // 在socket上的封装
    class TcpConnection : noncopyable,
                          public std::enable_shared_from_this<TcpConnection>
    {
    public:
        TcpConnection(EventLoop* loop, int sockfd, const EndPoint& local, const EndPoint& peer);
        ~TcpConnection();

        void setMessageCallback(MessageCallback cb) { messageCallback_ = std::move(cb); }
        void setConnectionCallback(ConnectionCallback cb) { connectionCallback_ = std::move(cb); }
        void setWriteCompleteCallback(WriteCompleteCallback cb) { writeCompleteCallback_ = std::move(cb); }
        void setCloseCallback(CloseCallback cb) { closeCallback_ = std::move(cb); } // 内部使用?

        void forceClose(); // 主动关闭,丢弃未发送数据
        void shutdown(); // 关闭,但先发送完数据
        bool send(const char* data, size_t len); // 返回值为true,表示可尝试发送,false表示无法发送(可能已断开连接)
        bool send(Buffer& buffer);
        bool send(const std::string& msg);
        
        void connectionEstablished(); // for TcpServer
        void connectionDestroyed();
        EventLoop* getLoop() const { return loop_; }
        
    private:
        enum class State 
        { 
            Disconnected,
            Connecting,
            Connected, 
            Disconnecting
        };
        void setState(State newState) { state_ = newState; }

        void handleRead();
        void handleWrite();
        void handleClose();
        void handleError();

        void forceCloseInLoop();        
        void shutdownInLoop();
        void sendInLoop(const std::string& msg);
        void sendInLoop(const char* data, size_t len);

        State state_ = State::Connecting;
        EventLoop* loop_;
        std::unique_ptr<TcpSocket> socket_;
        Channel channel_;
        MessageCallback messageCallback_;
        ConnectionCallback connectionCallback_;
        WriteCompleteCallback writeCompleteCallback_;
        CloseCallback closeCallback_;
        Buffer outputBuffer_;
        Buffer inputBuffer_;
        const EndPoint localAddr_;
        const EndPoint peerAddr_;
    };
} // namespace oolong