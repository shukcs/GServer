#ifndef _SOCKETS_Thread_H
#define _SOCKETS_Thread_H

#include "socket_include.h"
#if !defined _WIN32 && !defined _WIN64
#include <pthread.h>
#endif
#include <queue>
#include <map>

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

#ifdef _WIN32
typedef unsigned threadfunc_t;
#else
typedef void *threadfunc_t;
#endif

namespace std {
    class mutex;
}
class Timer;
class Variant;
class Thread
{
    typedef std::queue<Timer *> TimerQue;
    typedef std::map<int64_t, Timer *> TimerMap;
public:
    SHARED_DECL Thread(bool run=false, unsigned ms=5); //ms: sleep time(unit: ms)while nothing to do in loop
    SHARED_DECL virtual ~Thread();
#ifdef _WIN32
    SHARED_DECL HANDLE GetThread()const;
#else
    SHARED_DECL pthread_t GetThread()const;
#endif
    SHARED_DECL unsigned GetThreadId()const;
    SHARED_DECL bool IsRunning()const;  //thread is running
    SHARED_DECL void SetRunning(bool x=true);//x:true,start thread; false, stop thread;

    SHARED_DECL bool IsDeleteOnExit()const;
    SHARED_DECL void SetDeleteOnExit(bool x = true);
    SHARED_DECL void SetEnableTimer(bool b);
    SHARED_DECL bool IsEnableTimer()const;

    SHARED_DECL Timer *AddTimer(const Variant &id, uint32_t tmRepeat, bool repeat);
    SHARED_DECL void KillTimer(Timer *&timer);
protected:
    virtual bool OnTimerTriggle(const Variant &, int64_t) { return false; }//定时期处理，需要重载
    virtual bool RunLoop() = 0;//
    
    bool PrcsTimer();
    bool PrcsTrigger();
    void PrcsTimerQue();
private:
    void _pushTimer(Timer *t);
    Timer *_popAddTimer();
    void _addTrigger(Timer *t);
    void _toRelease(Timer *tm);
private:
    static threadfunc_t StartThread(Thread *p);
private:
    unsigned m_threadId;
    bool m_running;
    unsigned m_msSleep;
    bool m_bDeleteOnExit;
    std::mutex *m_mtxQue;
    bool m_enableTimer;
#ifdef _WIN32
    HANDLE m_thread;
#else
    pthread_t m_thread;
    pthread_attr_t m_attr;
#endif
    TimerQue    m_timersAdd;
    TimerMap    m_timersTrigger;
};

#ifdef SOCKETS_NAMESPACE
}
#endif

#endif // _SOCKETS_Thread_H

