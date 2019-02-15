#include <string.h>
#include <unistd.h>
#include <cassert>

#include <oolong/base/Logger.h>
#include <oolong/net/SocketAPI.h>

using namespace oolong;

int SocketAPI::createTcpSocket(sa_family_t family)
{
    int sockfd = ::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0)
        LogFatal << "SocketApi::createTcpSocket() errno: " << errno;
    return sockfd;
}

struct sockaddr_in6 SocketAPI::getLocalAddr(int sockfd)
{
    struct sockaddr_in6 localAddr;
    bzero(&localAddr, sizeof localAddr);
    socklen_t addrlen = static_cast<socklen_t>(sizeof localAddr);
    if (::getsockname(sockfd, SocketAPI::sockAddrCast(&localAddr), &addrlen) < 0)
        LogError << "SocketAPI::getLocalAddr() errno: " << errno;
    return localAddr;
}

struct sockaddr_in6 SocketAPI::getPeerAddr(int sockfd)
{
    struct sockaddr_in6 peerAddr;
    bzero(&peerAddr, sizeof peerAddr);
    socklen_t addrlen = static_cast<socklen_t>(sizeof peerAddr);
    if (::getpeername(sockfd, SocketAPI::sockAddrCast(&peerAddr), &addrlen) < 0)
        LogError << "SocketAPI::getPeerAddr() errno: " << errno;
    return peerAddr;
}

void SocketAPI::close(int sockfd)
{
    if (::close(sockfd) < 0)
        LogError << "SocketAPI::close() errno: " << errno;
}

void SocketAPI::listenOrDie(int sockfd)
{
    int ret = ::listen(sockfd, SOMAXCONN);
    if (ret < 0)
        LogFatal << "SocketAPI::listenOrDie() errno: " << errno;
}

void SocketAPI::bindOrDie(int sockfd, const struct sockaddr* addr)
{
    int ret = ::bind(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
    if (ret < 0)
        LogFatal << "SocketAPI::bindOrDie() errno: " << errno;
}

int SocketAPI::accept(int sockfd, struct sockaddr_in6* addr)
{
    socklen_t addrlen = static_cast<socklen_t>(sizeof(*addr));
    int connfd = ::accept4(sockfd, sockAddrCast(addr), &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd < 0)
        LogError << "SocketAPI::accept() errno: " << errno;
    return connfd;
}


struct sockaddr* SocketAPI::sockAddrCast(struct sockaddr_in6* addr)
{
    return static_cast<struct sockaddr*>(static_cast<void*>(addr));   
}
const struct sockaddr* SocketAPI::sockAddrCast(const struct sockaddr_in6* addr)
{
    return static_cast<const struct sockaddr*>(static_cast<const void*>(addr));
}
const struct sockaddr_in6* SocketAPI::sockAddrIn6Cast(const struct sockaddr* addr)
{
    return static_cast<const struct sockaddr_in6*>(static_cast<const void*>(addr));
}
const struct sockaddr_in* SocketAPI::sockAddrInCast(const struct sockaddr* addr)
{
    return static_cast<const struct sockaddr_in*>(static_cast<const void*>(addr));
}

ssize_t SocketAPI::write(int fd, const char* data, size_t len)
{
    return ::write(fd, data, len);
}

int SocketAPI::getSocketError(int sockfd)
{
    int optval;
    socklen_t optlen = sizeof(optval);
    if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
        return errno;
    else 
        return optval;
}

ssize_t SocketAPI::readv(int sockfd, const struct iovec* iov, int iovcnt)
{
    return ::readv(sockfd, iov, iovcnt);
}

void SocketAPI::shutdownWrite(int sockfd)
{
    if (::shutdown(sockfd, SHUT_WR) < 0)
        LogError << "SocketAPI::shutdownWrite() errno" << errno;
}

int SocketAPI::connect(int sockfd, const struct sockaddr* addr)
{
    return ::connect(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
}

bool SocketAPI::isSelfConnect(int sockfd)
{
    struct sockaddr_in6 localaddr = getLocalAddr(sockfd);
    struct sockaddr_in6 peeraddr = getPeerAddr(sockfd);
    if (localaddr.sin6_family == AF_INET)
    {
        const struct sockaddr_in* laddr4 = reinterpret_cast<struct sockaddr_in*>(&localaddr);
        const struct sockaddr_in* raddr4 = reinterpret_cast<struct sockaddr_in*>(&peeraddr);
        return laddr4->sin_port == raddr4->sin_port
            && laddr4->sin_addr.s_addr == raddr4->sin_addr.s_addr;
    }
    else if (localaddr.sin6_family == AF_INET6)
    {
        return localaddr.sin6_port == peeraddr.sin6_port
            && memcmp(&localaddr.sin6_addr, &peeraddr.sin6_addr, sizeof localaddr.sin6_addr) == 0;
    }
    else
    {
        return false;
    }
}

void SocketAPI::toIp(char* buf, size_t size, const struct sockaddr* addr)
{
    if (addr->sa_family == AF_INET)
    {
        assert(size >= INET_ADDRSTRLEN);
        const struct sockaddr_in* addr4 = sockAddrInCast(addr);
        ::inet_ntop(AF_INET, &addr4->sin_addr, buf, static_cast<socklen_t>(size));
    }
    else if (addr->sa_family == AF_INET6)
    {
        assert(size >= INET6_ADDRSTRLEN);
        const struct sockaddr_in6* addr6 = sockAddrIn6Cast(addr);
        ::inet_ntop(AF_INET6, &addr6->sin6_addr, buf, static_cast<socklen_t>(size));
    }
}

void SocketAPI::toIpPort(char* buf, size_t size, const struct sockaddr* addr)
{
    toIp(buf, size, addr);
    size_t end = strlen(buf);
    const struct sockaddr_in* addr4 = sockAddrInCast(addr);
    uint16_t port = ntohs(addr4->sin_port);
    assert(size>end);
    snprintf(buf+end, size-end, ":%u", port);
}