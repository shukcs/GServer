#include "Utility.h"
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
#include <thread>
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

static double power(double x, int y)
{
    auto ret = 1.0;
    bool b = y > 0;
    while (y)
    {
        b ? y-- : y++;
        ret *= x;
    }
    return b ? ret : 1 / ret;
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

int Utility::FindString(const char *src, int len, const char *cnt, int cntLen, bool bCase)
{
    int count = cntLen<0 ? strlen(cnt) : cntLen;
    for (int pos=0; pos <= len-count; ++pos)
    {
        if (bCase && 0==strncmp(src, cnt, (size_t)count))
            return pos;
        else if (!bCase && 0 == strnicmp(src, cnt, (size_t)count))
            return pos;

        src++;
    }
    return -1;
}

int Utility::FindString(const std::string &src, const std::string &str, bool bCase)
{
    return FindString(src.c_str(), src.size(), str.c_str(), str.length(), bCase);
}

int Utility::FindString(const char *src, int len, const std::string &str, bool bCase)
{
    return FindString(src, len, str.c_str(), str.length(), bCase);
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

int64_t Utility::atoi64(const string &a, bool *bSuc)
{
    auto str = Trim(a);
    bool bO = true;
    size_t i = 0;
    if (a[0] == '-')
    {
        bO = false;
        i++;
    }

    bool suc = true;
    int64_t l = 0;
	for (; i < str.size(); ++i)
	{
        int c = str[i];
        if (c<'0' || c>'9')
        {
            suc = false;
            break;
        }
		l = l * 10 + c - '0';
	}
    if (bSuc)
        *bSuc = suc;

    if (!bO)
        l = 0-l;

	return l;
}

double Utility::atodouble(const std::string &str, bool *bSuc)
{
    auto fmt = Upper(Trim(str));
    auto strList = SplitString(fmt, "E");
    bool suc = true;
    if (strList.size() > 2)
        suc = false;

    double base = 1.0;
    bool bSubmin = false;
    if (suc && strList.size()==2)
        base = power(10, atoi64(strList.back(), &suc));

    if (suc)
    {
        strList = SplitString(strList.front(), ".");
        if (strList.size()!=1 && strList.size()!=2)
            suc = false;
    }

    double ret = 0;
    if (suc)
    {
        string tmp = strList.front();
        if (tmp.size()>1 && tmp.front() == '-')
            bSubmin = true;

        ret = atoi64(tmp, &suc);
    }

    if (suc && strList.size() == 2)
    {
        const string &tmp = strList.back();
        auto aftPnt = (double)atoi64(tmp, &suc) / power(10, tmp.size());
        bSubmin ? (ret -= aftPnt) : (ret += aftPnt);
    }
    if (bSuc)
        *bSuc = suc;

    return ret * base;
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


bool Utility::IsLittleEndian(void)
{
    const static FlagEnddian sFlag = { 1 };
    return sFlag.cData == sFlag.nFlag;
}

void Utility::toBigendian(uint32_t value, void *buff)
{
    if (!buff)
        return;

    if (IsLittleEndian())
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
    if (IsLittleEndian())
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

uint16_t Utility::u16fromBigendian(const void *buff)
{
    if (!buff)
        return -1;

    uint16_t value;
    if (IsLittleEndian())
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

int Utility::ThreadID()
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
    std::this_thread::sleep_for(milliseconds(ms));
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

double Utility::ToCycleValue(double val, double lowBound, double upBound)
{
    auto cycle = upBound - lowBound;
    if (DoubleEqual(lowBound, upBound) || cycle<0)
        return 0.0;

    ///计算超出多少个周期
    auto nCycle = (int)((val - lowBound) / cycle);
    if (val < lowBound)///负数取整余数为负，所以再-1
        nCycle--;

    return val - cycle * nCycle;
}
