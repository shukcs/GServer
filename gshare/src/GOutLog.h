#ifndef _SOCKETS_StdoutLog_H
#define _SOCKETS_StdoutLog_H

#include "ILog.h"

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

class GOutLog : public ILog
{
public:
    GOutLog(){}
	void Log(const std::string &dsc, const std::string &obj, int evT = 0, int err = 0);
};


#ifdef SOCKETS_NAMESPACE
}
#endif

#endif // _SOCKETS_StdoutLog_H

