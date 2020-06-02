#include "Mutex.h"

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif


Mutex::Mutex()
#ifdef _DEBUG 
:m_cout(0)
#endif
{
#ifdef _WIN32
    InitializeCriticalSection(&m_mutex);
#else
	pthread_mutex_init(&m_mutex, NULL);
#endif
}


Mutex::~Mutex()
{
#ifdef _WIN32
    DeleteCriticalSection(&m_mutex);
#else
	pthread_mutex_destroy(&m_mutex);
#endif
}


void Mutex::Lock()
{
#ifdef _WIN32
    EnterCriticalSection(&m_mutex);
#else
	pthread_mutex_lock(&m_mutex);
#endif
#ifdef _DEBUG 
    m_cout++;
    if (m_cout > 1)
        printf("lock count %d\n", m_cout);
#endif
}


void Mutex::Unlock()
{
#ifdef _DEBUG 
    m_cout--;
#endif
#ifdef _WIN32
	::LeaveCriticalSection(&m_mutex);
#else
	pthread_mutex_unlock(&m_mutex);
#endif
}


#ifdef SOCKETS_NAMESPACE
}
#endif

