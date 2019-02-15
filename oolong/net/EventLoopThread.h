#pragma once
#include <functional>
#include <thread>
#include <memory>
#include <mutex>
#include <condition_variable>

namespace oolong
{
    class EventLoop;
    class EventLoopThread //:noncopyable
    {
    public:
        using ThreadInitCallback = std::function<void(EventLoop*)>;
        EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback());
        ~EventLoopThread();

        EventLoop* startLoop();

    private:
        void threadFunc();

        EventLoop* loop_ = nullptr;
        std::unique_ptr<std::thread> thread_;
        std::mutex mutex_;
        std::condition_variable cond_loop_ready_;
        ThreadInitCallback threadInitCallback_;
    };
} // namespace oolong