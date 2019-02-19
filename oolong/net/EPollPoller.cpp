#include <poll.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <cassert>
#include <string.h>
#include <errno.h>

#include <oolong/base/Logger.h>
#include <oolong/net/EventLoop.h>
#include <oolong/net/EPollPoller.h>
#include <oolong/net/Channel.h>

using namespace oolong;

int createEPoll()
{
    int epollfd = ::epoll_create1(EPOLL_CLOEXEC);
    if (epollfd < 0)
        LogFatal << "EPollPoller epoll_create()";
    return epollfd;
}

EPollPoller::EPollPoller(EventLoop* loop) :
    Poller(loop),
    epollfd_(createEPoll()),
    events_(kInitEventListSize)
{
}

EPollPoller::~EPollPoller()
{
    ::close(epollfd_);
}

Timestamp EPollPoller::poll(int timeoutMs, std::vector<Channel*>& channels)
{
    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), events_.size(), timeoutMs);
    Timestamp now(Timestamp::now());
    if (numEvents > 0)
    {
        LogDebug << numEvents << " events happened";
        for (int i = 0; i < numEvents; ++i)
        {
            Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
            channel->setRevents(events_[i].events);
            channels.push_back(channel);
        }
        if (static_cast<size_t>(numEvents) == events_.size())
            events_.resize(events_.size() * 2); // todo 管理缩小
    }
    else if (numEvents == 0)
        LogDebug << "poll wait nothing";
    else
    {
        LogError << "poll wait errno: " << errno;
    }
    return now;
}

void EPollPoller::updateChannel(Channel* channel)
{
    ownerLoop_->assertInLoopThread();
    if (channel->getAddToPoll())
    {
        if (channel->isNoEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            channel->setAddToPoll(false);
        }
        else 
            update(EPOLL_CTL_MOD, channel);
    }       
    else
    {
        channel->setAddToPoll(true);
        assert(!channel->isNoEvent());
        update(EPOLL_CTL_ADD, channel);
    } 
}

void EPollPoller::removeChannel(Channel* channel)
{
    ownerLoop_->assertInLoopThread();
    if (channel->getAddToPoll())
    {
        update(EPOLL_CTL_DEL, channel);
        channel->setAddToPoll(false); // todo 重复代码
    }
}

void EPollPoller::update(int operation, Channel* channel)
{
    struct epoll_event event;
    bzero(&event, sizeof event);
    event.events = channel->events();
    event.data.ptr = channel;
    if (::epoll_ctl(epollfd_, operation, channel->fd(), &event) < 0)
        LogError << "epoll_ctl op = " << operation << " fd = " << channel->fd() 
            << " errno: " << errno;
}