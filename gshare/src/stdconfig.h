#ifndef _SOCKETS_CONFIG_H
#define _SOCKETS_CONFIG_H

/* Limits */
#define TCP_LINE_SIZE 8192

#if defined _WIN32 || defined _WIN64
#include "crtdbg.h"
#endif

#ifndef _RUN_DP
/* First undefine symbols if already defined. */
#undef HAVE_OPENSSL
#undef ENABLE_IPV6
#undef USE_SCTP
#undef NO_GETADDRINFO
#undef ENABLE_POOL
#undef ENABLE_SOCKS4
#undef ENABLE_RESOLVER
#undef ENABLE_RECONNECT
#undef ENABLE_DETACH
#undef ENABLE_EXCEPTIONS
#undef ENABLE_XML
#endif // _RUN_DP


/* OpenSSL support. */
//#define HAVE_OPENSSL

/* Ipv6 support. */
//#define ENABLE_IPV6

/* SCTP support. */
//#define USE_SCTP

/* Define NO_GETADDRINFO if your operating system does not support
   the "getaddrinfo" and "getnameinfo" function calls. */
#define NO_GETADDRINFO

/* Connection pool support. */
//#define ENABLE_POOL

/* Socks4 client support. */
//#define ENABLE_SOCKS4

/* Asynchronous resolver. */
//#define ENABLE_RESOLVER

/* Enable TCP reconnect on lost connection.
	Socket::OnReconnect
	Socket::OnDisconnect
*/
//#define ENABLE_RECONNECT

/* Enable socket thread detach functionality. */
#define ENABLE_DETACH

/* Enabled exceptions. */
#define ENABLE_EXCEPTIONS

/* XML classes. */
//#define ENABLE_XML


/* Resolver uses the detach function so either enable both or disable both. */
#ifndef ENABLE_DETACH
//#undef ENABLE_RESOLVER
#endif

#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#  define DECL_EXPORT __declspec(dllexport)
#  define DECL_IMPORT __declspec(dllimport)
#elif defined(__ARMCC__) || defined(__CC_ARM)
/* work-around for missing compiler intrinsics */
#if defined(__ANDROID__) || defined(ANDROID)
#  if defined(__linux__) || defined(__linux)
#    define DECL_EXPORT     __attribute__((visibility("default")))
#    define DECL_IMPORT     __attribute__((visibility("default")))
#  else
#    define DECL_EXPORT     __declspec(dllexport)
#    define DECL_IMPORT     __declspec(dllimport)
#  endif
#endif
#elif defined(VISIBILITY_AVAILABLE)
#  define DECL_EXPORT     __attribute__((visibility("default")))
#  define DECL_IMPORT     __attribute__((visibility("default")))
#elif defined __sharegloabal
#  define DECL_EXPORT __global
#else
#  define DECL_EXPORT
#  define DECL_IMPORT
#endif

#ifdef SHARE_LIBRARY
#  define SHARED_DECL  DECL_EXPORT
#else
#  define SHARED_DECL  DECL_IMPORT
#endif

#define ReleasePointer(p) {\
    delete p;\
    p = NULL;\
}

#if (defined _WIN32 || defined _WIN64) && !defined USINGORIGINNEW
#ifdef _DEBUG
#define DEBUG_CLIENTBLOCK new(_CLIENT_BLOCK, __FILE__, __LINE__)
#define new DEBUG_CLIENTBLOCK
#else
#define DEBUG_CLIENTBLOCK
#endif//_DEBUG
#endif //defined _WIN32 || defined _WIN64

#endif // _SOCKETS_CONFIG_H
