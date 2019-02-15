#pragma once

#include <string>
#include <netinet/in.h>
#include <oolong/net/SocketAPI.h>

namespace oolong
{
    // todo 是不是内部都是用 in6 表示地址的?
    class EndPoint
    {
    public:
        EndPoint(const std::string& ip, uint16_t port, bool ipv6 = false);
        explicit EndPoint(uint16_t port = 0, bool ipv6 = false);
        EndPoint(const struct sockaddr_in& addr) :
            addr_(addr) {}
        EndPoint(const struct sockaddr_in6& addr6) :
            addr6_(addr6) {}

        const struct sockaddr* getSockAddr() const { return SocketAPI::sockAddrCast(&addr6_); };
        sa_family_t family() const { return addr_.sin_family; }
        void setSockAddrIn6(const struct sockaddr_in6 addr6) { addr6_ = addr6; }

        // std::string toString();
        std::string toIp() const;
        std::string toIpPort() const;

    private:
        union 
        {
            struct sockaddr_in addr_;
            struct sockaddr_in6 addr6_;
        };
    };
} // namespace oolong