#ifndef _SOCKETS_Mutex_H
#define _SOCKETS_Mutex_H

#ifndef _WIN32
#include <pthread.h>
#else
#include "socket_include.h"
#endif
#include "IMutex.h"

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

class Mutex : public IMutex
{
public:
    SHARED_DECL Mutex();
    SHARED_DECL ~Mutex();

    SHARED_DECL virtual void Lock();
    SHARED_DECL virtual void Unlock();
private:
#ifdef _DEBUG 
    int m_cout;
#endif
#ifdef _WIN32
    CRITICAL_SECTION        m_mutex;
#else
	mutable pthread_mutex_t m_mutex;
#endif
};

#ifdef SOCKETS_NAMESPACE
}
#endif
#endif // _SOCKETS_Mutex_H

