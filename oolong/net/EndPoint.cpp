#include <string.h>
#include <arpa/inet.h>

#include <oolong/net/EndPoint.h>
#include <oolong/net/SocketAPI.h>

using namespace oolong;

EndPoint::EndPoint(const std::string& ip, uint16_t port, bool ipv6)
{
    // todo 重复代码
    if (ipv6)
    {
        bzero(&addr6_, sizeof addr6_);
        addr_.sin_family = AF_INET6;
        ::inet_pton(AF_INET6, ip.c_str(), &addr6_.sin6_addr); // todo 处理ip格式错误
        addr_.sin_port = htobe16(port);
    }
    else 
    {
        bzero(&addr_, sizeof addr_);
        addr_.sin_family = AF_INET;
        ::inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr);
        addr_.sin_port = htobe16(port);
    }
}

EndPoint::EndPoint(uint16_t port, bool ipv6)
{
    if (ipv6)
    {
        bzero(&addr6_, sizeof addr6_);
        addr6_.sin6_family = AF_INET6;
        addr6_.sin6_addr = in6addr_any;
        addr6_.sin6_port = htobe16(port);
    }
    else 
    {
        bzero(&addr_, sizeof addr_);
        addr_.sin_family = AF_INET;
        addr_.sin_addr.s_addr = INADDR_ANY;
        addr_.sin_port = htobe16(port);
    }
}

std::string EndPoint::toIp() const
{
    char buf[64];
    SocketAPI::toIp(buf, sizeof buf, getSockAddr());
    return buf;
}

std::string EndPoint::toIpPort() const
{
    char buf[64];
    SocketAPI::toIpPort(buf, sizeof buf, getSockAddr());
    return buf;
}