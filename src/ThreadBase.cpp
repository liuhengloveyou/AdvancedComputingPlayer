#include "ThreadBase.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

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

    sleep(10);
    
#ifdef __EMSCRIPTEN_PTHREADS__
    pthread_create(&tid, NULL, ThreadEntry, this);
#else
    if (m_th)
    {
        return;
    }
    m_th = new std::thread(ThreadEntry, this);
#endif
}
