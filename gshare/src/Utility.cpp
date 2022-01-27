#include "Utility.h"
#include "Ipv4Address.h"
#include "Ipv6Address.h"
#include "Base64.h"
#include <vector>
#if defined _WIN32 || defined _WIN64
#include <time.h>
#include <direct.h>
#else
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


#define MAX_STACK_FRAMES   128
enum {
    Utf8_Byte1 = 0x00000080,
    Utf8_Byte2 = 0x00000800,
    Utf8_Byte3 = 0x00010000,
    Utf8_Byte4 = 0x00200000,
    Utf8_Byte5 = 0x04000000,
    Utf8_Byte6 = 0x80000000,

    Utf8Head_Commom = 0b10000000,
    Utf8Head_2Byte = 0b11000000,
    Utf8Head_3Byte = 0b11100000,
    Utf8Head_4Byte = 0b11110000,
    Utf8Head_5Byte = 0b11111000,
    Utf8Head_6Byte = 0b11111100,

    Utf8Mask_Commom = 0b11000000,
    Utf8Mask_2Byte = 0b11100000,
    Utf8Mask_3Byte = 0b11110000,
    Utf8Mask_4Byte = 0b11111000,
    Utf8Mask_5Byte = 0b11111100,
    Utf8Mask_6Byte = 0b11111110,

    Utf8DataMask_Common = 0B111111,
    Utf8DataMask_2 = 0B11111,
    Utf8DataMask_3 = 0B1111,
    Utf8DataMask_4 = 0B111,
    Utf8DataMask_5 = 0B11,
    Utf8DataMask_6 = 0B1,
};

union FlagEnddian
{
    int     nFlag;
    char    cData;
};
enum
{
    UperSpace = 'a' - 'A',
}; 

#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif
using namespace std;
using namespace std::chrono;
// statics
static const char *sRandStrTab = "QWERTYUIOPASDFGHJKLZXCVBNMqwertyuiopasdfghjklzxcvbnm0123456789";

static bool isUtf8(const uint8_t *str, size_t len)
{
    for (size_t i = 0; i < len; ++i)
    {
        if (Utf8Head_Commom != (Utf8Mask_Commom & str[i]))
            return false;
    }

    return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//namespace Utility
////////////////////////////////////////////////////////////////////////////////////////////////////
string Utility::RandString(int len /*= 6*/, bool skipLow/*=false*/)
{
    if (len > 16 || len<1)
        return string();

    int64_t tmp = rand() + duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
    char ret[17] = { 0 };
    uint32_t n = uint32_t(tmp / 1023);
    ret[n % len] = char('0' + n % 10);

    for (int i = 0; i < len; ++i)
    {
        if (ret[i] == 0)
            ret[i] = sRandStrTab[tmp / (19 + i) % 62];
    }

    return skipLow ? Upper(ret) : ret;
}

const std::string & Utility::EmptyStr()
{
    static string sEmptyStr;
    return sEmptyStr;
}

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

int Utility::FindString(const std::string &src, const std::string &str)
{
    return FindString(src.c_str(), src.size(), str.c_str(), str.length());
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


string Utility::Trim(const std::string& str)
{
    string ret;
    auto ptr = str.c_str();
    for (size_t i = 0; i < str.length(); i++)
    {
        if (ptr[i] == ' ' || ptr[i] == '\t' || ptr[i] == '\n' || ptr[i] == '\r')
            continue;

        ret += (ptr[i]);
    }
    return ret;
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
		else if (src[i] == '+')
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
    auto now = system_clock::now();
    auto ns = duration_cast<microseconds>(now.time_since_epoch());
    return ns.count();
}

int64_t Utility::msTimeTick()
{
    auto now = system_clock::now();
    auto ms = duration_cast<milliseconds>(now.time_since_epoch());
    return ms.count();
}

long Utility::secTimeCount()
{
    auto now = system_clock::now();
    auto sec = duration_cast<seconds>(now.time_since_epoch());
    return long(sec.count());
}

string Utility::dateString(int64_t ms, const string &fmt/*="y-M-d h:m:s.z"*/)
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
            strcpy(f, "%d");
            if (count > 1 && count < 5)
            {
                f[1] = '0';
                f[2] = '0' + count;
                f[3] = 'd';
                f[4] = 0;
            }

            count = -1;
            switch (last)
            {
            case 'y': count = tmNow->tm_year + 1900;break;
            case 'M': count = tmNow->tm_mon + 1; break;
            case 'd': count = tmNow->tm_mday; break;
            case 'h': count = tmNow->tm_hour; break;
            case 'm': count = tmNow->tm_min; break;
            case 's': count = tmNow->tm_sec; break;
            case 'z': count = ms % 1000; break;
            default:
                break;
            }

            if (count >= 0)
            {
                sprintf(tmp, f, count);
                strData += tmp;
            }
            count = 0;
            last = 0;
        }
    }
    return strData;
}

int64_t Utility::timeStamp(int y, int m, int d, int h, int nM, int s, int ms)
{
    tm tmLac = { 0 };
    tmLac.tm_year = y - 1900;
    tmLac.tm_mon = m - 1;
    tmLac.tm_mday = d;
    tmLac.tm_hour = h;
    tmLac.tm_min = nM;
    tmLac.tm_sec = s;
    int64_t ret = mktime(&tmLac);
    return ret * 1000 + ms;
}

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

int Utility::Utf8Length(const std::wstring &str)
{
    int ret = 0;
    for (size_t i = 0; i < str.length(); ++i)
    {
        uint32_t cTmp = 0xffffffff & str[i];
        if (cTmp & Utf8_Byte6)
            return -1;

        if (cTmp < Utf8_Byte1)
            ret++;
        else if (cTmp >= Utf8_Byte1 && cTmp < Utf8_Byte2)
            ret += 2;
        else if (cTmp >= Utf8_Byte2 && cTmp < Utf8_Byte3)
            ret += 3;
        else if (cTmp >= Utf8_Byte3 && cTmp < Utf8_Byte4)
            ret += 4;
        else if (cTmp >= Utf8_Byte4 && cTmp < Utf8_Byte5)
            ret += 5;
        else if (cTmp >= Utf8_Byte5 && cTmp < Utf8_Byte6)
            ret += 6;
    }

    return ret;
}

int Utility::UnicodeLength(const string & utf8)
{
    int ret = 0;
    auto nCount = utf8.length();
    auto pStr = (const uint8_t *)utf8.c_str();
    for (size_t i = 0; i < nCount; ++i)
    {
        if (pStr[i] < Utf8Head_Commom)
        {
            ++ret;
            continue;
        }
        
        int nCheck = 0;
        if ((pStr[i] & Utf8Mask_2Byte) == Utf8Head_2Byte)
            nCheck = 1;
        else if ((pStr[i] & Utf8Mask_3Byte) == Utf8Head_3Byte)
            nCheck = 2;
        else if ((pStr[i] & Utf8Mask_4Byte) == Utf8Head_4Byte)
            nCheck = 3;
        else if ((pStr[i] & Utf8Mask_5Byte) == Utf8Head_5Byte)
            nCheck = 4;
        else if ((pStr[i] & Utf8Mask_6Byte) == Utf8Head_6Byte)
            nCheck = 5;
        else
            return -1;

        if (i+nCheck >= nCount || !isUtf8(pStr + i + 1, nCheck))
            return -1;

        ret++;
        i+= nCheck;
    }

    return ret;
}

wstring Utility::Utf8ToUnicode(const string & str)
{
    size_t nCount = UnicodeLength(str);
    wstring ret;
    ret.resize(nCount);
    if (ret.size() >= nCount)
    {
        wchar_t *strUnicode = &ret.front();
        int nRst = 0;
        short nDecBt = 0;
        for (size_t i = 0; i < str.length(); ++i)
        {
            if (!nDecBt)
            {
                if(0 == (str[i] & Utf8Head_Commom))
                {
                    strUnicode[nRst++] = str[i];
                }
                else if (Utf8Head_2Byte == (str[i] & Utf8Mask_2Byte))
                {
                    nDecBt = 1;
                    strUnicode[nRst++] = (Utf8DataMask_2 & str[i]) << 6;
                }
                else if (Utf8Head_3Byte == (str[i] & Utf8Mask_3Byte))
                {
                    nDecBt = 2;
                    strUnicode[nRst++] = (Utf8DataMask_3 & str[i]) << 12;
                }
                else if (Utf8Head_4Byte == (str[i] & Utf8Mask_4Byte))
                {
                    nDecBt = 3;
                    strUnicode[nRst++] = (Utf8DataMask_4 & str[i]) << 12;
                }
                else if (Utf8Head_5Byte == (str[i] & Utf8Mask_5Byte))
                {
                    nDecBt = 4;
                    strUnicode[nRst++] = (Utf8DataMask_5 & str[i]) << 12;
                }
                else if (Utf8Head_6Byte == (str[i] & Utf8Mask_6Byte))
                {
                    nDecBt = 5;
                    strUnicode[nRst++] = (Utf8DataMask_6 & str[i]) << 12;
                }
            }
            else if (nDecBt-- > 0)
            {
                strUnicode[nRst-1] |= (str[i] & Utf8DataMask_Common) << 6*nDecBt;
            }
        }
        return ret;
    }
    return wstring();
}

string Utility::UnicodeToUtf8(const std::wstring &str)
{
    string ret;
    size_t nLen = Utf8Length(str);
    if (nLen < 0)
        return ret;

    ret.resize(nLen);
    if (ret.size() < nLen)
        return string();

    char *strUtf8 = &ret.front();
    int nRst = 0;
    for (size_t i = 0; i < str.length(); ++i)
    {
        uint32_t cTmp = 0xffffffff &str[i];
        uint8_t count = 0;
        if (cTmp < Utf8_Byte1)
        {
            strUtf8[nRst++] = char(cTmp);
        }
        else if (cTmp >= Utf8_Byte1 && cTmp < Utf8_Byte2)
        {
            strUtf8[nRst++] = Utf8Head_2Byte | ((cTmp >> 6) & Utf8DataMask_2);
            count = 1;
        }
        else if (cTmp >= Utf8_Byte2 && cTmp < Utf8_Byte3)
        {
            strUtf8[nRst++] = Utf8Head_3Byte | ((cTmp >> 12) & Utf8DataMask_3);
            count = 2;
        }
        else if (cTmp >= Utf8_Byte3 && cTmp < Utf8_Byte4)
        {
            strUtf8[nRst++] = Utf8Head_4Byte | ((cTmp >> 18) & Utf8DataMask_4);
            count = 3;
        }
        else if (cTmp >= Utf8_Byte4 && cTmp < Utf8_Byte5)
        {
            strUtf8[nRst++] = Utf8Head_5Byte | ((cTmp >> 24) & Utf8DataMask_5);
            count = 4;
        }
        else if (cTmp >= Utf8_Byte5)
        {
            strUtf8[nRst++] = Utf8Head_6Byte | ((cTmp >> 30) & Utf8DataMask_6);
            count = 5;
        }

        for (auto k = 0; k < count; ++k)
        {
            strUtf8[nRst++] = Utf8Head_Commom | ((cTmp >> 6*(count-k-1)) & Utf8DataMask_Common);
        }
    }
    return ret;
}

string Utility::FromUtf8(const string& str, const string &cd)
{
    string ret;
    wstring unic = Utf8ToUnicode(str);
    if (unic.length())
    {
        setlocale(LC_ALL, cd.c_str());
        auto nLen = wcstombs(NULL, unic.c_str(), 0) ;
        if (nLen > unic.length() * 6)///没有支持的字符集
            return ret;

        ret.resize(nLen);
        if (ret.size() < nLen)
            return string();

        char *strRet = &ret.front();
        if (wcstombs(strRet, unic.c_str(), nLen) <= 0)
            return string();

        setlocale(LC_ALL, "C");
    }
    return ret;
}

// 110yyyxx 10xxxxxx	
string Utility::ToUtf8(const string& str, const string &cd)
{
    string ret;
    setlocale(LC_ALL, cd.c_str());
    size_t nBuff = mbstowcs(NULL, str.c_str(), 0);
    if (nBuff > 0 && nBuff<= str.length())///nBuff>str.length()，没有支持的字符集
    {
        wstring strUnic;
        strUnic.resize(nBuff);
        if (strUnic.size() < nBuff)
            return string();

        int nTmp = mbstowcs(&strUnic.front(), str.c_str(), nBuff);
        if (nTmp > 0)
            ret = UnicodeToUtf8(strUnic);
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
    return idx >= 0 ? strRet.substr(0, idx+1) : string();
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

bool Utility::PipeCmd(char* buff, int len, const char *cmd)
{
    if (!buff || len < 1)
        return false;

    bool ret = false;
#if !(defined _WIN32 || defined _WIN64)
    FILE *file = popen(cmd, "r");
    if (file && NULL != fgets(buff, len - 1, file))
        ret = true;
    pclose(file);
#else
    HANDLE hReadPipe = NULL; ///读取管道
    HANDLE hWritePipe = NULL; ///写入管道	
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };   ///安全属性
    if (CreatePipe(&hReadPipe, &hWritePipe, &sa, 0))
    {
        STARTUPINFOA         si = {};   ///控制命令行窗口信息.设置命令行窗口的信息为指定的读写管道
        GetStartupInfoA(&si);
        si.hStdError = hWritePipe;
        si.hStdOutput = hWritePipe;
        si.wShowWindow = SW_HIDE; //隐藏命令行窗口
        si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;

        PROCESS_INFORMATION pi = {};   ///进程信息	
        if (CreateProcessA(NULL, &string(cmd).at(0), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
        {
            WaitForSingleObject(pi.hProcess, 500);
            if (ReadFile(hReadPipe, buff, len, (DWORD*)&len, 0) && len > 0)
            {
                ret = true;
                buff[len] = 0;
            }
        }

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    CloseHandle(hWritePipe);
    CloseHandle(hReadPipe);
#endif
    return ret;
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
        if (PipeCmd(szLine, sizeof(szLine), szLine))
            fprintf(fd, "%s\n", szLine);
    }
    free(strings);
    signal(sig, SIG_DFL);
#endif
    fclose(fd);
}