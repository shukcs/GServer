#include <stdio.h>
#if defined _WIN32 || defined _WIN64
#include <process.h>
#include "socket_include.h"
#else
#include <sys/types.h>
#include <unistd.h>
#endif

#include "Thread.h"
#include "Utility.h"


#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif

Thread::Thread(bool run, unsigned ms): m_threadId(0)
,m_thread(0), m_running(false), m_msSleep(ms)
, m_bDeleteOnExit(true)
{
    SetRunning(run);
}

Thread::~Thread()
{
    if (m_running)
    {
        SetRunning(false);
        Utility::Sleep(100);
    }
#ifdef _WIN32
    if (m_thread)
        ::CloseHandle(m_thread);
#else
	pthread_attr_destroy(&m_attr);
#endif
}

threadfunc_t STDPREFIX Thread::StartThread(threadparam_t zz)
{
    Thread *p = (Thread *)zz;
    if (!p)
        return NULL;
    p->m_threadId = Utility::ThreadID();
    while (p && p->IsRunning())
    {
        if (!p->RunLoop())
            Utility::Sleep(p->m_msSleep);
    }

    if (p && p->IsDeleteOnExit())
        delete p;

#if defined _WIN32 || defined _WIN64
	_endthreadex(0);
#endif
	return (threadfunc_t)NULL;
}

#if defined _WIN32 || defined _WIN64
HANDLE Thread::GetThread() const
#else
pthread_t Thread::GetThread() const
#endif
{
    return m_thread;
}

unsigned Thread::GetThreadId()const
{
    return m_threadId;
}

bool Thread::IsRunning()const
{
 	return m_running;
}

void Thread::SetRunning(bool x)
{
    if (m_running == x)
        return;

 	m_running = x;
    if (m_running)
    {
#if defined _WIN32 || defined _WIN64
        //m_thread = ::CreateThread(NULL, 0, StartThread, this, 0, &m_dwThreadId);
        m_thread = (HANDLE)_beginthreadex(NULL, 0, &StartThread, this, 0, (uint32_t*)&m_threadId);
#else
        pthread_attr_init(&m_attr);
        pthread_attr_setdetachstate(&m_attr, PTHREAD_CREATE_DETACHED);
        if (pthread_create(&m_thread, &m_attr, StartThread, this) == -1)
        {
            perror("Thread: create failed");
            SetRunning(false);
        }
#endif
    }
}

bool Thread::IsDeleteOnExit()const
{
	return m_bDeleteOnExit;
}

void Thread::SetDeleteOnExit(bool x)
{
    m_bDeleteOnExit = x;
}

void Thread::PostSim()
{
}

void Thread::WaitSim()
{
}