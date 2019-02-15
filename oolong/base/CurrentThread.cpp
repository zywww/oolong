#include <sys/syscall.h>
#include <unistd.h>

#include <oolong/base/CurrentThread.h>

namespace oolong
{
    thread_local int t_threadTid = 0;

    int currentThreadTid()
    {
        if (t_threadTid == 0)
        {
            t_threadTid = ::syscall(SYS_gettid);
        }
        return t_threadTid;
    }
}