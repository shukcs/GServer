#include <stdio.h>
#ifdef _WIN32
#include <process.h>
#include "socket_include.h"
#else
#include <unistd.h>
#endif

#include "Thread.h"
#include "Utility.h"


#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

Thread::Thread(bool run, unsigned ms): m_thread(0)
, m_running(false), m_msSleep(ms)
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
    while (p && p->m_running)
    {
        if (!p->RunLoop())
            Utility::Sleep(p->m_msSleep);
    }

    if (p && p->IsDeleteOnExit())
        delete p;

#ifdef _WIN32
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
#if defined _WIN32 || defined _WIN64
unsigned Thread::GetThreadId()const
{
    return m_dwThreadId;
}
#endif

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
#ifdef _WIN32
        //m_thread = ::CreateThread(NULL, 0, StartThread, this, 0, &m_dwThreadId);
        m_thread = (HANDLE)_beginthreadex(NULL, 0, &StartThread, this, 0, &m_dwThreadId);
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

#ifdef SOCKETS_NAMESPACE
}
#endif


