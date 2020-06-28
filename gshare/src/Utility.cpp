#include "Utility.h"
#include "Ipv4Address.h"
#include "Ipv6Address.h"
#include "Base64.h"
#include <vector>
#if defined _WIN32 || defined _WIN64
#include <time.h>
#include <direct.h>
#else
#include <netdb.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
// --- stack
#include <cxxabi.h>
#include <execinfo.h>
#include <dlfcn.h>
#include <execinfo.h>  
#endif

#include <map>
#include <string.h>
#include <chrono>
#include "zlib.h"

#if !defined _WIN32 && !defined _WIN64
#include <strings.h>
#define strnicmp strncasecmp
#define MAX_PATH 1024
#endif

union FlagEnddian
{
    int     nFlag;
    char    cData;
};

using namespace std::chrono;
#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif

#define MAX_STACK_FRAMES   128
// defines for the random number generator
#define TWIST_IA        397
#define TWIST_IB       (TWIST_LEN - TWIST_IA)
#define UMASK           0x80000000
#define LMASK           0x7FFFFFFF
#define MATRIX_A        0x9908B0DF
#define TWIST(b,i,j)   ((b)[i] & UMASK) | ((b)[j] & LMASK)
#define MAGIC_TWIST(s) (((s) & 1) * MATRIX_A)
enum
{
    UperSpace = 'a' - 'A',
}; 

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
//namespace Utility
////////////////////////////////////////////////////////////////////////////////////////////////////
int Utility::FindString(const char *src, int len, const char *cnt, int cntLen)
{
    int count = cntLen<0 ? strlen(cnt) : cntLen;
    for (int pos=0; pos <= len-count; ++pos)
    {
        if (0 == strncmp(src, cnt, (size_t)count))
            return pos;

        src++;
    }
    return -1;
}

int Utility::FindString(const char *src, int len, const std::string &str)
{
    return FindString(src, len, str.c_str(), str.length());
}

StringList Utility::SplitString(const string &str, const string &sp, bool bSkipEmpty/*=false*/)
{
    StringList strLsRet;
    int nSizeSp = sp.size();
    if (!nSizeSp)
        return strLsRet;

    unsigned nPos = 0;
    while (nPos < str.size())
    {
        int nTmp = str.find(sp, nPos);
        string strTmp = str.substr(nPos, nTmp < 0 ? -1 : nTmp - nPos);
        if (strTmp.size() || !bSkipEmpty)
            strLsRet.push_back(strTmp);

        if (nTmp < int(nPos))
            break;
        nPos = nTmp + nSizeSp;
    }
    return strLsRet;
}

void Utility::ReplacePart(std::string &str, char part, char rpc)
{
    int count = str.length();
    char *ptr = &str.front();
    for (int pos = 0; pos <= count; ++pos)
    {
        if (ptr[pos] == part)
            ptr[pos] = rpc;
    }
}

uint32_t Utility::Crc32(const char *src, int len)
{
    static const uint32_t crc32Table[] =
    {
        0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
        0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
        0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
        0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
        0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
        0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
        0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
        0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
        0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
        0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
        0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
        0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
        0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
        0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
        0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
        0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
        0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
        0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
        0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
        0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
        0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
        0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
        0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
        0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
        0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
        0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
        0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
        0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
        0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
        0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
        0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
        0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
        0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
        0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
        0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
        0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
        0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
        0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
        0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
        0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
        0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
        0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
        0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
    };

    if (!src)
        return 0;

    uint32_t crc32 = 0xffffffffL;
    for (int i = 0; i < len; ++i)
    {
        crc32 = crc32Table[(crc32 ^ src[i]) & 0xff] ^ (crc32 >> 8);
    }

    return (crc32 ^ 0xffffffffL);
}

string Utility::base64(const char *str_in, int len)
{
	return Base64::encode((uint8_t*)str_in, len);
}

size_t Utility::base64d(const std::string &str_in, char *buf, size_t len)
{
    return Base64::decode(str_in, (uint8_t*)buf, len);
}

string Utility::l2string(long l)
{
	string str;
	char tmp[100];
	snprintf(tmp,sizeof(tmp),"%ld",l);
	str = tmp;
	return str;
}

string Utility::bigint2string(int64_t l)
{
    bool bBt = l < 0;
    char str[32] = { 0 };
    int pos = sizeof(str) - 1;
    str[pos - 1] = '0';

    if (l == 0)
        --pos;

    for (uint64_t tmp = bBt ? -l : l; tmp != 0; tmp /= 10)
    {
        uint64_t a = tmp % 10;
        str[--pos] = char('0' + a);
    }
    if (bBt)
        str[--pos] = '-';

    return str + pos;
}

string Utility::bigint2string(uint64_t l)
{
    char str[32] = { 0 };
    int pos = sizeof(str) - 1;
    str[pos - 1] = '0';

    if (l == 0)
        --pos;
    for (uint64_t tmp = l; tmp != 0; tmp /= 10)
    {
        uint64_t a = tmp % 10;
        str[--pos] = char('0' + a);
    }

	return str+pos;
}

int64_t Utility::str2int(const std::string &str, unsigned radix, bool *suc)
{
    unsigned count = str.length();
    if ((radix != 8 && radix != 10 && radix != 16) || count<1)
    {
        if (suc)
            *suc = false;
        return 0;
    }

    bool bSubMin = false;
    const char *c = str.c_str();
    unsigned i = 0;
    while (' ' == c[i] || '\t' == c[i])
    {
        ++i;
    }

    if (i<count && (c[i]=='+'|| c[i]=='-'))
    {
        if (c[i] == '-')
            bSubMin = true;
        ++i;
    }
    if (i >= count)
    {
        if (suc)
            *suc = false;
        return 0;
    }

    int64_t nRet = 0;
    bool bS = true;
    for (; i < count; ++i)
    {
        int nTmp = c[i] - '0';
        if (nTmp>10 && radix==16)
            nTmp = 10+(c[i] > 'F' ? c[i] - 'a' : c[i] - 'A');

        if (nTmp <0 || nTmp>=int(radix))
        {
            if (' ' != c[i] && '\t' != c[i])
                bS = false;
            break;
        }
        nRet = nRet * radix + nTmp;
    }
    if (suc)
        *suc = bS;
    if (bSubMin && bS)
        nRet = -nRet;

    return bS ? nRet : 0;
}

uint64_t Utility::atoi64(const string& str) 
{
	uint64_t l = 0;
	for (size_t i = 0; i < str.size(); ++i)
	{
		l = l * 10 + str[i] - 48;
	}
	return l;
}

unsigned int Utility::hex2unsigned(const string& str)
{
	unsigned int r = 0;
	for (size_t i = 0; i < str.size(); ++i)
	{
		r = r * 16 + str[i] - 48 - ((str[i] >= 'A') ? 7 : 0) - ((str[i] >= 'a') ? 32 : 0);
	}
	return r;
}

/*
* Encode string per RFC1738 URL encoding rules
* tnx rstaveley
*/
string Utility::rfc1738_encode(const string& src)
{
static	char hex[] = "0123456789ABCDEF";
	string dst;
	for (size_t i = 0; i < src.size(); ++i)
	{
		unsigned char c = static_cast<unsigned char>(src[i]);
		if (isalnum(c))
		{
			dst += c;
		}
		else
		if (c == ' ')
		{
			dst += '+';
		}
		else
		{
			dst += '%';
			dst += hex[c / 16];
			dst += hex[c % 16];
		}
	}
	return dst;
} // rfc1738_encode

/*
* Decode string per RFC1738 URL encoding rules
* tnx rstaveley
*/
string Utility::rfc1738_decode(const string& src)
{
	string dst;
	for (size_t i = 0; i < src.size(); ++i)
	{
		if (src[i] == '%' && isxdigit(src[i + 1]) && isxdigit(src[i + 2]))
		{
			char c1 = src[++i];
			char c2 = src[++i];
			c1 = c1 - 48 - ((c1 >= 'A') ? 7 : 0) - ((c1 >= 'a') ? 32 : 0);
			c2 = c2 - 48 - ((c2 >= 'A') ? 7 : 0) - ((c2 >= 'a') ? 32 : 0);
			dst += (char)(c1 * 16 + c2);
		}
		else
		if (src[i] == '+')
		{
			dst += ' ';
		}
		else
		{
			dst += src[i];
		}
	}
	return dst;
} // rfc1738_decode


bool Utility::IsBigEndian(void)
{
    const static FlagEnddian sFlag = { 1 };
    return sFlag.cData == sFlag.nFlag;
}

void Utility::toBigendian(uint32_t value, void *buff)
{
    if (!buff)
        return;

    if (IsBigEndian())
    {
        char *abyte = (char *)buff;
        for (uint32_t i = 0; i < sizeof(value); ++i)
        {
            abyte[sizeof(value) - i - 1] = ((char *)&value)[i];
        }
    }
    else
    {
        memcpy(buff, &value, sizeof(value));
    }
}

uint32_t Utility::fromBigendian(const void *buff)
{
    if (!buff)
        return -1;

    uint32_t value;
    if (IsBigEndian())
    {
        const char *src = (char *)buff;
        char *tmp = (char *)&value;
        uint32_t sz = sizeof(value);
        for (uint32_t i = 0; i < sz; ++i)
        {
            tmp[i] = src[sz - 1 - i];
        }
    }
    else
    {
        memcpy(&value, buff, sizeof(value));
    }

    return value;
}

int64_t Utility::usTimeTick()
{
    chrono::system_clock::time_point now = chrono::system_clock::now();
    auto ns = chrono::duration_cast<chrono::microseconds>(now.time_since_epoch());
    return ns.count();
}

int64_t Utility::msTimeTick()
{
    chrono::system_clock::time_point now = chrono::system_clock::now();
    auto ms = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch());
    return ms.count();
}

long Utility::secTimeCount()
{
    chrono::system_clock::time_point now = chrono::system_clock::now();
    auto sec = chrono::duration_cast<chrono::seconds>(now.time_since_epoch());
    return long(sec.count());
}

string Utility::dateString(int64_t ms, const std::string &fmt/*="y-M-d h-m-s"*/)
{
    string strData;
    auto src = fmt.c_str();
    int count = 0;
    char last = 0;
    char tmp[24];
    char f[12] = { '%' };
    auto sec = ms / 1000;
    tm *tmNow = localtime(&sec);
    for (size_t i = 0; i < fmt.length(); ++i)
    {
        switch (src[i])
        {
        case 'y':
        case 'M':
        case 'd':
        case 'h':
        case 'm':
        case 's':
        case 'z':
            last = src[i]; count++; break;
        default:
            strData += src[i]; break;
        }
        if (last && (i+1==fmt.length() || last!=src[i+1]))
        {
            if (count > 1)
                snprintf(f + 1, 10, "0%dd", count);
            else
                strcpy(f, "%d");
            switch (last)
            {
            case 'y':
                snprintf(tmp, 23, f, tmNow->tm_year + 1900);
                strData += tmp; break;
            case 'M':
                snprintf(tmp, 23, f, tmNow->tm_mon + 1);
                strData += tmp; break;
            case 'd':
                snprintf(tmp, 23, f, tmNow->tm_mday);
                strData += tmp; break;
            case 'h':
                snprintf(tmp, 23, f, tmNow->tm_hour);
                strData += tmp; break;
            case 'm':
                snprintf(tmp, 23, f, tmNow->tm_min);
                strData += tmp; break;
            case 's':
                snprintf(tmp, 23, f, tmNow->tm_sec);
                strData += tmp; break;
            case 'z':
                snprintf(tmp, 23, f, ms % 1000);
                strData += tmp; break;
            default:
                break;
            }
            count = 0;
            last = 0;
        }
    }
    return strData;
}

bool Utility::isipv4(const string& str)
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

bool Utility::isipv6(const string& str)
{
    StringList ls = SplitString(str, ":");
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
        uint64_t res = str2int(itr, 16, &bSuc);
        if (!bSuc || res>0xffff)
            return false;
    }

    return true;
}

bool Utility::u2ip(const string& str, ipaddr_t& l)
{
	struct sockaddr_in sa;
	bool r = Utility::u2ip(str, sa);
	memcpy(&l, &sa.sin_addr, sizeof(l));
	return r;
}


#ifdef ENABLE_IPV6
#ifdef IPPROTO_IPV6
bool Utility::u2ip(const string& str, struct in6_addr& l)
{
	struct sockaddr_in6 sa;
	bool r = Utility::u2ip(str, sa);
	l = sa.sin6_addr;
	return r;
}
#endif
#endif

void Utility::l2ip(const ipaddr_t ip, string& str)
{
	struct sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	memcpy(&sa.sin_addr, &ip, sizeof(sa.sin_addr));
	Utility::reverse( (struct sockaddr *)&sa, sizeof(sa), str, NI_NUMERICHOST);
}

void Utility::l2ip(const in_addr& ip, string& str)
{
	struct sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_addr = ip;
	Utility::reverse( (struct sockaddr *)&sa, sizeof(sa), str, NI_NUMERICHOST);
}


#ifdef ENABLE_IPV6
#ifdef IPPROTO_IPV6
void Utility::l2ip(const struct in6_addr& ip, string& str,bool mixed)
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
		Utility::reverse( (struct sockaddr *)&sa, sizeof(sa), str, NI_NUMERICHOST);
		return;
	}
	str = slask;
}

int Utility::in6_addr_compare(in6_addr a,in6_addr b)
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

void Utility::ResolveLocal()
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

const string& Utility::GetLocalHostname()
{
	if (!s_local_resolved)
		ResolveLocal();

	return s_hostName;
}

ipaddr_t Utility::GetLocalIP()
{
	if (!s_local_resolved)
		ResolveLocal();

	return s_ip;
}

const string& Utility::GetLocalAddress()
{
	if (!s_local_resolved)
		ResolveLocal();

	return s_address;
}

#ifdef ENABLE_IPV6
#ifdef IPPROTO_IPV6
const struct in6_addr& Utility::GetLocalIP6()
{
	if (!s_local_resolved)
		ResolveLocal();

	return s_local_ip6;
}

const string& Utility::GetLocalAddress6()
{
	if (!s_local_resolved)
		ResolveLocal();

	return s_local_addr6;
}
#endif
#endif

const string Utility::GetEnv(const string &name)
{
#if defined( _WIN32) && !defined(__CYGWIN__)
	size_t sz = 0;
	char tmp[2048];
	if (getenv_s(&sz, tmp, sizeof(tmp), name.c_str()))
		*tmp = 0;

	return tmp;
#else
	char *s = getenv(name.c_str());
	if (!s)
		return "";
	return s;
#endif
}

void Utility::SetEnv(const string& var,const string& value)
{
#if (defined(SOLARIS8) || defined(SOLARIS))
	{
		static map<string, char *> vmap;
		if (vmap.find(var) != vmap.end())
			delete[] vmap[var];

		size_t sz = var.size() + 1 + value.size() + 1;
		vmap[var] = new char[sz];
		snprintf(vmap[var], sz, "%s=%s", var.c_str(), value.c_str());
		putenv( vmap[var] );
	}
#elif defined _WIN32
	{
		string slask = var + "=" + value;
		_putenv( (char *)slask.c_str());
	}
#else
	setenv(var.c_str(), value.c_str(), 1);
#endif
}

string Utility::Sa2String(struct sockaddr *sa)
{
#ifdef ENABLE_IPV6
#ifdef IPPROTO_IPV6
	if (sa -> sa_family == AF_INET6)
	{
		struct sockaddr_in6 *sa6 = (struct sockaddr_in6 *)sa;
		string tmp;
		Utility::l2ip(sa6 -> sin6_addr, tmp);
		return tmp + ":" + Utility::l2string(ntohs(sa6 -> sin6_port));
	}
#endif
#endif
	if (sa -> sa_family == AF_INET)
	{
		struct sockaddr_in *sa4 = (struct sockaddr_in *)sa;
		ipaddr_t a;
		memcpy(&a, &sa4 -> sin_addr, 4);
		string tmp;
		Utility::l2ip(a, tmp);
		return tmp + ":" + Utility::l2string(ntohs(sa4 -> sin_port));
	}
	return "";
}

bool Utility::u2ip(const string& host, struct sockaddr_in& sa, int ai_flags)
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
    if (Utility::isipv4(host))
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
        ai = vec[Utility::Rnd() % vec.size()];
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
bool Utility::u2ip(const string& host, struct sockaddr_in6& sa, int ai_flags)
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
				addr16[i++] = htons(Utility::hex2unsigned(bytepair));
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
	if (Utility::isipv6(host))
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
		ai = vec[Utility::Rnd() % vec.size()];
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

bool Utility::reverse(struct sockaddr *sa, socklen_t sa_len, string& hostname, int flags)
{
	string service;
	return reverse(sa, sa_len, hostname, service, flags);
}

bool Utility::reverse(struct sockaddr *sa, socklen_t sa_len, string& hostname, string& service, int flags)
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

unsigned long Utility::ThreadID()
{
#ifdef _WIN32
	return GetCurrentThreadId();
#else
	return (unsigned long)pthread_self();
#endif
}

string Utility::ToString(double d)
{
	char tmp[100];
	snprintf(tmp, sizeof(tmp), "%f", d);
	return tmp;
}

wstring Utility::Utf8ToUnicode(const string & str)
{
    size_t nCount  = str.length();
    wstring ret;
    ret.resize(nCount + 1);
    if (ret.size() >= nCount + 1)
    {
        wchar_t *strUnicode = &ret.front();
        int nRst = 0;
        short nDecBt = 0;
        for (size_t i = 0; i < nCount; ++i)
        {
            if (!nDecBt)
            {
                if (0xc0 == (str[i] & 0xE0))
                {
                    nDecBt = 1;
                    wchar_t c = (short)str[i] << 6;
                    strUnicode[nRst] = c & 0x07c0;
                }
                else if (0xE0 == (str[i] & 0xF0))
                {
                    nDecBt = 2;
                    wchar_t c = (short)str[i] << 12;
                    strUnicode[nRst] = 0xf000 & c;
                }
                else
                {
                    strUnicode[nRst++] = str[i];
                }
            }
            else if (nDecBt == 2 && 0x80 == (str[i] & 0xc0))
            {
                --nDecBt;
                wchar_t c = (short)str[i] << 6;
                strUnicode[nRst] |= 0x0fc0 & c;
            }
            else if (nDecBt == 1 && 0x80 == (str[i] & 0xc0))
            {
                --nDecBt;
                strUnicode[nRst++] |= 0x3f & str[i];
            }
            else
            {
                return wstring();
            }
        }
        strUnicode[nRst] = 0;
        return ret;
    }
    return wstring();
}

string Utility::UnicodeToUtf8(const std::wstring &str)
{
    size_t nLen = str.length();
    string ret;
    ret.resize(nLen * 3 + 1);
    if (ret.size() >= nLen * 3 + 1)
    {
        char *strUtf8 = &ret.at(0);
        int nRst = 0;
        short nDecBt = 0;
        for (size_t i = 0; i < nLen; ++i)
        {
            wchar_t cTmp = str[i];
            if (!nDecBt)
            {
                if (cTmp < 0x80)
                    strUtf8[nRst++] = char(cTmp);
                else if (cTmp >= 0x80 && cTmp < 0x07ff)
                {
                    strUtf8[nRst++] = 0xc0 | ((cTmp >> 6) & 0x1f);
                    strUtf8[nRst++] = 0x80 | (cTmp & 0x3f);
                }
                else if (cTmp > 0x07ff)
                {
                    strUtf8[nRst++] = 0xe0 | ((cTmp >> 12) & 0x0f);
                    strUtf8[nRst++] = 0x80 | ((cTmp >> 6) & 0x3f);
                    strUtf8[nRst++] = 0x80 | (cTmp & 0x3f);
                }
            }
        }
        strUtf8[nRst] = 0;
        return ret;
    }
    return string();
}

string Utility::FromUtf8(const string& str)
{
    string ret;
    wstring unic = Utf8ToUnicode(str);
    if (unic.length())
    {
        setlocale(LC_ALL, "");
        size_t nLen = wcstombs(NULL, unic.c_str(), 0) + 1;
        ret.resize(nLen);
        if (ret.size() < nLen)
            return string();

        char *strRet = &ret.front();
        if (wcstombs(strRet, unic.c_str(), nLen) <= 0)
            return string();

        strRet[nLen - 1] = 0;
        setlocale(LC_ALL, "C");
    }
    return ret;
}

// 110yyyxx 10xxxxxx	
string Utility::ToUtf8(const string& str)
{
    setlocale(LC_ALL, "");
    string ret;
    size_t nBuff = mbstowcs(NULL, str.c_str(), 0) + 1;
    if (nBuff > 0)
    {
        wstring strUnic;
        strUnic.resize(nBuff);
        if (strUnic.size() < nBuff)
            return string();

        int nTmp = mbstowcs(&strUnic.front(), str.c_str(), nBuff);
        if (nTmp > 0)
        {
            strUnic.at(nBuff - 1) = 0;
            ret = UnicodeToUtf8(strUnic);
        }
    }
    setlocale(LC_ALL, "C");
    return ret;
}

string Utility::Upper(const string &str)
{
    if (str.empty())
        return str;

    string ret = str;
    int count = ret.length();
    char *p = &ret.at(0);
    for (int i = 0; i< count; ++i)
    {
        if ('a' <= p[i] && p[i] <= 'z')
            p[i] -= UperSpace;
    }
    return ret;
}

string Utility::Lower(const std::string &str)
{
    if (str.empty())
        return str;

    string ret = str;
    int count = ret.length();
    char *p = count>0 ? &ret.at(0):NULL;
    for (int i = 0; i < count; ++i)
    {
        if ('A' <= p[i] && p[i] <= 'Z')
            p[i] += UperSpace;
    }
    return ret;
}

bool Utility::Compare(const string &str1, const string &str2, bool bCase)
{
    uint32_t len = str1.length();
    if (len != str2.length())
        return false;

    if (len == 0)
        return true;

    if (bCase)
        return 0 == strcmp(str1.c_str(), str2.c_str());

    return 0 == strnicmp(str1.c_str(), str2.c_str(), len);
}

int Utility::GzCompress(const char *data, unsigned n, char *zdata, unsigned nz)
{
    z_stream c_stream;
    int err = 0;
    if (data && n > 0)
    {
        c_stream.zalloc = (alloc_func)0;
        c_stream.zfree = (free_func)0;
        c_stream.opaque = (voidpf)0;
        if (deflateInit(&c_stream, Z_DEFAULT_COMPRESSION) != Z_OK) return -1;
        c_stream.next_in = (Bytef*)data;
        c_stream.avail_in = n;
        c_stream.next_out = (Bytef*)zdata;
        c_stream.avail_out = nz;
        while (c_stream.avail_in != 0 && c_stream.total_out < nz)
        {
            if (deflate(&c_stream, Z_NO_FLUSH) != Z_OK)
                return -1;
        }
        if (c_stream.avail_in != 0) return c_stream.avail_in;
        for (;;)
        {
            if ((err = deflate(&c_stream, Z_FINISH)) == Z_STREAM_END) break;
            if (err != Z_OK)
                return -1;
        }
        if (deflateEnd(&c_stream) != Z_OK)
            return -1;

        return c_stream.total_out;
    }
    return 0;
}

int Utility::GzDecompress(const char *zdata, unsigned nz, char *data, unsigned n)
{
    int err = 0;
    z_stream d_stream; /* decompression stream */

    d_stream.zalloc = (alloc_func)0;
    d_stream.zfree = (free_func)0;
    d_stream.opaque = (voidpf)0;
    d_stream.next_in = (Bytef *)zdata;
    d_stream.avail_in = 0;
    d_stream.next_out = (Bytef *)data;
    if (inflateInit(&d_stream) != Z_OK)
        return -1;

    while (d_stream.total_out<n && d_stream.total_in<nz)
    {
        d_stream.avail_in = d_stream.avail_out = 1;
        if ((err = inflate(&d_stream, Z_NO_FLUSH)) == Z_STREAM_END)
            break;
        if (err != Z_OK)
            return -1;
    }
    if (inflateEnd(&d_stream) != Z_OK || d_stream.total_in > nz)
        return -1;

    return d_stream.total_out;
}

string Utility::ModulePath()
{
    char strRet[MAX_PATH] = {0};
#ifdef _WIN32
    DWORD ret = ::GetModuleFileNameA(NULL, strRet, MAX_PATH - 1);
#else
    size_t ret = readlink("/proc/self/exe", strRet, MAX_PATH - 1);
#endif
    if (ret < 1)
        string();

    return strRet;
}

string Utility::ModuleDirectory()
{
    string strRet = ModulePath();
    if (strRet.empty())
        return strRet;
#ifdef _WIN32
    ReplacePart(strRet, '\\', '/');
#endif
    int idx = strRet.find_last_of('/');
    return idx >= 0 ? strRet.substr(0, idx) : string();
}

string Utility::ModuleName()
{
    string strRet = ModulePath();
    if (strRet.empty())
        return strRet;
#ifdef _WIN32
    ReplacePart(strRet, '\\', '/');
#endif
    int idx = strRet.find_last_of('/');
    return idx >= 0 ? strRet.substr(idx+1, strRet.length()-1) : string();
}

string Utility::CurrentDirectory()
{
    string strRet;
    strRet.resize(1024);
    if (strRet.size() < 1024)
        return string();

    if (!getcwd(&strRet.at(0), 1024))
    {
        return string(".");
    }
    return strRet.c_str();
}

bool Utility::ChangeDirectory(const string &to_dir)
{
#ifdef _WIN32
	return SetCurrentDirectory(to_dir.c_str()) ? true : false;
#else
	if (chdir(to_dir.c_str()) == -1)
		return false;
	
	return true;
#endif
}

void Utility::Sleep(int ms)
{
#if defined _WIN32 || defined _WIN64
	::Sleep(ms);
#else
	struct timeval tv;
	tv.tv_sec = ms / 1000;
	tv.tv_usec = (ms % 1000) * 1000;
	select(0, NULL, NULL, NULL, &tv);
#endif
}

void Utility::Dump(const std::string &file, int sig)
{
    FILE *fd = fopen(file.c_str(), "wb");
    if (!fd)
        return;

    char szLine[512] = { 0 };
#if !(defined _WIN32 || defined _WIN64)
    void *arrayB[MAX_STACK_FRAMES];
    size_t size = backtrace(arrayB, MAX_STACK_FRAMES);
    char **strings = (char**)backtrace_symbols(arrayB, size);
    int pid = getpid();
    for (size_t i = 0; i < size; ++i)
    {
        int len = sprintf(szLine, "%s\n", strings[i]);
        fwrite(szLine, 1, len, fd);

        char *str1 = strchr(szLine, '[');
        char *str2 = strchr(szLine, ']');
        if (str1 == NULL || str2 == NULL)
            continue;

        char addrline[32] = { 0 };
        memcpy(addrline, str1 + 1, str2 - str1);
        snprintf(szLine, sizeof(szLine), "addr2line -e /proc/%d/exe %s ", pid, addrline);
        FILE *file = popen(szLine, "r");
        if (NULL != fgets(szLine, 256, file))
            fprintf(fd, "%s\n", szLine);
        pclose(file);
    }
    free(strings);
    signal(sig, SIG_DFL);
#endif
    fclose(fd);
}
