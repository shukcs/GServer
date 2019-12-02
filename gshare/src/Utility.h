#ifndef _SOCKETS_Utility_H
#define _SOCKETS_Utility_H

#include "socket_include.h"
#include <map>
#include <list>
#include <string>
#include <type_traits>
#include <stdconfig.h>

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

#define TWIST_LEN     624

class SocketAddress;
template <typename T>
class TypeInfo
{
public:
    enum {
        isSpecialized = std::is_enum<T>::value, // don't require every enum to be marked manually
        isPointer = false,
        isIntegral = std::is_integral<T>::value,
        isComplex = !isIntegral && !std::is_enum<T>::value,
        isStatic = true,
        isRelocatable = std::is_enum<T>::value,
        isLarge = (sizeof(T) > sizeof(void*)),
        isDummy = false, //### Qt6: remove
        sizeOf = sizeof(T)
    };
};
/** Conversion utilities. 
	\ingroup util */
namespace Utility
{
    SHARED_DECL int FindString(const char *src, int len, const char *cnt, int cntLen = -1);
    SHARED_DECL std::list<std::string> SplitString(const std::string &str, const std::string &sp, bool bSkipEmpty=true);
    SHARED_DECL uint32_t Crc32(const char *src, int len);
    SHARED_DECL std::string base64(const char *str_in, int len);
    SHARED_DECL size_t base64d(const std::string &str_in, char *buf, size_t len);
    SHARED_DECL std::string l2string(long l);
    SHARED_DECL std::string bigint2string(int64_t l);
    SHARED_DECL std::string bigint2string(uint64_t l);
    SHARED_DECL int64_t str2int(const std::string &str, unsigned radix=10, bool *suc = NULL);
    SHARED_DECL uint64_t atoi64(const std::string& str);
    SHARED_DECL unsigned int hex2unsigned(const std::string& str);
    SHARED_DECL std::string rfc1738_encode(const std::string& src);
    SHARED_DECL std::string rfc1738_decode(const std::string& src);
    SHARED_DECL bool IsBigEndian(void);
    SHARED_DECL void toBigendian(int s, void *buff);
    SHARED_DECL int fromBigendian(const void *buff);
    SHARED_DECL int64_t usTimeTick();
    SHARED_DECL int64_t msTimeTick();
    SHARED_DECL long secTimeCount();

    //utf8 utf16 local8bit相互转换，linux Unicode为32位
    SHARED_DECL std::wstring Utf8ToUnicode(const std::string & str);
    SHARED_DECL std::string UnicodeToUtf8(const std::wstring &str);
    /** Utf8 decrypt, encrypt. */
    SHARED_DECL std::string FromUtf8(const std::string&);
    SHARED_DECL std::string ToUtf8(const std::string&);
    SHARED_DECL std::string Upper(const std::string&);
    SHARED_DECL std::string Lower(const std::string&);
    SHARED_DECL bool Compare(const std::string&, const std::string&, bool=true);

    SHARED_DECL int GzCompress(const char *data, unsigned n, char *zdata, unsigned);
    SHARED_DECL int GzDecompress(const char *zdata, unsigned nz, char *data, unsigned);
	/** Checks whether a string is a valid ipv4/ipv6 ip number. */
    SHARED_DECL bool isipv4(const std::string&);
	/** Checks whether a string is a valid ipv4/ipv6 ip number. */
    SHARED_DECL bool isipv6(const std::string&);

	/** Hostname to ip resolution ipv4, not asynchronous. */
    SHARED_DECL bool u2ip(const std::string&, ipaddr_t&);
    SHARED_DECL bool u2ip(const std::string&, struct sockaddr_in& sa, int ai_flags = 0);
#ifdef ENABLE_IPV6
#ifdef IPPROTO_IPV6
	/** Hostname to ip resolution ipv6, not asynchronous. */
    SHARED_DECL bool u2ip(const std::string&, struct in6_addr&);
#endif
#endif
	/** Reverse lookup of address to hostname */
    SHARED_DECL bool reverse(struct sockaddr *sa, socklen_t sa_len, std::string&, int flags = 0);
    SHARED_DECL bool reverse(struct sockaddr *sa, socklen_t sa_len, std::string& hostname, std::string& service, int flags = 0);

    SHARED_DECL bool u2service(const std::string& name, int& service, int ai_flags = 0);

	/** Convert binary ip address to string: ipv4. */
    SHARED_DECL void l2ip(const ipaddr_t,std::string& );
    SHARED_DECL void l2ip(const in_addr&,std::string& );
#ifdef ENABLE_IPV6
#ifdef IPPROTO_IPV6
	/** Convert binary ip address to string: ipv6. */
    SHARED_DECL void l2ip(const struct in6_addr&,std::string& ,bool mixed = false);

	/** ipv6 address compare. */
    SHARED_DECL int in6_addr_compare(in6_addr,in6_addr);
#endif
#endif
	/** ResolveLocal (hostname) - call once before calling any GetLocal method. */
    SHARED_DECL void ResolveLocal();
	/** Returns local hostname, ResolveLocal must be called once before using.
		\sa ResolveLocal */
    SHARED_DECL const std::string& GetLocalHostname();
	/** Returns local ip, ResolveLocal must be called once before using.
		\sa ResolveLocal */
    SHARED_DECL ipaddr_t GetLocalIP();
	/** Returns local ip number as string.
		\sa ResolveLocal */
    SHARED_DECL const std::string& GetLocalAddress();
#ifdef ENABLE_IPV6
#ifdef IPPROTO_IPV6
	/** Returns local ipv6 ip.
		\sa ResolveLocal */
    SHARED_DECL const struct in6_addr& GetLocalIP6();
	/** Returns local ipv6 address.
		\sa ResolveLocal */
    SHARED_DECL const std::string& GetLocalAddress6();
#endif
#endif
	/** Get environment variable */
    SHARED_DECL const std::string GetEnv(const std::string& name);
	/** Set environment variable.
		\param var Name of variable to set
		\param value Value */
    SHARED_DECL void SetEnv(const std::string& var,const std::string& value);
	/** Convert sockaddr struct to human readable string.
		\param sa Ptr to sockaddr struct */
    SHARED_DECL std::string Sa2String(struct sockaddr *sa);

    SHARED_DECL unsigned long ThreadID();
    SHARED_DECL std::string ToString(double d);
    /** File system stuff */
    SHARED_DECL std::string ModuleDirectory();
    SHARED_DECL std::string CurrentDirectory();
    SHARED_DECL bool ChangeDirectory(const std::string &to_dir);

	/** wait a specified number of ms */
    SHARED_DECL void Sleep(int ms);
}

#ifdef SOCKETS_NAMESPACE
}
#endif

#endif // _SOCKETS_Utility_H

