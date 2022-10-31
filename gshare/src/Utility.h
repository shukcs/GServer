#ifndef _SOCKETS_Utility_H
#define _SOCKETS_Utility_H

#include "stdconfig.h"
#include <list>
#include <string>
#include <queue>

#define DoubleEqual(a, b) (0 == (int)(((a)-(b))*0x100000))
typedef std::list<std::string> StringList;
namespace Utility
{
    SHARED_DECL std::string RandString(int len = 6, bool skipLow=false);
    SHARED_DECL const std::string &EmptyStr();
    SHARED_DECL int FindString(const char *src, int len, const char *cnt, int cntLen = -1);
    SHARED_DECL int FindString(const char *src, int len, const std::string &str);
    SHARED_DECL int FindString(const std::string &src, const std::string &str);
    SHARED_DECL StringList SplitString(const std::string &str, const std::string &sp, bool bSkipEmpty = true); 
    std::string Trim(const std::string& str);
    SHARED_DECL void ReplacePart(std::string &str, char part, char rpc);
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
    SHARED_DECL void toBigendian(uint32_t s, void *buff);
    SHARED_DECL uint32_t fromBigendian(const void *buff);
    SHARED_DECL int64_t usTimeTick();
    SHARED_DECL int64_t msTimeTick();
    SHARED_DECL long secTimeCount();
    SHARED_DECL std::string dateString(int64_t ms, const std::string &fmt = "y-M-d h:m:s.z");
    SHARED_DECL int64_t timeStamp(int y, int m=1, int d=1, int h=0, int nM=0, int s=0, int ms=0);

    //utf8 utf16 local8bit相互转换，linux Unicode为32位,linux 需指定转码;选择编码 sudo dpkg-reconfigure --force locales
    SHARED_DECL int Utf8Length(const std::wstring &str);
    SHARED_DECL int UnicodeLength(const std::string &utf8);
    SHARED_DECL std::wstring Utf8ToUnicode(const std::string & str);
    SHARED_DECL std::string UnicodeToUtf8(const std::wstring &str);
    SHARED_DECL std::string FromUtf8(const std::string&, const std::string &c = "zh_CN.GB2312");
    SHARED_DECL std::string ToUtf8(const std::string&, const std::string &c= "zh_CN.GB2312");

    SHARED_DECL std::string Upper(const std::string&);
    SHARED_DECL std::string Lower(const std::string&);
    SHARED_DECL bool Compare(const std::string&, const std::string&, bool=true);

    SHARED_DECL int GzCompress(const char *data, unsigned n, char *zdata, unsigned);
    SHARED_DECL int GzDecompress(const char *zdata, unsigned nz, char *data, unsigned);
	
	/** Get environment variable */
    SHARED_DECL const std::string GetEnv(const std::string& name);
	/** Set environment variable.
		\param var Name of variable to set
		\param value Value */
    SHARED_DECL void SetEnv(const std::string& var,const std::string& value);
	/** Convert sockaddr struct to human readable string.
		\param sa Ptr to sockaddr struct */

    SHARED_DECL unsigned long ThreadID();
    SHARED_DECL std::string ToString(double d);
    /** File system stuff */
    SHARED_DECL std::string ModulePath();
    SHARED_DECL std::string ModuleDirectory();
    SHARED_DECL std::string ModuleName();
    SHARED_DECL std::string CurrentDirectory();
    SHARED_DECL bool ChangeDirectory(const std::string &to_dir);

	/** wait a specified number of ms */
    SHARED_DECL void Sleep(int ms);
    SHARED_DECL bool PipeCmd(char* buff, int len, const char *cmd);
    SHARED_DECL void Dump(const std::string &file, int sig);

    template<typename T, typename Contianer = std::list<T> >
    static bool IsContainsInList(const Contianer &ls, const T &e)
    {
        for (const T &itr : ls)
        {
            if (itr == e)
                return true;
        }
        return false;
    }
    template<typename T, typename Contianer = std::queue<T> >
    static bool Pop(Contianer &q, T &e)
    {
        if (q.empty())
            return false;

        e = q.back();
        q.pop();
        return true;
    }
}

#endif // _SOCKETS_Utility_H

