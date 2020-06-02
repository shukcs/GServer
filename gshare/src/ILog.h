#ifndef _SOCKETS_ILog_H
#define _SOCKETS_ILog_H

#include "stdconfig.h"
#include <string>

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

/** error level enum. */
enum LogLevel
{
	LOG_LEVEL_INFO = 0,
	LOG_LEVEL_WARNING,
	LOG_LEVEL_ERROR,
	LOG_LEVEL_FATAL
};


class ISocketManager;
class ISocket;

/** \defgroup logging Log help classes */
/** Log class interface. 
	\ingroup logging */
class ILog
{
public:
	virtual ~ILog() {}
    virtual void Log(const std::string &dsc, const std::string &obj, int evT = 0, int err = 0) = 0;
};


#ifdef SOCKETS_NAMESPACE
}
#endif

#endif // _SOCKETS_StdLog_H

