#ifndef _SOCKETS_Utility_H
#define _SOCKETS_Utility_H

#include <ctype.h>
#include <memory>
#include "socket_include.h"
#include <map>
#include <list>
#include <string>
#include <cstring>
#include <stdconfig.h>

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

#define TWIST_LEN     624

class SocketAddress;

/** Conversion utilities. 
	\ingroup util */
class Utility
{
private:
	class Rng {
	public:
		Rng(unsigned long seed);

		unsigned long Get();

	private:
		int m_value;
		unsigned long m_tmp[TWIST_LEN];
	};
	class ncmap_compare {
	public:
		bool operator()(const std::string& x, const std::string& y) const;
	};
public:
	template<typename Y> class ncmap : public std::map<std::string, Y, ncmap_compare> {
	public:
		ncmap() {}
	};	
public:
    SHARED_DECL static int FindString(const char *src, int len, const char *cnt, int cntLen = -1);
    SHARED_DECL static std::list<std::string> SplitString(const std::string &str, const std::string &sp, bool bSkipEmpty=true);
    SHARED_DECL static uint32_t Crc32(const char *src, int len);
    SHARED_DECL static std::string base64(const char *str_in, int len);
    SHARED_DECL static std::string base64d(const std::string& str_in);
    SHARED_DECL static std::string l2string(long l);
    SHARED_DECL static std::string bigint2string(int64_t l);
    SHARED_DECL static std::string bigint2string(uint64_t l);
    SHARED_DECL static int64_t str2int(const std::string &str, unsigned radix=10, bool *suc = NULL);
    SHARED_DECL static uint64_t atoi64(const std::string& str);
    SHARED_DECL static unsigned int hex2unsigned(const std::string& str);
    SHARED_DECL static std::string rfc1738_encode(const std::string& src);
    SHARED_DECL static std::string rfc1738_decode(const std::string& src);
    SHARED_DECL static bool IsBigEndian(void);
    SHARED_DECL static void toBigendian(int s, void *buff);
    SHARED_DECL static int fromBigendian(const void *buff);
    SHARED_DECL static int64_t msTimeTick();
    SHARED_DECL static int64_t secTimeCount();

    //utf8 utf16 local8bit相互转换，linux Unicode为32位
    SHARED_DECL static std::wstring Utf8ToUnicode(const std::string & str);
    SHARED_DECL static std::string UnicodeToUtf8(const std::wstring &str);
    /** Utf8 decrypt, encrypt. */
    SHARED_DECL static std::string FromUtf8(const std::string&);
    SHARED_DECL static std::string ToUtf8(const std::string&);

	/** Checks whether a string is a valid ipv4/ipv6 ip number. */
    SHARED_DECL static bool isipv4(const std::string&);
	/** Checks whether a string is a valid ipv4/ipv6 ip number. */
    SHARED_DECL static bool isipv6(const std::string&);

	/** Hostname to ip resolution ipv4, not asynchronous. */
    SHARED_DECL static bool u2ip(const std::string&, ipaddr_t&);
    SHARED_DECL static bool u2ip(const std::string&, struct sockaddr_in& sa, int ai_flags = 0);

#ifdef ENABLE_IPV6
#ifdef IPPROTO_IPV6
	/** Hostname to ip resolution ipv6, not asynchronous. */
    SHARED_DECL static bool u2ip(const std::string&, struct in6_addr&);
    SHARED_DECL static bool u2ip(const std::string&, struct sockaddr_in6& sa, int ai_flags = 0);
#endif
#endif
	/** Reverse lookup of address to hostname */
    SHARED_DECL static bool reverse(struct sockaddr *sa, socklen_t sa_len, std::string&, int flags = 0);
    SHARED_DECL static bool reverse(struct sockaddr *sa, socklen_t sa_len, std::string& hostname, std::string& service, int flags = 0);

    SHARED_DECL static bool u2service(const std::string& name, int& service, int ai_flags = 0);

	/** Convert binary ip address to string: ipv4. */
    SHARED_DECL static void l2ip(const ipaddr_t,std::string& );
    SHARED_DECL static void l2ip(const in_addr&,std::string& );
#ifdef ENABLE_IPV6
#ifdef IPPROTO_IPV6
	/** Convert binary ip address to string: ipv6. */
    SHARED_DECL static void l2ip(const struct in6_addr&,std::string& ,bool mixed = false);

	/** ipv6 address compare. */
    SHARED_DECL static int in6_addr_compare(in6_addr,in6_addr);
#endif
#endif
	/** ResolveLocal (hostname) - call once before calling any GetLocal method. */
    SHARED_DECL static void ResolveLocal();
	/** Returns local hostname, ResolveLocal must be called once before using.
		\sa ResolveLocal */
    SHARED_DECL static const std::string& GetLocalHostname();
	/** Returns local ip, ResolveLocal must be called once before using.
		\sa ResolveLocal */
    SHARED_DECL static ipaddr_t GetLocalIP();
	/** Returns local ip number as string.
		\sa ResolveLocal */
    SHARED_DECL static const std::string& GetLocalAddress();
#ifdef ENABLE_IPV6
#ifdef IPPROTO_IPV6
	/** Returns local ipv6 ip.
		\sa ResolveLocal */
    SHARED_DECL static const struct in6_addr& GetLocalIP6();
	/** Returns local ipv6 address.
		\sa ResolveLocal */
    SHARED_DECL static const std::string& GetLocalAddress6();
#endif
#endif
	/** Get environment variable */
    SHARED_DECL static const std::string GetEnv(const std::string& name);
	/** Set environment variable.
		\param var Name of variable to set
		\param value Value */
    SHARED_DECL static void SetEnv(const std::string& var,const std::string& value);
	/** Convert sockaddr struct to human readable string.
		\param sa Ptr to sockaddr struct */
    SHARED_DECL static std::string Sa2String(struct sockaddr *sa);

	/** Get current time in sec/microseconds. */
    SHARED_DECL static void GetTime(struct timeval *);
    SHARED_DECL static unsigned long ThreadID();

    SHARED_DECL static std::string ToLower(const std::string& str);
    SHARED_DECL static std::string ToUpper(const std::string& str);

    SHARED_DECL static std::string ToString(double d);

	/** Returns a random 32-bit integer */
    SHARED_DECL static unsigned long Rnd();

    /** File system stuff */
    SHARED_DECL static std::string ModuleDirectory();
    SHARED_DECL static std::string CurrentDirectory();
    SHARED_DECL static bool ChangeDirectory(const std::string &to_dir);

	/** wait a specified number of ms */
    SHARED_DECL static void Sleep(int ms);

private:
	static std::string m_host; ///< local hostname
	static ipaddr_t m_ip; ///< local ip address
	static std::string m_addr; ///< local ip address in string format
#ifdef ENABLE_IPV6
#ifdef IPPROTO_IPV6
	static struct in6_addr m_local_ip6; ///< local ipv6 address
#endif
	static std::string m_local_addr6; ///< local ipv6 address in string format
#endif
	static bool m_local_resolved; ///< ResolveLocal has been called if true
};


#ifdef SOCKETS_NAMESPACE
}
#endif

#endif // _SOCKETS_Utility_H

