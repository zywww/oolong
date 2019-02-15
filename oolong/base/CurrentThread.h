#pragma once 

namespace oolong
{
    extern thread_local int t_threadTid;

    int currentThreadTid();
}