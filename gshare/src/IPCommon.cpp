#include "IPCommon.h"
#include "Ipv4Address.h"
#include "Ipv6Address.h"
#if defined _WIN32 || defined _WIN64
#else
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
// --- stack
#include <cxxabi.h>
#include <execinfo.h>
#include <dlfcn.h>
#include <execinfo.h>  
#endif

#include "Utility.h"

#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif
using namespace std;
// statics
static string s_hostName;
static bool s_local_resolved = false;
static ipaddr_t s_ip = 0;
static string s_address;
#ifdef ENABLE_IPV6
#ifdef IPPROTO_IPV6
static struct in6_addr s_local_ip6;
static string s_local_addr6;
#endif
#endif
static uint32_t str2ipv4(const string &ip)
{
    union {
        struct {
            unsigned char b[4];
        };
        ipaddr_t l;
    } u;
    int i = 0;
    for (const string &itr : Utility::SplitString(ip, "."))
    {
        u.b[i++] = (unsigned char)Utility::str2int(itr);
    }
    return u.l;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//namespace IPCommon
////////////////////////////////////////////////////////////////////////////////////////////////////
bool IPCommon::isipv4(const string& str)
{
	int dots = 0;
	// %! ignore :port?
	for (size_t i = 0; i < str.size(); ++i)
	{
		if (str[i] == '.')
			dots++;
		else
		if (!isdigit(str[i]))
			return false;
	}
	if (dots != 3)
		return false;
	return true;
}

bool IPCommon::isipv6(const string& str)
{
    auto ls = Utility::SplitString(str, ":");
    if (ls.size() != 7 && ls.size() != 8)
        return false;
    if (ls.size() == 7)
    {
        const string &ipv4 = ls.back();
        if (!isipv4(ipv4))
            return false;
        ls.pop_back();
    }
    bool bSuc;
    for (const string &itr : ls)
    {
        uint64_t res = Utility::str2int(itr, 16, &bSuc);
        if (!bSuc || res>0xffff)
            return false;
    }

    return true;
}

bool IPCommon::u2ip(const string& str, ipaddr_t& l)
{
	struct sockaddr_in sa;
	bool r = IPCommon::u2ip(str, sa);
	memcpy(&l, &sa.sin_addr, sizeof(l));
	return r;
}


#ifdef ENABLE_IPV6
#ifdef IPPROTO_IPV6
bool IPCommon::u2ip(const string& str, struct in6_addr& l)
{
	struct sockaddr_in6 sa;
	bool r = IPCommon::u2ip(str, sa);
	l = sa.sin6_addr;
	return r;
}
#endif
#endif

void IPCommon::l2ip(const ipaddr_t ip, string& str)
{
	struct sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	memcpy(&sa.sin_addr, &ip, sizeof(sa.sin_addr));
	IPCommon::reverse( (struct sockaddr *)&sa, sizeof(sa), str, NI_NUMERICHOST);
}

void IPCommon::l2ip(const in_addr& ip, string& str)
{
	struct sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_addr = ip;
	IPCommon::reverse( (struct sockaddr *)&sa, sizeof(sa), str, NI_NUMERICHOST);
}


#ifdef ENABLE_IPV6
#ifdef IPPROTO_IPV6
void IPCommon::l2ip(const struct in6_addr& ip, string& str,bool mixed)
{
	char slask[100]; // l2ip temporary
	*slask = 0;
	unsigned int prev = 0;
	bool skipped = false;
	bool ok_to_skip = true;
	if (mixed)
	{
		unsigned short x;
		unsigned short addr16[8];
		memcpy(addr16, &ip, sizeof(addr16));
		for (size_t i = 0; i < 6; ++i)
		{
			x = ntohs(addr16[i]);
			if (*slask && (x || !ok_to_skip || prev))
			{
#if defined( _WIN32) && !defined(__CYGWIN__)
				strcat_s(slask,sizeof(slask),":");
#else
				strcat(slask,":");
#endif
			}
			if (x || !ok_to_skip)
			{
				snprintf(slask + strlen(slask),sizeof(slask) - strlen(slask),"%x", x);
				if (x && skipped)
					ok_to_skip = false;
			}
			else
			{
				skipped = true;
			}
			prev = x;
		}
		x = ntohs(addr16[6]);
		snprintf(slask + strlen(slask),sizeof(slask) - strlen(slask),":%u.%u",x / 256,x & 255);
		x = ntohs(addr16[7]);
		snprintf(slask + strlen(slask),sizeof(slask) - strlen(slask),".%u.%u",x / 256,x & 255);
	}
	else
	{
		struct sockaddr_in6 sa;
		memset(&sa, 0, sizeof(sa));
		sa.sin6_family = AF_INET6;
		sa.sin6_addr = ip;
		IPCommon::reverse( (struct sockaddr *)&sa, sizeof(sa), str, NI_NUMERICHOST);
		return;
	}
	str = slask;
}

int IPCommon::in6_addr_compare(in6_addr a,in6_addr b)
{
	for (size_t i = 0; i < 16; ++i)
	{
		if (a.s6_addr[i] < b.s6_addr[i])
			return -1;
		if (a.s6_addr[i] > b.s6_addr[i])
			return 1;
	}
	return 0;
}
#endif
#endif

void IPCommon::ResolveLocal()
{
	char h[256];

	// get local hostname and translate into ip-address
	*h = 0;
	gethostname(h,255);

	if (u2ip(h, s_ip))
		l2ip(s_ip, s_address);
#ifdef ENABLE_IPV6
#ifdef IPPROTO_IPV6
	memset(&s_local_ip6, 0, sizeof(s_local_ip6));
    if (u2ip(h, s_local_ip6))
        l2ip(s_local_ip6, s_local_addr6);
#endif
#endif
	s_hostName = h;
	s_local_resolved = true;
}

const string& IPCommon::GetLocalHostname()
{
	if (!s_local_resolved)
		ResolveLocal();

	return s_hostName;
}

ipaddr_t IPCommon::GetLocalIP()
{
	if (!s_local_resolved)
		ResolveLocal();

	return s_ip;
}

const string& IPCommon::GetLocalAddress()
{
	if (!s_local_resolved)
		ResolveLocal();

	return s_address;
}

#ifdef ENABLE_IPV6
#ifdef IPPROTO_IPV6
const struct in6_addr& IPCommon::GetLocalIP6()
{
	if (!s_local_resolved)
		ResolveLocal();

	return s_local_ip6;
}

const string& IPCommon::GetLocalAddress6()
{
	if (!s_local_resolved)
		ResolveLocal();

	return s_local_addr6;
}
#endif
#endif

string IPCommon::Sa2String(struct sockaddr *sa)
{
#ifdef ENABLE_IPV6
#ifdef IPPROTO_IPV6
	if (sa -> sa_family == AF_INET6)
	{
		struct sockaddr_in6 *sa6 = (struct sockaddr_in6 *)sa;
		string tmp;
		IPCommon::l2ip(sa6 -> sin6_addr, tmp);
		return tmp + ":" + IPCommon::l2string(ntohs(sa6 -> sin6_port));
	}
#endif
#endif
	if (sa -> sa_family == AF_INET)
	{
		struct sockaddr_in *sa4 = (struct sockaddr_in *)sa;
		ipaddr_t a;
		memcpy(&a, &sa4 -> sin_addr, 4);
		string tmp;
		IPCommon::l2ip(a, tmp);
		return tmp + ":" + Utility::l2string(ntohs(sa4 -> sin_port));
	}
	return "";
}

bool IPCommon::u2ip(const string& host, struct sockaddr_in& sa, int ai_flags)
{
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
#ifdef NO_GETADDRINFO
    if ((ai_flags & AI_NUMERICHOST) != 0 || isipv4(host))
    {
        uint32_t ip = str2ipv4(host);
        memcpy(&sa.sin_addr, &ip, sizeof(sa.sin_addr));
        return true;
    }
#ifndef LINUX
    struct hostent *he = gethostbyname(host.c_str());
    if (!he)
        return false;

    memcpy(&sa.sin_addr, he->h_addr, sizeof(sa.sin_addr));
#else
    struct hostent he;
    struct hostent *result = NULL;
    int myerrno = 0;
    char buf[2000];
    int n = gethostbyname_r(host.c_str(), &he, buf, sizeof(buf), &result, &myerrno);
    if (n || !result)
        return false;

    if (he.h_addr_list && he.h_addr_list[0])
        memcpy(&sa.sin_addr, he.h_addr, 4);
    else
        return false;
#endif
    return true;
#else
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = ai_flags;
    hints.ai_family = AF_INET;
    hints.ai_socktype = 0;
    hints.ai_protocol = 0;
    struct addrinfo *res;
    if (IPCommon::isipv4(host))
        hints.ai_flags |= AI_NUMERICHOST;
    int n = getaddrinfo(host.c_str(), NULL, &hints, &res);
    if (!n)
    {
        std::vector<struct addrinfo *> vec;
        struct addrinfo *ai = res;
        while (ai)
        {
            if (ai->ai_addrlen == sizeof(sa))
                vec.push_back(ai);
            ai = ai->ai_next;
        }
        if (!vec.size())
            return false;
        ai = vec[IPCommon::Rnd() % vec.size()];
        memcpy(&sa, ai->ai_addr, ai->ai_addrlen);

        freeaddrinfo(res);
        return true;
    }
    std::string error = "Error: ";
#ifndef __CYGWIN__
    error += gai_strerror(n);
#endif
    return false;
#endif // NO_GETADDRINFO
}

#ifdef ENABLE_IPV6
#ifdef IPPROTO_IPV6
bool IPCommon::u2ip(const string& host, struct sockaddr_in6& sa, int ai_flags)
{
	memset(&sa, 0, sizeof(sa));
	sa.sin6_family = AF_INET6;
#ifdef NO_GETADDRINFO
	if ((ai_flags & AI_NUMERICHOST) != 0 || isipv6(host))
	{
		StringList vec = SplitString(host, ":");
		
        if (vec.size() == 7)
        {
            const string &ipv4 = vec.back();
            uint32_t ip = str2ipv4(ipv4);
            char slask[6];
            snprintf(slask, sizeof(slask), "%lx", ip>>16);
            vec.push_back(slask);
            snprintf(slask, sizeof(slask), "%lx", ip&0xffff);
            vec.push_back(slask);
        }
		size_t sz = vec.size(); // number of byte pairs
		size_t i = 0; // index in in6_addr.in6_u.u6_addr16[] ( 0 .. 7 )
		unsigned short addr16[8];
		for (list<string>::iterator it = vec.begin(); it != vec.end(); ++it)
		{
			string bytepair = *it;
			if (bytepair.size())
			{
				addr16[i++] = htons(IPCommon::hex2unsigned(bytepair));
			}
			else
			{
				addr16[i++] = 0;
				while (sz++ < 8)
				{
					addr16[i++] = 0;
				}
			}
		}
		memcpy(&sa.sin6_addr, addr16, sizeof(addr16));
		return true;
	}
#ifdef SOLARIS
	int errnum = 0;
	struct hostent *he = getipnodebyname( host.c_str(), AF_INET6, 0, &errnum );
#else
	struct hostent *he = gethostbyname2( host.c_str(), AF_INET6 );
#endif
	if (!he)
	{
		return false;
	}
	memcpy(&sa.sin6_addr,he -> h_addr_list[0],he -> h_length);
#ifdef SOLARIS
	free(he);
#endif
	return true;
#else
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_flags = ai_flags;
	hints.ai_family = AF_INET6;
	hints.ai_socktype = 0;
	hints.ai_protocol = 0;
	struct addrinfo *res;
	if (IPCommon::isipv6(host))
		hints.ai_flags |= AI_NUMERICHOST;
	int n = getaddrinfo(host.c_str(), NULL, &hints, &res);
	if (!n)
	{
		vector<struct addrinfo *> vec;
		struct addrinfo *ai = res;
		while (ai)
		{
			if (ai -> ai_addrlen == sizeof(sa))
				vec.push_back( ai );
			ai = ai -> ai_next;
		}
		if (!vec.size())
			return false;
		ai = vec[IPCommon::Rnd() % vec.size()];
		{
			memcpy(&sa, ai -> ai_addr, ai -> ai_addrlen);
		}
		freeaddrinfo(res);
		return true;
	}
	string error = "Error: ";
#ifndef __CYGWIN__
	error += gai_strerror(n);
#endif
	return false;
#endif // NO_GETADDRINFO
}
#endif // IPPROTO_IPV6
#endif // ENABLE_IPV6

bool IPCommon::reverse(struct sockaddr *sa, socklen_t sa_len, string& hostname, int flags)
{
	string service;
	return reverse(sa, sa_len, hostname, service, flags);
}

bool IPCommon::reverse(struct sockaddr *sa, socklen_t sa_len, string& hostname, string& service, int flags)
{
	hostname = "";
	service = "";
#ifdef NO_GETADDRINFO
	switch (sa -> sa_family)
	{
	case AF_INET:
		if (flags & NI_NUMERICHOST)
		{
			union {
				struct {
					unsigned char b1;
					unsigned char b2;
					unsigned char b3;
					unsigned char b4;
				} a;
				ipaddr_t l;
			} u;
			struct sockaddr_in *sa_in = (struct sockaddr_in *)sa;
			memcpy(&u.l, &sa_in -> sin_addr, sizeof(u.l));
			char tmp[100];
			snprintf(tmp, sizeof(tmp), "%u.%u.%u.%u", u.a.b1, u.a.b2, u.a.b3, u.a.b4);
			hostname = tmp;
			return true;
		}
		else
		{
			struct sockaddr_in *sa_in = (struct sockaddr_in *)sa;
			struct hostent *h = gethostbyaddr( (const char *)&sa_in -> sin_addr, sizeof(sa_in -> sin_addr), AF_INET);
			if (h)
			{
				hostname = h -> h_name;
				return true;
			}
		}
		break;
#ifdef ENABLE_IPV6
	case AF_INET6:
		if (flags & NI_NUMERICHOST)
		{
			char slask[100]; // l2ip temporary
			*slask = 0;
			unsigned int prev = 0;
			bool skipped = false;
			bool ok_to_skip = true;
			{
				unsigned short addr16[8];
				struct sockaddr_in6 *sa_in6 = (struct sockaddr_in6 *)sa;
				memcpy(addr16, &sa_in6 -> sin6_addr, sizeof(addr16));
				for (size_t i = 0; i < 8; ++i)
				{
					unsigned short x = ntohs(addr16[i]);
					if (*slask && (x || !ok_to_skip || prev))
					{
#if defined( _WIN32) && !defined(__CYGWIN__)
						strcat_s(slask, sizeof(slask),":");
#else
						strcat(slask,":");
#endif
					}
					if (x || !ok_to_skip)
					{
						snprintf(slask + strlen(slask), sizeof(slask) - strlen(slask),"%x", x);
						if (x && skipped)
							ok_to_skip = false;
					}
					else
					{
						skipped = true;
					}
					prev = x;
				}
			}
			if (!*slask)
			{
#if defined( _WIN32) && !defined(__CYGWIN__)
				strcpy_s(slask, sizeof(slask), "::");
#else
				strcpy(slask, "::");
#endif
			}
			hostname = slask;
			return true;
		}
		else
		{
			// %! TODO: ipv6 reverse lookup
			struct sockaddr_in6 *sa_in = (struct sockaddr_in6 *)sa;
			struct hostent *h = gethostbyaddr( (const char *)&sa_in -> sin6_addr, sizeof(sa_in -> sin6_addr), AF_INET6);
			if (h)
			{
				hostname = h -> h_name;
				return true;
			}
		}
		break;
#endif
	}
	return false;
#else
	char host[NI_MAXHOST];
	// NI_NOFQDN
	// NI_NUMERICHOST
	// NI_NAMEREQD
	// NI_NUMERICSERV
	// NI_DGRAM
	int n = getnameinfo(sa, sa_len, host, sizeof(host), NULL, 0, flags);
	if (n)
	{
		// EAI_AGAIN
		// EAI_BADFLAGS
		// EAI_FAIL
		// EAI_FAMILY
		// EAI_MEMORY
		// EAI_NONAME
		// EAI_OVERFLOW
		// EAI_SYSTEM
		return false;
	}
	hostname = host;
	return true;
#endif // NO_GETADDRINFO
}