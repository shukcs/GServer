#ifndef _SOCKETS_Lock_H
#define _SOCKETS_Lock_H

#include "stdconfig.h"
#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

class IMutex;
/** IMutex encapsulation class. 
	\ingroup threading */
class SHARED_DECL Lock
{
public:
	Lock(IMutex&);
	~Lock();
private:
	IMutex& m_mutex;
};


#ifdef SOCKETS_NAMESPACE
}
#endif
#endif // _SOCKETS_Lock_H

