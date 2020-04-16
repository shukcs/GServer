#ifndef __LOGDB_H__
#define __LOGDB_H__

#include "ILog.h"

#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif

class LogDB : public ILog
{
public:
    static LogDB &Instance();
protected:
    void Log(const std::string &dsc, const std::string &obj, int evT = 0, int err = 0);
    void ProcessLog();
private:
    LogDB();
    ~LogDB();
};


#endif //__LOGDB_H__