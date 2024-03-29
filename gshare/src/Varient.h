﻿#ifndef __VG_VARIENT_H__
#define __VG_VARIENT_H__

#include <string>
#include <list>
#include <type_traits>
#include "stdconfig.h"

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
typedef std::list<std::string> StringList;

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

class Variant;
typedef std::list<Variant> VariantList;
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
        Type_StringList,
        Type_ListBuff = Type_StringList,
        Type_ListI32,
        Type_ListU32,
        Type_ListI16,
        Type_ListU16,
        Type_ListI8,
        Type_ListU8,
        Type_ListI64,
        Type_ListU64,
        Type_ListF64,
        Type_ListF32,
        Type_buff,
    };
public:
    Variant(VariantType tp = Type_Unknow);
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
    void SetBuff(const char *b, int sz);
    int GetBuffLength()const;
    bool IsNull()const;
    bool IsValid()const;
    Variant &operator=(const Variant &oth);
    template<class E>
    void SetValue(const E &val)
    {
        if (GetVarType<E>() == Type_Unknow)
            return;
        *this = Variant(val);
    }
    const VariantList &GetVarList()const;
    std::string Val2String() const;
public:
    template<typename T>
    static VariantType GetVarType()
    {
        return (VariantType)VarTypeDef<T>::eType;
    }
private:
    void ensureConstruct(VariantType tp);
    void ensureDestruct();
    bool isBasicalType()const;
    VariantType elementListType()const;
private:
    VariantType m_tp;
    bool        m_bValid;
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

#endif //__VG_VARIENT_H__

