#pragma once
#include <vector>
#include <oolong/base/Timestamp.h>

namespace oolong
{
    class Channel;
    class EventLoop;
    
    class Poller //:noncopyable
    {
    public:
        Poller(EventLoop* loop) : ownerLoop_(loop)
        {
        }
        virtual ~Poller() = default;

        virtual Timestamp poll(int timeoutMs, std::vector<Channel*>& channels) = 0;
        virtual void updateChannel(Channel* channel) = 0;
        virtual void removeChannel(Channel* channel) = 0; 
        
    protected:
        EventLoop* ownerLoop_;
    };
} // namespace oolong