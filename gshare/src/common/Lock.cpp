#include "Lock.h"

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

Lock::Lock(std::mutex *m) : m_mutex(m)
{
    if(m_mutex)
	    m_mutex->lock();
}

Lock::~Lock()
{
    if (m_mutex)
        m_mutex->unlock();
}

#ifdef SOCKETS_NAMESPACE
}
#endif

