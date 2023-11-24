#include "ThreadBase.h"
#include <pthread.h>
#include <stdio.h>

static void *ThreadEntry(void *arg)
{
    ThreadBase *th = (ThreadBase *)arg;
    th->run();
}

ThreadBase::ThreadBase()
{
}

void ThreadBase::stop()
{
    m_stop = true;

#ifdef __EMSCRIPTEN_PTHREADS__
    pthread_join(tid, NULL);
#else
    if (m_th)
    {
        m_th->join();
    }
#endif
}

void ThreadBase::start()
{
    m_stop = false;

#ifdef __EMSCRIPTEN_PTHREADS__
    pthread_create(&tid, NULL, ThreadEntry, this);
    printf(">>>>>>>>>>>>>%d\n", tid);
#else
    if (m_th)
    {
        return;
    }
    m_th = new std::thread(ThreadEntry, this);
#endif
}
