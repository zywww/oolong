#include <oolong/net/EventLoopThreadPool.h>
#include <oolong/net/EventLoopThread.h>
#include <oolong/net/EventLoop.h>

using namespace oolong;

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop) :
    baseLoop_(baseLoop)
{
}

EventLoopThreadPool::~EventLoopThreadPool()
{
}

void EventLoopThreadPool::start(const ThreadInitCallback& cb)
{
    baseLoop_->assertInLoopThread();
    for (int i = 0; i < numThreads_; ++i)
    {
        EventLoopThread* th = new EventLoopThread(cb);
        threads_.push_back(std::unique_ptr<EventLoopThread>(th));
        loops_.push_back(th->startLoop());
    }
    if (numThreads_ == 0 && cb)
        cb(baseLoop_);    
}

EventLoop* EventLoopThreadPool::getNextLoop()
{
    baseLoop_->assertInLoopThread();
    return getLoopForHash(next_++);
}

EventLoop* EventLoopThreadPool::getLoopForHash(size_t hashCode)
{
    baseLoop_->assertInLoopThread();
    return loops_.empty() ? baseLoop_ : loops_[hashCode%loops_.size()];
}