#include <sys/timerfd.h>
#include <unistd.h>
#include <functional>
#include <cassert>
#include <algorithm>
#include <cstdint>
#include <utility>
#include <string.h>

#include <oolong/base/Logger.h>
#include <oolong/net/Timer.h>
#include <oolong/net/EventLoop.h>

using namespace oolong;

std::atomic<uint32_t> Timer::s_numCreated{0};

struct timespec howMuchTimeFromNow(Timestamp when)
{
    int64_t microseconds = when.microSecondsSinceEpoch() - Timestamp::now().microSecondsSinceEpoch();
    if (microseconds < 100)
        microseconds = 100;
    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(microseconds/Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>((microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
    return ts;
}

int createTimerfd()
{
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timerfd < 0)
        LogFatal << "Failed in timerfd_create";
    return timerfd;
}

void readTimerfd(int timerfd)
{
    uint64_t howmany;
    ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
    if (n != sizeof howmany)
        LogError << "TimerQueue::hadleRead() reads " << n << " bytes instead of 8";
}

void resetTimerfd(int timerfd, Timestamp expiration)
{
    struct itimerspec newValue; bzero(&newValue, sizeof newValue);
    struct itimerspec oldVaule; bzero(&oldVaule, sizeof oldVaule);
    newValue.it_value = howMuchTimeFromNow(expiration); // todo 差值必须大于0
    int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldVaule);
    if (ret) 
        LogFatal << "timerfd_settime()";
}

auto TimerCmp = [](Timer* lhs, Timer* rhs)
{
    if (lhs->expiration() == rhs->expiration())
        return lhs->getTimerId() < rhs->getTimerId();
    else 
        return lhs->expiration() < rhs->expiration();
};

TimerQueue::TimerQueue(EventLoop* loop) :
    loop_(loop),
    timerfd_(createTimerfd()),
    timerChannel_(loop, timerfd_),
    timers_(TimerCmp)
{
    timerChannel_.setReadableCallback(std::bind(&TimerQueue::handleRead, this));
    timerChannel_.enableReading();
}

TimerQueue::~TimerQueue()
{
    // todo
    // ::close(timerfd_);
}

TimerId TimerQueue::addTimer(TimerCallback cb, Timestamp time, double interval)
{
    Timer* timer = new Timer(std::move(cb), time, interval);
    TimerId id = timer->getTimerId();
    loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));
    return id;   
}

void TimerQueue::cancel(TimerId id)
{
    loop_->runInLoop(std::bind(&TimerQueue::cancelInLoop, this, id));
}

void TimerQueue::addTimerInLoop(Timer* timer)
{
    bool earlistChanged = insertTimer(timer);
    if (earlistChanged)
        resetTimerfd(timerfd_, timer->expiration());
}

void TimerQueue::cancelInLoop(TimerId id)
{
    if (doingTimerTask_)
    {
        toCancelTimers_.insert(id);
    }
    else
    {
        deleteTimer(id.timer_);
    }
}

bool TimerQueue::insertTimer(Timer* timer)
{
    bool earliestChanged;
    if (timers_.empty())
        earliestChanged = true;
    else 
        earliestChanged = timer->expiration() < (*timers_.begin())->expiration();
    timers_.insert(timer);
    return earliestChanged;
}

void TimerQueue::deleteTimer(Timer* timer)
{
    assert(timers_.erase(timer));
    delete timer;
}

void TimerQueue::handleRead()
{
    loop_->assertInLoopThread();
    readTimerfd(timerfd_);

    // 取消定时器
    for (TimerId id : toCancelTimers_)
        deleteTimer(id.timer_);
    toCancelTimers_.clear();

    // 提出过时定时器
    Timestamp now = Timestamp::now();
    std::vector<Timer*> expired = getExpired(now);

    // 完成定时器任务
    doingTimerTask_ = true;
    for (Timer* timer : expired)
        timer->run();
    doingTimerTask_ = false;

    // 重置定时器
    reset(expired, now);
}

std::vector<Timer*> TimerQueue::getExpired(Timestamp now)
{
    Timer timer([]{}, now, 0);
    auto end = timers_.lower_bound(&timer);
    std::vector<Timer*> expired{timers_.begin(), end};
    timers_.erase(timers_.begin(), end);
    return expired;
}

void TimerQueue::reset(const std::vector<Timer*>& expired, Timestamp now)
{
    // 遍历所有定时器,若是重复定时器则加入,否则删除
    for (Timer* timer : expired)
    {
        if (timer->repeat() && toCancelTimers_.find(timer->getTimerId()) == toCancelTimers_.end())
        {
            timer->restart(now);
            insertTimer(timer);
        }
        else 
        {
            delete timer;
        }
    }

    if (!timers_.empty())
    {
        resetTimerfd(timerfd_, (*timers_.begin())->expiration());
    }
}