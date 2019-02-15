#include <sys/timerfd.h>
#include <string.h>
#include <functional>
#include <iostream>

#include <oolong/base/Logger.h>
#include <oolong/net/Channel.h>
#include <oolong/net/EventLoop.h>

using namespace oolong;
using namespace std;

void readTimerfd(int timerfd);

class PeriodicTimer
{
public:
    PeriodicTimer(EventLoop* loop, double interval, TimerCallback cb) :
        loop_(loop), 
        interval_(interval),
        cb_(cb),
        timerfd_(::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC)),
        timerChannel_(loop, timerfd_)
    {
        assert(timerfd_ > 0);
        LogDebug << "PeriodicTimer timerfd: " << timerfd_;
        timerChannel_.setReadableCallback(std::bind(&PeriodicTimer::handleRead, this));
        timerChannel_.enableReading();
    }

    void start()
    {
        struct itimerspec spec;
        bzero(&spec, sizeof spec);
        spec.it_interval = toTimeSpec(interval_);
        spec.it_value = spec.it_interval;
        int ret = ::timerfd_settime(timerfd_, 0, &spec, nullptr);
        if (ret < 0)
            LogError << "timerfd_settime errno: " << errno;
        else 
            LogDebug << "timerfd settime success";
    }

private:

    void handleRead() 
    {
        LogDebug << "PeriodicTimer::handleRead";
        readTimerfd(timerfd_);
        if (cb_) cb_();
    }

      static struct timespec toTimeSpec(double seconds)
  {
    struct timespec ts;
    bzero(&ts, sizeof ts);
    const int64_t kNanoSecondsPerSecond = 1000000000;
    const int kMinInterval = 100000;
    int64_t nanoseconds = static_cast<int64_t>(seconds * kNanoSecondsPerSecond);
    if (nanoseconds < kMinInterval)
      nanoseconds = kMinInterval;
    ts.tv_sec = static_cast<time_t>(nanoseconds / kNanoSecondsPerSecond);
    ts.tv_nsec = static_cast<long>(nanoseconds % kNanoSecondsPerSecond);
    return ts;
  }

    EventLoop* loop_;
    const double interval_;
    TimerCallback cb_;
    int timerfd_;
    Channel timerChannel_;
};

int main()
{
    EventLoop loop;
    PeriodicTimer timer(&loop, 1, [](){ cout << "periodic timer 1 sec" << endl; });
    timer.start();
    loop.runEvery(1, std::bind(printf, "run every 1 sec"));
    loop.loop();
    return 0;
}