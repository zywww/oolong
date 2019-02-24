#include <cassert>

#include <oolong/base/Logger.h>
#include <oolong/net/SocketAPI.h>
#include <oolong/net/TcpSocket.h>
#include <oolong/net/TcpConnection.h>
#include <oolong/net/EventLoop.h>

using namespace oolong;


namespace oolong
{
    void defaultConnectionCallback(const TcpConnectionPtr& conn, bool up)
    {
        LogDebug << "connection " << (up?"up":"down");
    }    

    void defaultMessageCallback(const TcpConnectionPtr& conn, Buffer* buffer)
    {
        buffer->retrieveAll();
    }
} // namespace oolong



TcpConnection::TcpConnection(EventLoop* loop, int sockfd, const EndPoint& local, const EndPoint& peer) :
    loop_(loop),
    socket_(std::make_unique<TcpSocket>(sockfd)),
    channel_(loop, sockfd),
    localAddr_(local),
    peerAddr_(peer)
{
    socket_->setTcpNoDelay(true);
    channel_.setReadableCallback(std::bind(&TcpConnection::handleRead, this));
    channel_.setWritableCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_.setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    channel_.setErrorCallback(std::bind(&TcpConnection::handleError, this));
    LogDebug << "TcpConnection::ctor fd: " << channel_.fd();
}

TcpConnection::~TcpConnection()
{
    LogDebug << "TcpConnction::dtor fd: " << channel_.fd();
    // assert(state_ == State::Disconnected);
}

void TcpConnection::forceClose()
{
    if (state_ == State::Connected || state_ == State::Disconnecting) // todo 非线程安全
    {
        setState(State::Disconnecting);
        loop_->queueInLoop(std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
    }
}

void TcpConnection::forceCloseInLoop()
{
    loop_->assertInLoopThread();
    if (state_ == State::Connected || state_ == State::Disconnecting)
    {
        handleClose();
    }
}

void TcpConnection::shutdown()
{
    if (state_ == State::Connected)
    {
        setState(State::Disconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, shared_from_this()));
    }
}

void TcpConnection::shutdownInLoop()
{
    loop_->assertInLoopThread();
    if (!channel_.isWriting())
    {
        socket_->shutdownWrite();
    }
}

bool TcpConnection::send(const char* data, size_t len)
{
    if (state_ == State::Connected)
    {
        if (loop_->isInLoopThread())
            sendInLoop(data, len);
        else
        {
            void (TcpConnection::*fp)(const std::string&) = &TcpConnection::sendInLoop;
            loop_->runInLoop(std::bind(fp, this, std::string(data, len)));
        }
        return true;
    }
    return false;
}

bool TcpConnection::send(Buffer& buffer)
{
    if (send(buffer.peek(), buffer.readableBytes()))
    {
        buffer.retrieveAll();
        return true;
    }
    return false;
}

bool TcpConnection::send(const std::string& msg)
{
    return send(msg.data(), msg.size());
}

void TcpConnection::sendInLoop(const std::string& msg)
{
    sendInLoop(msg.data(), msg.size());
}

void TcpConnection::sendInLoop(const char* data, size_t len)
{
    loop_->assertInLoopThread();
    if (state_ == State::Disconnected)
    {
        LogWarn << "disconnected, give up writing";
        return;
    }

    // 如果输出队列为空,则尝试直接发送
    ssize_t nwrote = 0;
    size_t remaining = len;
    // bool fatalError = false;
    if (!channel_.isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = SocketAPI::write(channel_.fd(), data, len);
        if (nwrote >= 0)
        {
            remaining = len - nwrote;
            if (remaining == 0 && writeCompleteCallback_)
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
        }
        else
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK)
            {
                LogError << "TcpConnection::sendInLoop()";
                // todo 看看muduo的fatalerror可能出现的错误
            }
        }
    }

    // 将剩余未发送 buffer 先缓存起来
    assert(remaining <= len);
    if (remaining > 0)
    {
        outputBuffer_.append(data+nwrote, remaining);
        if (!channel_.isWriting())
            channel_.enableWriting();
    }
}

void TcpConnection::connectionEstablished()
{
    loop_->assertInLoopThread();
    assert(state_ == State::Connecting);
    setState(State::Connected);
    channel_.enableReading();
    connectionCallback_(shared_from_this(), true);
}

void TcpConnection::connectionDestroyed()
{
    loop_->assertInLoopThread();
    if (state_ == State::Connected)
    {
        state_ = State::Disconnected;
        channel_.disableAll();

        connectionCallback_(shared_from_this(), false);
    }    
    // todo 想清楚怎么链接断开要怎么处理
}

void TcpConnection::handleRead()
{
    loop_->assertInLoopThread();
    ssize_t n = inputBuffer_.readFd(channel_.fd()); // todo 这里的read是buffer的成员,而write却不是,合适吗
    if (n > 0 && messageCallback_)
        messageCallback_(shared_from_this(), &inputBuffer_);
    else if (n == 0)
        handleClose(); // todo 收到消息长度0表示关闭,那channel绑定handleClose有什么用,
        // 会不会导致调用两次handleClose
    else 
        LogError << "TcpConnection::handleRead() errno: " << errno;

}

void TcpConnection::handleWrite()
{
    loop_->assertInLoopThread();
    if (channel_.isWriting())
    {
        ssize_t n = SocketAPI::write(channel_.fd(), outputBuffer_.peek(), outputBuffer_.readableBytes());
        if (n > 0)
        {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0)
            {
                channel_.disableWriting();
                if (writeCompleteCallback_)
                    loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                if (state_ == State::Disconnecting)
                {
                    //shutdownInLoop();
                }
            }
        }
        else
        {
            LogError << "TcpConnection::handleWrite() errno: " << errno;
        }
    }
    else 
    {
        LogInfo << "connection is no more writing()";
    }
}

void TcpConnection::handleClose()
{
    loop_->assertInLoopThread();
    assert(state_ == State::Connected || state_ == State::Disconnecting);
    setState(State::Disconnected);
    channel_.disableAll();

    connectionCallback_(shared_from_this(), false);
    closeCallback_(shared_from_this());
}

void TcpConnection::handleError()
{
    loop_->assertInLoopThread();
    int error = SocketAPI::getSocketError(channel_.fd());
    LogError << "TcpConnecting::handleError() errno: " << error;
}