#include <poll.h>

#include <oolong/base/Logger.h>
#include <oolong/net/Channel.h>
#include <oolong/net/EventLoop.h>
#include <oolong/net/SocketAPI.h>

using namespace oolong;

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop* loop, int fd, bool managerResource) :
    loop_(loop),
    fd_(fd),
    managerResource_(managerResource)
{
}

Channel::~Channel()
{
    // todo 可能不在loop线程析构会有问题吗?
    if (addToPoll_)
        loop_->removeChannel(this);
    if (managerResource_)
        SocketAPI::close(fd_);
}

void Channel::handleEvent(Timestamp reciveTime)
{
    if ((revents_ & POLLHUP) && !(revents_ & POLLIN))
    {
        if (closeCallback_) closeCallback_();
    }

    if (revents_ & (POLLERR | POLLNVAL))
        if (errorCallback_) errorCallback_();
    if (revents_ & (POLLIN | POLLPRI | POLLRDHUP))
        if (readableCallback_) readableCallback_();
    if (revents_ & POLLOUT)
        if (writableCallback_) writableCallback_();
}

void Channel::update()
{
    loop_->updateChannel(this);
}