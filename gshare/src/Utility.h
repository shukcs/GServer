#ifndef _SOCKETS_Utility_H
#define _SOCKETS_Utility_H

#include "socket_include.h"
#include <map>
#include <list>
#include <string>
#include <type_traits>
#include <stdconfig.h>

#define TWIST_LEN     624

typedef std::list<std::string> StringList;
template<typename E>
class VarTypeDef {
public:
    enum { eType = 0 };
};

#define MK_FUNDA_TYPE(T, enV) \
template<>  \
class VarTypeDef<T> { \
public: \
    enum { eType = enV }; \
};
MK_FUNDA_TYPE(std::string, 1)
MK_FUNDA_TYPE(int32_t, 2)
MK_FUNDA_TYPE(uint32_t, 3)
MK_FUNDA_TYPE(int16_t, 4)
MK_FUNDA_TYPE(uint16_t, 5)
MK_FUNDA_TYPE(int8_t, 6)
MK_FUNDA_TYPE(uint8_t, 7)
MK_FUNDA_TYPE(int64_t, 8)
MK_FUNDA_TYPE(uint64_t, 9)
MK_FUNDA_TYPE(double, 10)
MK_FUNDA_TYPE(float, 11)
MK_FUNDA_TYPE(bool, 12)
MK_FUNDA_TYPE(StringList, 13)
MK_FUNDA_TYPE(std::list<uint32_t>, 14)
MK_FUNDA_TYPE(std::list<int32_t>, 15)
MK_FUNDA_TYPE(std::list<int16_t>, 16)
MK_FUNDA_TYPE(std::list<uint16_t>, 17)
MK_FUNDA_TYPE(std::list<int8_t>, 18)
MK_FUNDA_TYPE(std::list<uint8_t>, 19)
MK_FUNDA_TYPE(std::list<int64_t>, 20)
MK_FUNDA_TYPE(std::list<uint64_t>, 21)
MK_FUNDA_TYPE(std::list<double>, 22)
MK_FUNDA_TYPE(std::list<float>, 23)

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

namespace Utility
{
    SHARED_DECL int FindString(const char *src, int len, const char *cnt, int cntLen = -1);
    SHARED_DECL StringList SplitString(const std::string &str, const std::string &sp, bool bSkipEmpty=true);
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

class SHARED_DECL Variant
{
public:
    enum VariantType
    {
        Type_Unknow = 0,
        Type_string = VarTypeDef<std::string>::eType,
        Type_int32 = VarTypeDef<int32_t>::eType,
        Type_uint32 = VarTypeDef<uint32_t>::eType,
        Type_int16 = VarTypeDef<int16_t>::eType,
        Type_uint16 = VarTypeDef<uint16_t>::eType,
        Type_int8 = VarTypeDef<int8_t>::eType,
        Type_uint8 = VarTypeDef<uint8_t>::eType,
        Type_int64 = VarTypeDef<int64_t>::eType,
        Type_uint64 = VarTypeDef<uint64_t>::eType,
        Type_double = VarTypeDef<double>::eType,
        Type_float = VarTypeDef<float>::eType,
        Type_bool = VarTypeDef<bool>::eType,
        Type_StringList = VarTypeDef<StringList>::eType,
        Type_ListBuff = Type_StringList,
        Type_ListI32 = VarTypeDef<std::list<int32_t>>::eType,
        Type_ListU32 = VarTypeDef<std::list<uint32_t>>::eType,
        Type_ListI16 = VarTypeDef<std::list<int16_t>>::eType,
        Type_ListU16 = VarTypeDef<std::list<uint16_t>>::eType,
        Type_ListI8 = VarTypeDef<std::list<int8_t>>::eType,
        Type_ListU8 = VarTypeDef<std::list<uint8_t>>::eType,
        Type_ListI64 = VarTypeDef<std::list<int64_t>>::eType,
        Type_ListU64 = VarTypeDef<std::list<uint64_t>>::eType,
        Type_ListF64 = VarTypeDef<std::list<double>>::eType,
        Type_ListF32 = VarTypeDef<std::list<float>>::eType,
        Type_buff,
    };
public:
    Variant(VariantType tp=Type_Unknow);
    Variant(const Variant &oth);
    Variant(const std::string &str);
    Variant(int sz, const char *buf);
    Variant(int32_t v);
    Variant(uint32_t v);
    Variant(int16_t v);
    Variant(uint16_t v);
    Variant(int8_t v);
    Variant(uint8_t v);
    Variant(int64_t v);
    Variant(uint64_t v);
    Variant(double v);
    Variant(float v);
    Variant(bool v);
    Variant(const StringList &v);
    ~Variant();

    bool Add(const Variant&v);
    VariantType GetType()const;
    const std::string &ToString()const;
    int32_t ToInt32()const;
    uint32_t ToUint32()const;
    int16_t ToInt16()const;
    uint16_t ToUint16()const;
    int8_t ToInt8()const;
    uint8_t ToUint8()const;
    int64_t ToInt64()const;
    uint64_t ToUint64()const;
    double ToDouble()const;
    float ToFloat()const;
    bool ToBool()const;
    const StringList &ToStringList()const;
    const char *GetBuff()const;
    int GetBuffLength()const;
    bool IsNull()const;
    Variant &operator=(const Variant &oth);
    template<class E>
    void SetValue(const E & val)
    {
        if (GetVarType<E>() == Type_Unknow)
            return;
        *this = Variant(val);
    }
    template<typename T, typename Contianer = std::list<T> >
    Contianer GetVarList()const
    {
        VariantType tp = GetVarType<Contianer>();
        if (tp == m_tp && tp >= Type_StringList && tp <= Type_ListF32)
            return *(Contianer*)m_list;
        return Contianer();
    }
public:
    template<typename T>
    static VariantType GetVarType()
    {
        return (VariantType)VarTypeDef<T>::eType;
    }
private:
    void ensureConstruction(VariantType tp);
    void ensureDestruct();
    bool isBasicalType()const;
    VariantType elementListType()const;
private:
    VariantType m_tp;
    union {
        int32_t             m_nS;
        uint32_t            m_nU;
        int16_t             m_nS16;
        uint16_t            m_nU16;
        int8_t              m_nS8;
        uint8_t             m_nU8;
        int64_t             m_nS64;
        uint64_t            m_nU64;
        double              m_f64;
        float               m_f32;
        bool                m_bool;
        void                *m_list;
    };
};

#endif // _SOCKETS_Utility_H

