#ifndef _SOCKETS_Thread_H
#define _SOCKETS_Thread_H

#include "socket_include.h"
#if !defined _WIN32 && !defined _WIN64
#include <pthread.h>
#endif

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

#ifdef _WIN32
// to be
//typedef DWORD  threadfunc_t;
//typedef LPVOID threadparam_t;
//#define STDPREFIX WINAPI
typedef unsigned threadfunc_t;
typedef void * threadparam_t;
#define STDPREFIX __stdcall
#else

typedef void * threadfunc_t;
typedef void * threadparam_t;
#define STDPREFIX
#endif

class SHARED_DECL Thread
{
public:
	Thread(bool run=false, unsigned ms=5); //ms: sleep time(unit: ms)while nothing to do in loop
	virtual ~Thread();
#ifdef _WIN32
    HANDLE GetThread()const;
    unsigned GetThreadId()const;
#else
    pthread_t GetThread()const;
#endif
	bool IsRunning()const;  //thread is running
	void SetRunning(bool x=true);//x:true,start thread; false, stop thread;

	bool IsDeleteOnExit()const;
	void SetDeleteOnExit(bool x = true);

    virtual void PostSim();  //sim increase, for syc
    virtual void WaitSim();  //sim decrease, for syc
protected:
    virtual bool RunLoop() = 0;//
private:
    static threadfunc_t STDPREFIX StartThread(threadparam_t);
private:
#ifdef _WIN32
    HANDLE m_thread;
    unsigned m_dwThreadId;
#else
    pthread_t m_thread;
    pthread_attr_t m_attr;
#endif
	bool m_running;
	unsigned m_msSleep;
    bool m_bDeleteOnExit;
};

#ifdef SOCKETS_NAMESPACE
}
#endif

#endif // _SOCKETS_Thread_H

