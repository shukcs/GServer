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

#ifdef _WIN32
typedef HRESULT(*SetThreadDescFunc)(HANDLE hThread, PCWSTR lpThreadDescription);
static SetThreadDescFunc s_pfSetThreadDesc = NULL;
#else
typedef void* (*ThreadFunc)(void*);
#endif

#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif

Thread::Thread(const char *name, bool run, unsigned ms): m_threadId(0)
, m_running(false), m_msSleep(ms), m_bDeleteOnExit(true), m_thread(0)
{
    if (name)
        strncpy(m_name, name, sizeof(m_name));

    SetRunning(run);
}

Thread::~Thread()
{
    if (m_running)
    {
        SetRunning(false);
        Utility::Sleep(50);
    }
#ifdef _WIN32
    if (m_thread)
        ::CloseHandle(m_thread);
#else
	pthread_attr_destroy(&m_attr);
#endif
}

threadfunc_t Thread::StartThread(Thread *p)
{
    p->m_threadId = Utility::ThreadID();
    p->setThreadName(p->m_name);
    printf("%s: %s start!\n", __FUNCTION__, p->m_name);
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

void Thread::SetRunning(bool bRun)
{
    if (m_running == bRun)
        return;

    m_running = bRun;
    if (bRun)
    {
#if defined _WIN32 || defined _WIN64
        //m_thread = ::CreateThread(NULL, 0, StartThread, this, 0, &m_dwThreadId);
        m_thread = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&StartThread, this, 0, (uint32_t*)&m_threadId);
#else
        pthread_attr_init(&m_attr);
        pthread_attr_setdetachstate(&m_attr, PTHREAD_CREATE_DETACHED);
        if (pthread_create(&m_thread, &m_attr, (ThreadFunc)StartThread, this) == -1)
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

bool Thread::setThreadName(const char *name)
{
    if (!name || name[0]==0)
        return false;
#ifdef _WIN32
    ///.Kernel32函数SetThreadDescription(HANDLE, PCWSTR)修改线程名字
    if (s_pfSetThreadDesc == NULL)
    {
        HINSTANCE h = LoadLibraryA("Kernel32");
        s_pfSetThreadDesc = h ? (SetThreadDescFunc)GetProcAddress(h, "SetThreadDescription") : NULL;
    }
    if (s_pfSetThreadDesc != NULL)
        return s_pfSetThreadDesc(GetCurrentThread(), Utility::Utf8ToUnicode(name).c_str())==S_OK;

    return false;
#elif defined(__APPLE__)
    return 0 == pthread_setname_np(m_name);
#else
    return 0 == pthread_setname_np(pthread_self(), m_name);
#endif
}