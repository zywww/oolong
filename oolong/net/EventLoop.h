#pragma once 

#include <memory>
#include <functional>
#include <vector>
#include <mutex>
#include <atomic>
#include <cassert>

#include <oolong/base/Timestamp.h>
#include <oolong/base/noncopyable.h>
#include <oolong/net/Poller.h>
#include <oolong/net/Callbacks.h>
#include <oolong/net/Timer.h>

namespace oolong
{
    class Channel;

    class EventLoop : noncopyable
    {
    public:
        using TaskCallback = std::function<void()>;
        EventLoop();
        ~EventLoop();

        void loop(); // 必须在创建loop的线程调用
        void quit(); // 线程安全
        void runInLoop(TaskCallback task); // 线程安全
        void queueInLoop(TaskCallback task); // 线程安全


        void updateChannel(Channel* channel);
        void removeChannel(Channel* channel);


        TimerId runAt(Timestamp time, TimerCallback cb);
        TimerId runAfter(double delay, TimerCallback cb); // 延迟delay秒
        TimerId runEvery(double interval, TimerCallback cb); // 每隔interval秒
        void cancel(TimerId id);

        void assertInLoopThread() { assert(isInLoopThread()); }
        bool isInLoopThread() const;

    private:
        void wakeup(); // 唤醒阻塞在wait的事件循环,以继续执行任务
        void handleRead(); // 处理wakeupfd上的读事件
        void doTask();

        bool looping_ = false;
        std::atomic<bool> quit_{false};
        bool doingTask_ = false;
        const int threadTid_;

        std::unique_ptr<Poller> poller_;
        TimerQueue timerQueue_;

        int wakeupFd_;
        Channel wakeupChannel_;

        std::mutex mutex_;
        std::vector<TaskCallback> tasks_;
    };
}