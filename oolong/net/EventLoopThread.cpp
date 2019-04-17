// #include <unistd.h>
// #include <sys/syscall.h>

#include <oolong/base/Logger.h>
#include <oolong/net/EventLoopThread.h>
#include <oolong/net/EventLoop.h>

using namespace oolong;

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb) :
    threadInitCallback_(cb)
{
}

EventLoopThread::~EventLoopThread()
{
    if (loop_)
    {
        loop_->quit();
        thread_->join();
    }
}

EventLoop* EventLoopThread::startLoop()
{
    assert(thread_ == nullptr);
    thread_ = std::make_unique<std::thread>(std::bind(&EventLoopThread::threadFunc, this));
    std::unique_lock<std::mutex> lock(mutex_);
    cond_loop_ready_.wait(lock, [&]{ return loop_; });
    return loop_;
}

void EventLoopThread::threadFunc()
{
    EventLoop loop;
    // LogDebug << "pid: " << getpid() << ", tid: " << ::syscall(SYS_gettid);
    if (threadInitCallback_)
        threadInitCallback_(&loop);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_loop_ready_.notify_one();
    }

    loop.loop();
    std::lock_guard<std::mutex> lock(mutex_);
    loop_ = nullptr;
}