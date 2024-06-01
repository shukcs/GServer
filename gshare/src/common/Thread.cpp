#include <stdio.h>
#if defined _WIN32 || defined _WIN64
#include <process.h>
#else
#include <sys/types.h>
#include <unistd.h>
#endif

#include "Thread.h"
#include <mutex>
#include "Utility.h"
#include "Varient.h"
#include "Lock.h"
#include "GLoop.h"

#ifdef _WIN32
typedef HRESULT(*SetThreadDescFunc)(HANDLE hThread, PCWSTR lpThreadDescription);
static SetThreadDescFunc s_pfSetThreadDesc = NULL;
#else
typedef void* (*ThreadFunc)(void*);
#endif

#ifdef COMMON_NAMESPACE
using namespace COMMON_NAMESPACE;
#endif
using namespace std;
//////////////////////////////////////////////////////////////////////////////////////
///Thread
//////////////////////////////////////////////////////////////////////////////////////
Thread::Thread(unsigned ms) : m_bDeleteOnExit(true), m_loop(new GLoop(ms*1000))
, m_thread(NULL)
{
}

Thread::~Thread()
{
    if (m_loop)
    {
        m_loop->Exit();
        Utility::Sleep(50);
    }
    delete m_thread;
}

unsigned Thread::GetThreadId()const
{
    if (m_loop)
        return m_loop->GetThreadId();

    return -1;
}

bool Thread::IsRunning()const
{
    if (m_loop)
        return m_loop->IsRun();

 	return false;
}

void Thread::SetRunning(bool bRun)
{
    bool bRunNow = m_loop && m_loop->IsRun();
    if (bRunNow == bRun)
        return;

    if (!bRunNow && !m_thread)
    {
        m_thread = new std::thread(&Thread::runLoop, this);
        m_thread->detach();
    }
    else if (bRunNow && m_loop)
    {
        m_loop->Exit();
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

void Thread::SetEnableTimer(bool b)
{
    if (m_loop)
        m_loop->SetEnableTimer(b);
}

bool Thread::IsEnableTimer()const
{
    return m_loop && m_loop->IsEnableTimer();
}

Timer *Thread::AddTimer(const Variant &id, uint32_t tmRepeat, bool repeat)
{
    if (m_loop)
        return m_loop->AddTimer(id, tmRepeat, repeat);

    return NULL;
}

void Thread::KillTimer(Timer *&timer)
{
    if (m_loop)
        m_loop->KillTimer(timer);
}


void Thread::SetTimerHandle(std::function<bool(const Variant &, int64_t)> hd)
{
    if (m_loop)
        m_loop->SetTimerHandle(hd);
}

GLoop *Thread::GetLoop() const
{
    return m_loop;
}

bool Thread::ExecTimer(const std::function<bool()> &handle, uint32_t usIdle)
{
    GLoop loop(usIdle);
    loop.AddLoopHandle(handle);
    loop.Run();
    return true;
}

void Thread::runLoop()
{
    if (m_loop)
        m_loop->Run();

    delete m_thread;
    m_thread = NULL;
    if (m_bDeleteOnExit)
        delete this;
}

unsigned Thread::addHandle(const std::function<bool()> &f)
{
    if (m_loop)
        return m_loop->AddLoopHandle(f);

    return 0;
}

//bool Thread::setThreadName(const char *name)
//{
//    if (!name || name[0]==0)
//        return false;
//#ifdef _WIN32
//    ///.Kernel32函数SetThreadDescription(HANDLE, PCWSTR)修改线程名字
//    bool ret = false;
//    if (s_pfSetThreadDesc == NULL)
//    {
//        HINSTANCE h = LoadLibraryA("Kernel32");
//        s_pfSetThreadDesc = h ? (SetThreadDescFunc)GetProcAddress(h, "SetThreadDescription") : NULL;
//    }
//    if (s_pfSetThreadDesc != NULL)
//        ret = s_pfSetThreadDesc(GetCurrentThread(), Utility::Utf8ToUnicode(name).c_str()) == S_OK;
//
//    return ret;
//#elif defined(__APPLE__)
//    return 0 == pthread_setname_np(m_name);
//#else
//    return 0 == pthread_setname_np(pthread_self(), m_name);
//#endif
//}