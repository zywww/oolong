#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace oolong
{
    class EventLoop;
    class EventLoopThread;
    class EventLoopThreadPool
    {
    public:
        using ThreadInitCallback = std::function<void(EventLoop*)>; // todo 和thread里面定义一样
        EventLoopThreadPool(EventLoop* baseLoop);
        ~EventLoopThreadPool();

        void setThreadNum(size_t num) { numThreads_ = num; }
        void start(const ThreadInitCallback& cb = ThreadInitCallback());

        EventLoop* getNextLoop();
        EventLoop* getLoopForHash(size_t hashCode);

private:
        EventLoop* baseLoop_;
        size_t numThreads_ = 0;
        size_t next_ = 0;
        std::vector<std::unique_ptr<EventLoopThread>> threads_;
        std::vector<EventLoop*> loops_;
    };
} // namespace oolong