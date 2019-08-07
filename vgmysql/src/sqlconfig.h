#ifndef _STD_CONFIG_H
#define _STD_CONFIG_H

#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#  define SQL_EXPORT __declspec(dllexport)
#  define SQL_IMPORT __declspec(dllimport)
#else
#  define SQL_EXPORT
#  define SQL_IMPORT
#endif

#ifdef SHARE_LIBRARY
#  define SHARED_SQL  SQL_EXPORT
#else
#  define SHARED_SQL  SQL_IMPORT
#endif

#endif // _SOCKETS_CONFIG_H
