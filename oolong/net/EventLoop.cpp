#include <cassert>
#include <unistd.h>
#include <sys/eventfd.h>

#include <oolong/base/Logger.h>
#include <oolong/base/CurrentThread.h>
#include <oolong/net/EventLoop.h>
#include <oolong/net/EPollPoller.h>


using namespace oolong;

namespace 
{
    thread_local EventLoop* t_loopInThisThread = nullptr;
}

int createEventfd()
{
    int eventfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (eventfd < 0)
        LogFatal << "fail in createEventfd()";
    return eventfd;
}


EventLoop::EventLoop() : 
    threadTid_(currentThreadTid()),
    poller_(new EPollPoller(this)),
    timerQueue_(this),
    wakeupFd_(createEventfd()),
    wakeupChannel_(this, wakeupFd_)
{
    if (t_loopInThisThread)
    {
        LogFatal << "another EventLoop exists in this thread" << threadTid_;
    }
    else 
    {
        t_loopInThisThread = this;
    }
    quit_ = false;
    wakeupChannel_.setReadableCallback(std::bind(&EventLoop::handleRead, this));
    wakeupChannel_.enableReading();
}

EventLoop::~EventLoop()
{
    // ::close(wakeupFd_);
}

bool EventLoop::isInLoopThread() const
{
    return threadTid_ == currentThreadTid();
}

void EventLoop::loop()
{
    assert(!looping_);
    assertInLoopThread();
    looping_ = true;
    while (!quit_)
    {
        std::vector<Channel*> activeChannels; // todo 试试作为成员会不会好点
        static const int kPollTimeMs = 10000;
        Timestamp pollReturnTime = poller_->poll(kPollTimeMs, activeChannels);
        for (Channel* channel : activeChannels)
            channel->handleEvent(pollReturnTime);
        doTask();
    }
    looping_ = false;
}

void EventLoop::quit()
{
    quit_ = true;
    if (!isInLoopThread())
        wakeup();
}

void EventLoop::runInLoop(TaskCallback task)
{
    if (isInLoopThread())
        task();
    else 
        queueInLoop(std::move(task));
    
}

void EventLoop::queueInLoop(TaskCallback task)
{
    std::lock_guard<std::mutex> lock(mutex_);
    tasks_.push_back(std::move(task));
    if (!isInLoopThread() || doingTask_)
        wakeup();
}

TimerId EventLoop::runAt(Timestamp time, TimerCallback cb)
{
    return timerQueue_.addTimer(std::move(cb), time, 0.0);
}

TimerId EventLoop::runAfter(double delay, TimerCallback cb)
{
    Timestamp time = Timestamp::now();
    time.addTime(delay);
    return timerQueue_.addTimer(std::move(cb), time, 0.0);
}

TimerId EventLoop::runEvery(double interval, TimerCallback cb)
{
    Timestamp time = Timestamp::now();
    time.addTime(interval);
    return timerQueue_.addTimer(std::move(cb), time, interval);
}

void EventLoop::cancel(TimerId id)
{
    timerQueue_.cancel(id);
}

void EventLoop::updateChannel(Channel* channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread(); // todo 有没有可能在非loop线程调用
    // LogDebug << "EevntLoop::updateChannel() fd: " << channel->fd();
    poller_->updateChannel(channel);
}


void EventLoop::removeChannel(Channel* channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread(); // todo 有没有可能在非loop线程调用
    // LogDebug << "EventLoop::removeChannel() fd: " << channel->fd();
    poller_->removeChannel(channel);
}

void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = ::write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
        LogError << "EventLoop::wakeup() writes " << n << " bytes";
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = ::read(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
        LogError << "EventLoop::handleRead() reads " << n << " bytes";
}

void EventLoop::doTask()
{
    doingTask_ = true;

    decltype(tasks_) tasks;
    {
    std::lock_guard<std::mutex> lock(mutex_);
    tasks.swap(tasks_);
    // LogDebug << "tasksize: " << tasks.size() << " task_size: " << tasks_.size();
    }

    for (const auto& task : tasks)
        task();

    doingTask_ = false;
}