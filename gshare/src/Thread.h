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
typedef unsigned threadfunc_t;
#else
typedef void *threadfunc_t;
#endif

class SHARED_DECL Thread
{
public:
	Thread(const char *name=NULL, bool run=false, unsigned ms=5); //ms: sleep time(unit: ms)while nothing to do in loop
	virtual ~Thread();
#ifdef _WIN32
    HANDLE GetThread()const;
#else
    pthread_t GetThread()const;
#endif
    unsigned GetThreadId()const;
	bool IsRunning()const;  //thread is running
	void SetRunning(bool x=true);//x:true,start thread; false, stop thread;

	bool IsDeleteOnExit()const;
	void SetDeleteOnExit(bool x = true);
protected:
    virtual bool RunLoop() = 0;//
private:
    bool setThreadName(const char *name);
private:
    static threadfunc_t StartThread(Thread *p);
private:
    unsigned m_threadId = 0;
    bool m_running = false;
    unsigned m_msSleep = 5;
    bool m_bDeleteOnExit = true;
#ifdef _WIN32
    HANDLE m_thread;
#else
    pthread_t m_thread;
    pthread_attr_t m_attr;
#endif
    char m_name[32] = {0};
};

#ifdef SOCKETS_NAMESPACE
}
#endif

#endif // _SOCKETS_Thread_H

