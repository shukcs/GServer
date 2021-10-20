#ifndef __MAIL_CONFIG_H__
#define __MAIL_CONFIG_H__

#if defined _WIN32 || defined _WIN64
#include "crtdbg.h"
#endif

#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#  define MAIL_EXPORT __declspec(dllexport)
#  define MAIL_IMPORT __declspec(dllimport)
#else
#  define MAIL_EXPORT
#  define MAIL_IMPORT
#endif

#ifdef MAIL_SHARELIB
#  define SHARED_MAIL  MAIL_EXPORT
#else
#  define SHARED_MAIL  MAIL_IMPORT
#endif

#if (defined _WIN32 || defined _WIN64) && !defined USINGORIGINNEW 
#ifdef _DEBUG 
#define DEBUG_CLIENTBLOCK new(_CLIENT_BLOCK, __FILE__, __LINE__)
#define new DEBUG_CLIENTBLOCK
#else
#define DEBUG_CLIENTBLOCK
#endif//_DEBUG
#endif //defined _WIN32 || defined _WIN64

#endif // __MAIL_CONFIG_H__
