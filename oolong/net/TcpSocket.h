#pragma once
#include <oolong/net/EndPoint.h>

namespace oolong
{
    class TcpSocket //: noncopyable
    {
    public:
        explicit TcpSocket(int fd) :
            fd_(fd)
        {
        }

        ~TcpSocket();

        void listen();
        void bind(const EndPoint&);
        void setReusePort(bool on);
        void setReuseAddr(bool on);
        void setTcpNoDelay(bool on);
        int accept(EndPoint* peer);
        void shutdownWrite();

        int fd() const { return fd_; }

    private:
        int fd_;
    };
} // namespace oolong