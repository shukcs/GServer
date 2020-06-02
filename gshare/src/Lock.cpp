#include "IMutex.h"
#include "Lock.h"

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif


Lock::Lock(IMutex *m) : m_mutex(m)
{
    if(m_mutex)
	    m_mutex->Lock();
}


Lock::~Lock()
{
    if (m_mutex)
        m_mutex->Unlock();
}


#ifdef SOCKETS_NAMESPACE
}
#endif

