#include <cassert>
#include <fcntl.h>
#include <unistd.h>

#include <oolong/base/Logger.h>
#include <oolong/net/Acceptor.h>
#include <oolong/net/SocketAPI.h>
#include <oolong/net/TcpConnection.h>
#include <oolong/net/EventLoop.h>

using namespace oolong;

Acceptor::Acceptor(EventLoop* loop, const EndPoint& endpoint, bool reusePort) : 
    loop_(loop),
    endpoint_(endpoint),
    acceptSocket_(SocketAPI::createTcpSocket(endpoint.family())),
    acceptChannel_(loop, acceptSocket_.fd()),
    idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))
{
    acceptSocket_.setReusePort(reusePort);
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.bind(endpoint_);
    acceptChannel_.setReadableCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
    ::close(idleFd_);
}

void Acceptor::listen()
{
    assert(!listening_);
    loop_->assertInLoopThread();
    listening_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();
}

void Acceptor::stopListening()
{
    SocketAPI::close(acceptSocket_.fd()); // todo 合适吗
}

void Acceptor::handleRead()
{
    loop_->assertInLoopThread();
    EndPoint peer;
    int connfd = acceptSocket_.accept(&peer);
    if (connfd >= 0)
    {
        if (newConnectionCallback_)
            newConnectionCallback_(connfd, peer);
        else 
            SocketAPI::close(connfd);
    }
    else 
    {
        LogError << "Acceptor::handleRead() errno: " << errno;
        if (errno == EMFILE)
        {
            // 防止cpu100
            ::close(idleFd_);
            idleFd_ = ::accept(acceptSocket_.fd(), nullptr, nullptr);
            ::close(idleFd_);
            idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
        }
    }
}