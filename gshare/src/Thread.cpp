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
#include "Varient.h"
#include "Lock.h"
#include <mutex>

#ifdef _WIN32
typedef HRESULT(*SetThreadDescFunc)(HANDLE hThread, PCWSTR lpThreadDescription);
static SetThreadDescFunc s_pfSetThreadDesc = NULL;
#else
typedef void* (*ThreadFunc)(void*);
#endif

#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif
using namespace std;
////////////////////////////////////////////////////////////////////////////////////////////
///ThreadManager
////////////////////////////////////////////////////////////////////////////////////////////
class ThreadManager
{
public:
    static ThreadManager &Instance()
    {
        static ThreadManager s_mgr;
        return s_mgr;
    }
    static Thread *GetThreadById(int id)
    {
        return Instance()._findThread(id);
    }
    void AddThread(Thread *t)
    {
        if (t)
        {
            Lock l(&m_mutex);
            m_threads[t->GetThreadId()] = t;
        }
    }
    void RemoveThread(Thread *t)
    {
        if (t)
        {
            Lock l(&m_mutex);
            m_threads.erase(t->GetThreadId());
        }
    }
protected:
    ThreadManager() {}
    Thread *_findThread(int id)
    {
        Lock l(&m_mutex);
        auto itr = m_threads.find(id);
        if (itr != m_threads.end())
            return itr->second;

        return NULL;
    }
private:
    std::mutex              m_mutex;
    std::map<int, Thread*>  m_threads;
};
///////////////////////////////////////////////////////////////////////////////////////////////
//Timer
///////////////////////////////////////////////////////////////////////////////////////////////
class Timer
{
public:
    Timer(const Variant &v, uint32_t ms, bool repeat = true) : m_usStart(Utility::usTimeTick())
	, m_id(v), m_threadID(Utility::ThreadID()), m_usSpace(ms*1000), m_count(1), m_offSet(0)
    , m_repeat(repeat), m_release(false)
    {
		m_usStart = Utility::usTimeTick();
        if (m_usSpace == 0)
			m_usSpace = 1000;
    }
	~Timer()
	{
	}
    int GetThreadID()const
    {
        return m_threadID;
    }
    void Kill()
    {
        m_release = true;
    }
    bool IsKilled()const
    {
        return m_release;
    }
    bool IsRepeat()const
    {
        return m_repeat;
    }
    void SetOffsetTime(uint32_t us)
    {
        m_offSet = us;
    }
    int64_t TriggerTime()const
    {
        return m_usStart+ m_usSpace*m_count+m_offSet;
    }
    void Trigger(int64_t curUs)
    {
		m_count = uint32_t((curUs - m_usStart) / m_usSpace + 1);
		m_offSet = 0;
    }
    const Variant &GetIdentify()const
    {
        return m_id;
    }
private:
    int64_t     m_usStart;
    Variant     m_id;
    int         m_threadID;
	uint32_t    m_usSpace;
	uint32_t    m_count;
	uint32_t    m_offSet;
	bool        m_release;
	bool        m_repeat;
};
//////////////////////////////////////////////////////////////////////////////////////
///Thread
//////////////////////////////////////////////////////////////////////////////////////
Thread::Thread(bool run, unsigned ms): m_threadId(0), m_running(false), m_msSleep(ms)
, m_bDeleteOnExit(true), m_enableTimer(false), m_mtxQue(new mutex), m_thread(0)
{
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
    ThreadManager::Instance().AddThread(p);
    while (p && p->IsRunning())
    {
        auto bSleep = p->PrcsTimer();
        if (!p->RunLoop() || bSleep)
            Utility::Sleep(p->m_msSleep);
    }
    ThreadManager::Instance().RemoveThread(p);

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

void Thread::SetEnableTimer(bool b)
{
    m_enableTimer = b;
}

bool Thread::IsEnableTimer()const
{
    return m_enableTimer;
}

Timer *Thread::AddTimer(const Variant &id, uint32_t tmRepeat, bool repeat)
{
    if (m_enableTimer && id.IsValid())
    {
        if (auto tm = new Timer(id, tmRepeat, repeat))
        {
            _pushTimer(tm);
            return tm;
        }
    }
    return NULL;
}

void Thread::KillTimer(Timer *&timer)
{
    timer->Kill();
    timer = NULL;
}

bool Thread::PrcsTimer()
{
    PrcsTimerQue();
    if (m_enableTimer)
        return PrcsTrigger();

    return false;
}

bool Thread::PrcsTrigger()
{
    auto us = Utility::usTimeTick();
    bool ret = false;
    for (auto itr = m_timersTrigger.begin(); itr!=m_timersTrigger.end(); )
    {
        if (us < itr->first)
            break;
        auto tm = itr->second;
        itr = m_timersTrigger.erase(itr);
        ret = true;
        if (tm->IsKilled())
        {
            _toRelease(tm);
            continue;
        }
		tm->Trigger(us);
        if (OnTimerTriggle(tm->GetIdentify(), us/1000) && tm->IsRepeat())
            _addTrigger(tm);
        else
            _toRelease(tm);
    }
    return true;
}

void Thread::PrcsTimerQue()
{
    while (auto tm = _popAddTimer())
    {
        if (tm->IsKilled())
            _toRelease(tm);
        else
            _addTrigger(tm);
    }
}

Timer *Thread::_popAddTimer()
{
    Timer *ret = NULL;
    Lock ll(m_mtxQue);
    Utility::PullQue(m_timersAdd, ret);
    return ret;
}

void Thread::_addTrigger(Timer *t)
{
    if (!t)
        return;

    int64_t tm = t->TriggerTime();
    auto itr = m_timersTrigger.upper_bound(t->TriggerTime());
    while (itr!=m_timersTrigger.end() && itr->first==tm)
    {
        tm++;
        itr++;
    }
    t->SetOffsetTime(uint32_t(tm - t->TriggerTime()));
    m_timersTrigger[tm] = t;
}

void Thread::_pushTimer(Timer *t)
{
    Lock ll(m_mtxQue);
    m_timersAdd.push(t);
}

void Thread::_toRelease(Timer *tm)
{
    if (!tm)
        return;
    auto t = ThreadManager::GetThreadById(tm->GetThreadID());
    if (!t || this ==t)
    {
        delete tm;
        return;
    }
    tm->Kill();
    t->_pushTimer(tm);
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