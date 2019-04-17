#include <oolong/base/Logger.h>
#include <oolong/net/SocketAPI.h>
#include <oolong/net/EventLoop.h>
#include <oolong/net/Connector.h>

using namespace oolong;

const int Connector::kMaxRetryDelayMs;

Connector::Connector(EventLoop* loop, const EndPoint& peer) : 
    loop_(loop),
    peer_(peer)
{
    LogDebug << "Connector::Connector()";   
}

Connector::~Connector()
{
    // todo
    LogDebug << "Connector::~Connector()";
}

void Connector::start()
{
    connect_ = true;
    loop_->runInLoop(std::bind(&Connector::startInLoop, shared_from_this()));
}

void Connector::startInLoop()
{
    LogDebug << "Connector::startInLoop()";
    loop_->assertInLoopThread();
    assert(state_ == State::Disconnected);
    if (connect_)
        connect();
    else
        LogDebug << "do no connect";
}

void Connector::connect()
{
    int sockfd = SocketAPI::createTcpSocket(peer_.family());
    int ret = SocketAPI::connect(sockfd, peer_.getSockAddr());
    int savedErrno = ret == 0 ? 0 : errno;
    switch (savedErrno)
    {
        case 0:
        case EINPROGRESS:
        case EINTR:
        case EISCONN:
        connecting(sockfd);
        break;

        case EAGAIN:
        case EADDRINUSE:
        case EADDRNOTAVAIL:
        case ECONNREFUSED:
        case ENETUNREACH:
        retry(sockfd);
        break;

        case EACCES:
        case EPERM:
        case EAFNOSUPPORT:
        case EALREADY:
        case EBADF:
        case EFAULT:
        case ENOTSOCK:
        LogError << "connect error in Connector::startInLoop " << savedErrno;
        SocketAPI::close(sockfd);
        break;

        default:
        LogError << "Unexpected error in Connector::startInLoop " << savedErrno;
        SocketAPI::close(sockfd);
        // connectErrorCallback_();
        break;
    }
}

void Connector::connecting(int sockfd)
{
    LogDebug << "Connector::connecting";
    setState(State::Connecting);
    channel_ = std::make_unique<Channel>(loop_, sockfd, false); // 这里的channel不管理fd资源
    channel_->setWritableCallback(std::bind(&Connector::handleWrite, shared_from_this()));
    channel_->setErrorCallback(std::bind(&Connector::handleError, shared_from_this()));
    channel_->enableWriting();
}

void Connector::handleWrite()
{
    if (state_ == State::Connecting)
    {
        assert(channel_);
        int sockfd = channel_->fd();
        channel_.reset();
        int err = SocketAPI::getSocketError(sockfd);
        if (err)
        {
            LogError << "Connector::handleWrite() sock error " << err;
            retry(sockfd);
        }
        else if (SocketAPI::isSelfConnect(sockfd))
        {
            LogWarn << "Connector::handleWrite() self connect";
            retry(sockfd);
        }
        else
        {
            setState(State::Connected);
            if (connect_)
                newConnectionCallback_(sockfd);
            else
                SocketAPI::close(sockfd);
        }
    }
}

void Connector::handleError()
{
    LogError << "Connector::handleError() errno: " << errno;
}

void Connector::retry(int sockfd)
{
    SocketAPI::close(sockfd);
    setState(State::Disconnected);
    if (connect_)
    {
        LogDebug << "Connector::retry() in " << retryDelayMs_ << " milliseconds";
        loop_->runAfter(retryDelayMs_/1000.0, std::bind(&Connector::startInLoop, shared_from_this()));
        retryDelayMs_ = std::min(retryDelayMs_*2, kMaxRetryDelayMs);
    }
    else    
        LogDebug << "do not connect";
}

void Connector::stop()
{
    connect_ = false;
    loop_->queueInLoop(std::bind(&Connector::stopInLoop, shared_from_this()));
}

void Connector::stopInLoop()
{
    loop_->assertInLoopThread();
    if (state_ == State::Connecting)
    {
        setState(State::Disconnected);
        if (channel_)
        {
            int sockfd = channel_->fd();
            channel_.reset();
            retry(sockfd);
        }
    }
}

void Connector::restart()
{
    loop_->assertInLoopThread();
    setState(State::Disconnected);
    retryDelayMs_ = kInitRetryDelayMs;
    connect_ = true;
    startInLoop();
}
