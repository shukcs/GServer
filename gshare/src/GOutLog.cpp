#ifdef _MSC_VER
#pragma warning(disable:4786)
#include <time.h>
#endif
#include "GOutLog.h"
#include <stdio.h>
#include "socket_include.h"

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

void GOutLog::Log(const std::string &dsc, const std::string &obj, int evT, int err)
{
    char dt[48];
    std::string level("");
    switch (evT)
    {
    case LOG_LEVEL_INFO:
        level = "Event";
        break;
    case LOG_LEVEL_WARNING:
        level = "Warning";
        break;
    case LOG_LEVEL_ERROR:
        level = "Error";
        break;
    case LOG_LEVEL_FATAL:
        level = "Fatal";
        break;
    }
    time_t t = time(NULL);
    struct tm tp;
#if defined( _WIN32) && !defined(__CYGWIN__)
    localtime_s(&tp, &t);
#else
    localtime_r(&t, &tp);
#endif
    sprintf(dt, "%s(%d-%02d-%02d %02d:%02d:%02d)", level.c_str(),
        tp.tm_year + 1900, tp.tm_mon + 1,tp.tm_mday,
        tp.tm_hour, tp.tm_min, tp.tm_sec );
    if (err)
        printf("%s%s: %s; %s!\n", dt, obj.c_str(), dsc.c_str(), StrError(err));
    else
        printf("%s%s: %s!\n", dt, obj.c_str(), dsc.c_str());
}

#ifdef SOCKETS_NAMESPACE
}
#endif