#ifndef _SOCKETS_Lock_H
#define _SOCKETS_Lock_H

#include "stdconfig.h"
#include <mutex>

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

class SHARED_DECL Lock
{
public:
	Lock(std::mutex *);
	~Lock();
private:
	std::mutex *m_mutex;
};


#ifdef SOCKETS_NAMESPACE
}
#endif
#endif // _SOCKETS_Lock_H

