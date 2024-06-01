#ifndef _SOCKETS_Thread_H
#define _SOCKETS_Thread_H

#include "stdconfig.h"
#if defined _WIN32 || defined _WIN64
#include <Windows.h>
#else
#include <pthread.h>
#endif
#include <queue>
#include <map>
#include <functional>

#ifdef COMMON_NAMESPACE
namespace COMMON_NAMESPACE {
#endif

#ifdef _WIN32
typedef unsigned threadfunc_t;
#else
typedef void *threadfunc_t;
#endif

namespace std {
    class mutex;
    class thread;
}
class Timer;
class Variant;
class GLoop;

class Thread
{
public:
    SHARED_DECL Thread(unsigned ms=5); //ms: sleep time(unit: ms)while nothing to do in loop
    SHARED_DECL virtual ~Thread();

    SHARED_DECL unsigned GetThreadId()const;
    SHARED_DECL bool IsRunning()const;  //thread is running
    SHARED_DECL void SetRunning(bool x=true);//x:true,start thread; false, stop thread;

    SHARED_DECL bool IsDeleteOnExit()const;
    SHARED_DECL void SetDeleteOnExit(bool x = true);
    SHARED_DECL void SetEnableTimer(bool b);
    SHARED_DECL bool IsEnableTimer()const;

    SHARED_DECL Timer *AddTimer(const Variant &id, uint32_t tmRepeat, bool repeat);
    SHARED_DECL void KillTimer(Timer *&timer);
    SHARED_DECL void SetTimerHandle(std::function<bool(const Variant &, int64_t)> hd);

    template< class _Fx,
        class... _Types >
    unsigned AddHandle(_Fx&& f, _Types..._args)
    {
        return addHandle(std::bind(f, _STD forward<_Types>(_args)...));
    }

    GLoop *GetLoop()const;
public:
    SHARED_DECL static bool ExecTimer(const std::function<bool()> &handle, uint32_t usIdle=5000);
protected:
    void runLoop();
    ///return 0, Binded fail
    SHARED_DECL unsigned addHandle(const std::function<bool()> &f);
private:
    unsigned m_msSleep;
    bool m_bDeleteOnExit;
    std::thread  *m_thread;
    GLoop *m_loop;
};

#ifdef COMMON_NAMESPACE
}
#endif

#endif // _SOCKETS_Thread_H

