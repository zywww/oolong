#include <netinet/tcp.h>
#include <netinet/in.h>
#include <string.h>

#include <oolong/base/Logger.h>
#include <oolong/net/TcpSocket.h>
#include <oolong/net/SocketAPI.h>

using namespace oolong;

TcpSocket::~TcpSocket()
{
    // SocketAPI::close(fd_);
}

void TcpSocket::listen()
{
    SocketAPI::listenOrDie(fd_);
}

void TcpSocket::bind(const EndPoint& local)
{
    SocketAPI::bindOrDie(fd_, local.getSockAddr());
}

void TcpSocket::setReusePort(bool on)
{
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &optval, static_cast<socklen_t>(sizeof optval));
    if (ret < 0)
        LogError << "TcpSocket::setReusePort() errno: " << errno;
}

void TcpSocket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &optval, static_cast<socklen_t>(sizeof optval));
    if (ret < 0)
        LogError << "TcpSocket::setReuseAddr() errno: " << errno;
}

void TcpSocket::setTcpNoDelay(bool on)
{
    int ret = ::setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &on, static_cast<socklen_t>(sizeof on));
    if (ret < 0)
        LogError << "TcpSocket::setTcpNoDelay() errno: " << errno;
}

int TcpSocket::accept(EndPoint* peer)
{
    struct sockaddr_in6 addr;
    bzero(&addr, sizeof addr);
    int connfd = SocketAPI::accept(fd_, &addr);
    if (connfd >= 0)
        peer->setSockAddrIn6(addr);
    return connfd;
}

void TcpSocket::shutdownWrite()
{
    SocketAPI::shutdownWrite(fd_);
}