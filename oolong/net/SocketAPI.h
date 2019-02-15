#pragma once
#include <arpa/inet.h>


namespace oolong
{
    namespace SocketAPI
    {
        int createTcpSocket(sa_family_t family); // 创建 socket fd 并返回
        struct sockaddr_in6 getLocalAddr(int sockfd); // 获取socket本端地址
        struct sockaddr_in6 getPeerAddr(int sockfd); // 获取socket对端地址
        void close(int sockfd); // 关闭socket fd
        void listenOrDie(int sockfd); // 失败就挂掉 // todo这样合适吗?参考别的程序
        void bindOrDie(int sockfd, const struct sockaddr* addr);
        int accept(int sockfd, struct sockaddr_in6* addr);
        ssize_t write(int fd, const char* data, size_t len);
        int getSocketError(int sockfd);
        ssize_t readv(int sockfd, const struct iovec* iov, int iovcnt);
        void shutdownWrite(int sockfd);
        int connect(int sockfd, const struct sockaddr* addr);
        bool isSelfConnect(int sockfd);

        struct sockaddr* sockAddrCast(struct sockaddr_in6* addr);
        const struct sockaddr* sockAddrCast(const struct sockaddr_in6* addr);
        const struct sockaddr_in6* sockAddrIn6Cast(const struct sockaddr* addr);
        const struct sockaddr_in* sockAddrInCast(const struct sockaddr* addr);

        void toIp(char* buf, size_t size, const struct sockaddr* addr);
        void toIpPort(char* buf, size_t size, const struct sockaddr* addr);
    }
}