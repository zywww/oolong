#include <sys/uio.h>

#include <oolong/net/Buffer.h>
#include <oolong/net/SocketAPI.h>

using namespace oolong;

ssize_t Buffer::readFd(int fd)
{
    char extraBuffer[65536];
    const int iovcnt = writableBytes() < sizeof extraBuffer ? 2 : 1;
    struct iovec vec[2];
    const size_t writable = writableBytes();
    vec[0].iov_base = data_ + writerIndex_; // todo 比较一下和muduo两种风格优劣
    vec[0].iov_len = writable;
    if (iovcnt == 2)
    {
        vec[1].iov_base = extraBuffer;
        vec[1].iov_len = sizeof extraBuffer;
    }
    const ssize_t n = SocketAPI::readv(fd, vec, iovcnt);
    if (n < 0)
    {

    }
    else if (static_cast<size_t>(n) <= writable)
        writerIndex_ += n;
    else
    {
        writerIndex_ = size_;
        append(extraBuffer, n - writable);
    }
    return n;
}