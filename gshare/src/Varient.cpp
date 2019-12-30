#include "Varient.h"

using namespace std;
const static StringList sEmptyStrLs;
////////////////////////////////////////////////////////////////////////////////
//Variant
////////////////////////////////////////////////////////////////////////////////
Variant::Variant(VariantType tp) : m_tp(tp), m_nU64(0)
{
    ensureConstruction(tp);
}

Variant::Variant(const Variant &oth) : m_tp(Type_Unknow)
{
    *this = oth;
}

Variant::Variant(const std::string &str) : m_tp(Type_string), m_list(new string(str))
{
}

Variant::Variant(int sz, const char *buf) : m_tp(Type_buff), m_list(new string(buf, sz))
{
}

Variant::Variant(int32_t v) : m_tp(Type_int32), m_nS(v)
{
}

Variant::Variant(uint32_t v) : m_tp(Type_uint32), m_nU(v)
{
}

Variant::Variant(int16_t v) : m_tp(Type_int16), m_nS16(v)
{
}

Variant::Variant(uint16_t v) : m_tp(Type_uint16), m_nU16(v)
{
}

Variant::Variant(int8_t v) : m_tp(Type_int8), m_nS8(v)
{
}

Variant::Variant(uint8_t v) : m_tp(Type_uint8), m_nU8(v)
{
}

Variant::Variant(int64_t v) : m_tp(Type_int64), m_nS64(v)
{
}

Variant::Variant(uint64_t v) : m_tp(Type_uint64), m_nU64(v)
{
}

Variant::Variant(double v) : m_tp(Type_double), m_f64(v)
{
}

Variant::Variant(float v) : m_tp(Type_float), m_f32(v)
{
}

Variant::Variant(bool v) : m_tp(Type_bool), m_bool(v)
{
}

Variant::Variant(const StringList &v) : m_tp(Type_StringList), m_list(new StringList(v))
{
}

Variant::~Variant()
{
    ensureDestruct();
}

bool Variant::Add(const Variant&v)
{
    if (!v.isBasicalType() || (Type_Unknow != m_tp && m_tp != v.elementListType()))
        return false;

    if (m_tp == Type_Unknow)
        ensureConstruction(v.elementListType());

    switch (v.GetType())
    {
    case Variant::Type_string:
        ((StringList*)m_list)->push_back(v.ToString()); break;
    case Variant::Type_int32:
        ((list<int32_t>*)m_list)->push_back(v.m_nS); break;
    case Variant::Type_uint32:
        ((list<uint32_t>*)m_list)->push_back(v.m_nU); break;
    case Variant::Type_int16:
        ((list<int16_t>*)m_list)->push_back(v.m_nS16); break;
    case Variant::Type_uint16:
        ((list<uint16_t>*)m_list)->push_back(v.m_nU16); break;
    case Variant::Type_int8:
        ((list<int8_t>*)m_list)->push_back(v.m_nS8); break;
    case Variant::Type_uint8:
        ((list<uint8_t>*)m_list)->push_back(v.m_nU8); break;
    case Variant::Type_int64:
        ((list<int64_t>*)m_list)->push_back(v.m_nS64); break;
    case Variant::Type_uint64:
        ((list<uint64_t>*)m_list)->push_back(v.m_nU64); break;
    case Variant::Type_double:
        ((list<double>*)m_list)->push_back(v.m_f64); break;
    case Variant::Type_float:
        ((list<float>*)m_list)->push_back(v.m_f32); break;
    case Variant::Type_buff:
        ((StringList*)m_list)->push_back(*(string*)v.m_list); break;
    default:
        break;
    }
    return true;
}

Variant::VariantType Variant::GetType() const
{
    return m_tp;
}

const string &Variant::ToString() const
{
    if (Type_string == m_tp)
        return *(string*)m_list;

    const static string sEmpryStr;
    return sEmpryStr;
}

int32_t Variant::ToInt32() const
{
    if (Type_int32 == m_tp || Type_uint32 == m_tp)
        return m_nS;
    return 0;
}

uint32_t Variant::ToUint32() const
{
    if (Type_int32 == m_tp || Type_uint32 == m_tp)
        return m_nU;
    return 0;
}

int16_t Variant::ToInt16() const
{
    if (Type_int32 == m_tp || Type_uint32 == m_tp)
        return m_nS16;
    return 0;
}

uint16_t Variant::ToUint16() const
{
    if (Type_int32 == m_tp || Type_uint32 == m_tp)
        return m_nU16;
    return 0;
}

int8_t Variant::ToInt8() const
{
    if (Type_int8 == m_tp || Type_uint8 == m_tp)
        return m_nS8;
    return 0;
}

uint8_t Variant::ToUint8() const
{
    if (Type_int8 == m_tp || Type_uint8 == m_tp)
        return m_nU8;
    return 0;
}

int64_t Variant::ToInt64() const
{
    if (Type_int64 == m_tp || Type_uint64 == m_tp)
        return m_nS64;
    return 0;
}

uint64_t Variant::ToUint64() const
{
    if (Type_int64 == m_tp || Type_uint64 == m_tp)
        return m_nU64;
    return 0;
}

double Variant::ToDouble() const
{
    if (Type_double == m_tp)
        return m_f64;
    return 0;
}

float Variant::ToFloat() const
{
    if (Type_float == m_tp)
        return m_f32;

    return 0;
}

bool Variant::ToBool() const
{
    if (Type_bool == m_tp)
        return m_bool;

    return false;
}

const StringList &Variant::ToStringList() const
{
    if (Type_StringList == m_tp)
        return *(StringList*)m_list;

    return sEmptyStrLs;
}

const char *Variant::GetBuff() const
{
    if (Type_buff == m_tp)
        return ((string*)m_list)->c_str();

    return NULL;
}

int Variant::GetBuffLength() const
{
    if (Type_buff == m_tp)
        return ((string*)m_list)->size();

    return 0;
}

bool Variant::IsNull() const
{
    return m_tp == Type_Unknow;
}

Variant &Variant::operator=(const Variant &oth)
{
    if (m_tp != oth.m_tp)
    {
        ensureDestruct();
        ensureConstruction(oth.m_tp);
    }

    switch (m_tp)
    {
    case Type_string:
    case Type_buff:
        *(string*)m_list = *(string*)oth.m_list; break;
    case Type_int32:
        m_nS = oth.m_nS; break;
    case Type_uint32:
        m_nU = oth.m_nU; break;
    case Type_int16:
        m_nS16 = oth.m_nS16; break;
    case Type_uint16:
        m_nU16 = oth.m_nU16; break;
    case Type_int8:
        m_nS8 = oth.m_nS8; break;
    case Type_uint8:
        m_nU8 = oth.m_nU8; break;
    case Type_int64:
        m_nS64 = oth.m_nS64; break;
    case Type_uint64:
        m_nU64 = oth.m_nU64; break;
    case Type_double:
        m_f64 = oth.m_f64; break;
    case Type_float:
        m_f32 = oth.m_f32; break;
    case Type_bool:
        m_bool = oth.m_bool; break;
    case Type_StringList:
        *(StringList*)m_list = *(StringList*)oth.m_list; break;
    case Type_ListI32:
        *(list<int32_t>*)m_list = *(list<int32_t>*)oth.m_list; break;
    case Type_ListU32:
        *(list<uint32_t>*)m_list = *(list<uint32_t>*)oth.m_list; break;
    case Type_ListI16:
        *(list<int16_t>*)m_list = *(list<int16_t>*)oth.m_list; break;
    case Type_ListU16:
        *(list<uint16_t>*)m_list = *(list<uint16_t>*)oth.m_list; break;
    case Type_ListI8:
        *(list<int8_t>*)m_list = *(list<int8_t>*)oth.m_list; break;
    case Type_ListU8:
        *(list<uint8_t>*)m_list = *(list<uint8_t>*)oth.m_list; break;
    case Type_ListI64:
        *(list<int64_t>*)m_list = *(list<int64_t>*)oth.m_list; break;
    case Type_ListU64:
        *(list<uint64_t>*)m_list = *(list<uint64_t>*)oth.m_list; break;
    case Type_ListF64:
        *(list<double>*)m_list = *(list<double>*)oth.m_list; break;
    case Type_ListF32:
        *(list<float>*)m_list = *(list<float>*)oth.m_list; break;
    default:
        break;
    }
    return *this;
}

void Variant::ensureConstruction(VariantType tp)
{
    switch (tp)
    {
    case Variant::Type_string:
    case Variant::Type_buff:
        m_list = new string;  break;
    case Variant::Type_StringList:
        m_list = new StringList;  break;
    case Variant::Type_ListI32:
        m_list = new list<int32_t>; break;
    case Variant::Type_ListU32:
        m_list = new list<uint32_t>; break;
    case Variant::Type_ListI16:
        m_list = new list<int16_t>; break;
    case Variant::Type_ListU16:
        m_list = new list<uint16_t>; break;
    case Variant::Type_ListI8:
        m_list = new list<int8_t>; break;
    case Variant::Type_ListU8:
        m_list = new list<uint8_t>; break;
    case Variant::Type_ListI64:
        m_list = new list<int64_t>; break;
    case Variant::Type_ListU64:
        m_list = new list<uint64_t>; break;
    case Variant::Type_ListF64:
        m_list = new list<double>; break;
    case Variant::Type_ListF32:
        m_list = new list<float>; break;
    default:
        break;
    }
    m_tp = tp;
}

void Variant::ensureDestruct()
{
    switch (m_tp)
    {
    case Variant::Type_string:
    case Variant::Type_buff:
        delete (string*)m_list; break;
    case Variant::Type_StringList:
        delete (StringList*)m_list;  break;
    case Variant::Type_ListI32:
        delete (list<int32_t>*)m_list; break;
    case Variant::Type_ListU32:
        delete (list<uint32_t>*)m_list; break;
    case Variant::Type_ListI16:
        delete (list<int16_t>*)m_list; break;
    case Variant::Type_ListU16:
        delete (list<uint16_t>*)m_list; break;
    case Variant::Type_ListI8:
        delete (list<int8_t>*)m_list; break;
    case Variant::Type_ListU8:
        delete (list<uint8_t>*)m_list; break;
    case Variant::Type_ListI64:
        delete (list<int64_t>*)m_list; break;
    case Variant::Type_ListU64:
        delete (list<uint64_t>*)m_list; break;
    case Variant::Type_ListF64:
        delete (list<double>*)m_list; break;
    case Variant::Type_ListF32:
        delete (list<float>*)m_list; break;
    default:
        break;
    }
}

bool Variant::isBasicalType() const
{
    return m_tp == Type_buff || (m_tp >= Type_string && m_tp < Type_bool);
}

Variant::VariantType Variant::elementListType() const
{
    switch (m_tp)
    {
    case Type_string:
    case Type_buff:
        return Type_StringList;
    case Type_int32:
        return Type_ListI32;
    case Type_uint32:
        return Type_ListU32;
    case Type_int16:
        return Type_ListI16;
    case Type_uint16:
        return Type_ListU16;
    case Type_int8:
        return Type_ListI8;
    case Type_uint8:
        return Type_ListU8;
    case Type_int64:
        return Type_ListI64;
    case Type_uint64:
        return Type_ListU64;
    case Type_double:
        return Type_ListF64;
    case Type_float:
        return Type_ListF32;
    default:
        break;
    }
    return Type_Unknow;
}
