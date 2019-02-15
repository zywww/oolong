#pragma once
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <atomic>
#include <set>
#include <utility>
#include <cstdint>
#include <functional>

#include <oolong/base/noncopyable.h>
#include <oolong/base/Timestamp.h>
#include <oolong/net/Callbacks.h>
#include <oolong/net/Channel.h>

namespace oolong
{
    class Timer;
    class TimerId
    {
        friend struct TimerIdHash;
        friend class TimerQueue;
        friend bool operator==(const TimerId, const TimerId);
        friend bool operator<(const TimerId, const TimerId);
    public:
        TimerId(uint32_t seq, Timer* timer) : 
            sequence_(seq),
            timer_(timer)
            {}
    private:
        uint32_t sequence_;
        Timer* timer_;
    };

    bool inline operator==(const TimerId lhs, const TimerId rhs)
    {
        return lhs.sequence_ == rhs.sequence_;
    }
    bool inline operator<(const TimerId lhs, const TimerId rhs)
    {
        return lhs.sequence_ < rhs.sequence_;
    }

    struct TimerIdHash
    {
        std::size_t operator()(const TimerId& id) const
        {
            return std::hash<uint32_t>()(id.sequence_);
        }
    };

    class Timer : noncopyable
    {
    public:
        Timer(TimerCallback cb, Timestamp expiration, double interval) :
            callback_(std::move(cb)),
            expiration_(expiration),
            interval_(interval),
            id_(s_numCreated++)
        {
        }

        TimerId getTimerId() { return TimerId(id_, this); }
        Timestamp expiration() const { return expiration_; }
        void run() { callback_(); }
        bool repeat() const { return interval_ > 0.0; }
        void restart(Timestamp now) 
        {
            if (repeat())
            {
                now.addTime(interval_);
                expiration_ = now;
            }
            else 
                expiration_ = Timestamp::invalidTime();
        }

    private:
        TimerCallback callback_;
        Timestamp expiration_;
        const double interval_;
        const uint32_t id_;

        static std::atomic<uint32_t> s_numCreated;
    };

    class EventLoop;
    class TimerQueue : noncopyable
    {
    public:
        TimerQueue(EventLoop* loop);
        ~TimerQueue();
        
        TimerId addTimer(TimerCallback cb, Timestamp time, double interval); // 线程安全
        void cancel(TimerId id); // 线程安全

    private:
        using Entry = std::pair<Timestamp, Timer*>;
        
        void addTimerInLoop(Timer* timer);
        void cancelInLoop(TimerId id);

        void handleRead();
        void deleteTimer(Timer* timer);
        bool insertTimer(Timer* timer);
        std::vector<Timer*> getExpired(Timestamp now);
        void reset(const std::vector<Timer*>& expired, Timestamp now);
        

        EventLoop* loop_;
        const int timerfd_;
        Channel timerChannel_;
        bool doingTimerTask_ = false;

        std::unordered_set<TimerId, TimerIdHash> toCancelTimers_;
        std::set<Timer*, std::function<bool(Timer*, Timer*)>> timers_;
    };
} // namespace oolong