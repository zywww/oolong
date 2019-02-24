#pragma once
#include <functional>

#include <oolong/base/Timestamp.h>
#include <oolong/base/noncopyable.h>

namespace oolong
{
    class EventLoop;

    // 可选择性的把fd资源交由chnnel管理
    // 处理io事件,fd可能为socket,timerfd
    class Channel : noncopyable
    {
    public:
        using EventCallback = std::function<void()>;

        Channel(EventLoop* loop, int fd, bool managerResource = true);
        ~Channel();

        EventLoop* ownerLoop() const { return loop_; }

        void setReadableCallback(EventCallback cb) { readableCallback_ = std::move(cb);}
        void setWritableCallback(EventCallback cb) { writableCallback_ = std::move(cb);}
        void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb);}
        void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb);}

        // 必须在loopthread调用s
        void enableReading() { events_ |= kReadEvent; update(); }
        void disableReading() { events_ &= ~kReadEvent; update(); }
        void enableWriting() { events_ |= kWriteEvent; update(); }
        void disableWriting() { events_ &= ~kWriteEvent; update(); }
        void disableAll() { events_ = kNoneEvent; update(); }

        void setRevents(int revents) { revents_ = revents; } // for poller
        bool getAddToPoll() const { return addToPoll_; } // for poller
        void setAddToPoll(bool added) { addToPoll_ = added; } // for poller
        bool isNoEvent() const { return events_ == kNoneEvent; }
        int events() const { return events_; }
        int fd() const { return fd_; }
        bool isWriting() const { return events_ & kWriteEvent; }

        void handleEvent(Timestamp reciveTime); // for EventLoop

    private:
        static const int kNoneEvent;
        static const int kReadEvent;
        static const int kWriteEvent;

        // 必须在 loopthread 调用
        void update();

        EventLoop* loop_;
        const int fd_;
        bool managerResource_;
        int events_ = 0;
        int revents_ = 0; // todo 用uint32_t会不会更好
        bool addToPoll_ = false; // todo poll 实现相关的属性放在这里合适吗

        EventCallback readableCallback_;
        EventCallback writableCallback_;
        EventCallback closeCallback_;
        EventCallback errorCallback_;
    };
} // namespace oolong