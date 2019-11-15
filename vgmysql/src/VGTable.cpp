#include "VGTable.h"
#if !defined _WIN32 && !defined _WIN64
#include <mysql/mysql.h>
#else
#include <mysql.h>
#endif
#include <map>
#include <string.h>

#include "VGDBManager.h"
#include <tinyxml.h>

////////////////////////////////////////////////////////////////////////////////
//VGConstraint
////////////////////////////////////////////////////////////////////////////////
class VGForeignKey
{
public:
    const string &GetGetForeignTable()const
    {
        return m_tableForeign;
    }
    string ToString() const
    {
        if (!IsValid())
            return string();

        string foreign = GetForeignStr();
        string ret;
        ret.resize(40 + foreign.length() + m_keyLocal.length() * 2);
        sprintf(&ret.at(0), "constraint %s foreign key(%s) references %s"
            , m_keyLocal.c_str(), m_keyLocal.c_str(), foreign.c_str());

        return string(ret.c_str());
    }
public:
    static VGForeignKey *ParseForeignKey(const TiXmlElement &e, VGTable *parent)
    {
        const char *tmp = e.Attribute("name");
        if (!tmp || !parent)
            return NULL;

        string local = tmp;
        tmp = e.Attribute("foreign");
        if (!tmp || !parent->FindFieldByName(local))
            return NULL;

        string foreign = tmp;
        tmp = e.Attribute("ref");
        if (!tmp)
            return NULL;

        if (VGTable *tb = VGDBManager::GetTableByName(tmp))
        {
            if (!tb->FindFieldByName(foreign))
                return NULL;

            if(VGForeignKey *ret = new VGForeignKey())
            {
                ret->m_bValid = true;
                ret->m_keyForeign = foreign;
                ret->m_keyLocal = local;
                ret->m_tableForeign = tmp;
                return ret;
            }
        }
        return NULL;
    }
protected:
    VGForeignKey() :m_bValid(false){}
    bool IsValid()const
    {
        return m_bValid;
    }
    string GetForeignStr()const
    {
        return m_tableForeign + "(" + m_keyForeign + ")";
    }
private:
    bool    m_bValid;
    string  m_keyForeign;
    string  m_keyLocal;
    string  m_tableForeign;
};
////////////////////////////////////////////////////////////////////////////////
//VGTableField
////////////////////////////////////////////////////////////////////////////////
VGTableField::VGTableField(VGTable *tb, const string &n)
: m_table(tb), m_type(0), m_name(n)
{
}

string VGTableField::GetName(bool bTable) const
{
    if(bTable && m_table)
        return m_table->GetName()+"."+m_name;

    return m_name;
}

void VGTableField::SetName(const string &n)
{
    m_name = n;
}

void VGTableField::SetConstrains(const string &n)
{
    m_constraints.clear();
    
    for (const string &itr : VGDBManager::SplitString(n, ";"))
    {
        if (itr.length()>0)
            m_constraints.push_back(itr);
    }
}

const StringList & VGTableField::GetConstrains() const
{
    return m_constraints;
}

const string &VGTableField::GetTypeName()const
{
    return m_typeName;
}

int VGTableField::GetTypeByName(const string &type)
{
    int ret = 0;
    if (0 == strnicmp(type.c_str(), "BIT", 3))
        ret = _parseBits(type.substr(3));
    else if (0 == strnicmp(type.c_str(), "TINYINT", 8))
        ret = (1 << 16) | MYSQL_TYPE_TINY;
    else if (0 == strnicmp(type.c_str(), "SMALLINT", 9))
        ret = (2 << 16) | MYSQL_TYPE_SHORT;
    else if (0 == strnicmp(type.c_str(), "INTEGER", 8) || 0 == strnicmp(type.c_str(), "INT", 4))
        ret = (4 << 16) | MYSQL_TYPE_LONG;
    else if (0 == strnicmp(type.c_str(), "BIGINT", 8))
        ret = (8 << 16) | MYSQL_TYPE_LONGLONG;
    else if (0 == strnicmp(type.c_str(), "FLOAT", 6))
        ret = (4 << 16) | MYSQL_TYPE_FLOAT;
    else if (0 == strnicmp(type.c_str(), "DOUBLE", 6))
        ret = (8 << 16) | MYSQL_TYPE_DOUBLE;
    else if (0 == strnicmp(type.c_str(), "DATE", 5))
        ret = (4 << 16) | MYSQL_TYPE_DATE;
    else if (0 == strnicmp(type.c_str(), "TIME", 5))
        ret = (4 << 16) | MYSQL_TYPE_TIME;
    else if (0 == strnicmp(type.c_str(), "DATETIME", 9))
        ret = (8 << 16) | MYSQL_TYPE_DATETIME;
    else if (0 == strnicmp(type.c_str(), "TIMESTAMP", 10))
        ret = (4 << 16) | MYSQL_TYPE_DATETIME;
    else if (0 == strnicmp(type.c_str(), "CHAR", 4))
        ret = _parseChar(type.substr(4));
    else if (0 == strnicmp(type.c_str(), "VARCHAR", 7))
        ret = _parseVarChar(type.substr(7));
    else if (0 == strnicmp(type.c_str(), "BINARY", 6))
        ret = _parseBinary(type.substr(6));
    else if (0 == strnicmp(type.c_str(), "VARBINARY", 9))
        ret = _parseVarBinary(type.substr(9));
    else if (0 == strnicmp(type.c_str(), "BLOB", 8))
        ret = (1 << 16) | MYSQL_TYPE_BLOB;
    else if (0 == strnicmp(type.c_str(), "TEXT", 5))
        ret = (1 << 16) | MYSQL_TYPE_STRING;
    return ret;
}

int VGTableField::TransSqlType(int t)
{
    return t & 0xff;
}

int VGTableField::TransBuffLength(int t)
{
    return (t >> 16) & 0xffff;
}

int VGTableField::GetType()
{
    return TransSqlType(m_type);
}

int VGTableField::GetLength() const
{
    return TransBuffLength(m_type);
}

bool VGTableField::IsValid() const
{
    return m_name.length() > 0 && GetLength() > 0 && m_table;
}

string VGTableField::ToString() const
{
    if (!IsValid())
        return string();

    string ret = m_name + " " + m_typeName;
    for (const string &itr : m_constraints)
    {
        ret += " " + itr;
    }
    return ret;
}

VGTableField *VGTableField::ParseFiled(const TiXmlElement &e, VGTable *parent)
{
    const char *tmp = e.Attribute("name");
    if (!tmp || !parent)
        return NULL;

    if (VGTableField *fd = new VGTableField(parent, tmp))
    {
        if (const char *s = e.Attribute("type"))
        {
            fd->m_type = GetTypeByName(s);
            if (fd->m_type != 0)
                fd->m_typeName = s;
        }
        if (const char *s = e.Attribute("constraint"))
            fd->SetConstrains(s);

        if (fd->IsValid())
            return fd;
        else
            delete fd;
    }

    return NULL;
}

int VGTableField::_parseBits(const string &n)
{
    if (n.length() < 3 || n.at(0) != '(' || *(--n.end()) != ')')
        return 0;

    int nTmp = VGDBManager::str2int(n.substr(1, n.length() - 2));
    if (nTmp > 0 && nTmp < 256)
        return (nTmp << 16) | MYSQL_TYPE_BIT;

    return 0;
}

int VGTableField::_parseChar(const string &n)
{
    if (n.length() < 3 || n.at(0) != '(' || *(--n.end()) != ')')
        return 0;

    int nTmp = VGDBManager::str2int(n.substr(1, n.length()-2));
    if(nTmp>0 && nTmp<256)
        return (nTmp << 16) | MYSQL_TYPE_STRING;

    return 0;
}

int VGTableField::_parseVarChar(const string &n)
{
    if (n.length() < 3 || n.at(0) != '(' || *(--n.end()) != ')')
        return 0;

    int nTmp = VGDBManager::str2int(n.substr(1, n.length() - 2));
    nTmp = (nTmp + 7) / 8;
    if (nTmp > 0 && nTmp<0xffff)
        return (nTmp << 16) | MYSQL_TYPE_VAR_STRING;

    return 0;
}

int VGTableField::_parseBinary(const string &n)
{
    if (n.length() < 3 || n.at(0) != '(' || *(--n.end()) != ')')
        return 0;

    int nTmp = VGDBManager::str2int(n.substr(1, n.length() - 2));
    if (nTmp > 0 && nTmp<256)
        return (nTmp << 16) | MYSQL_TYPE_VAR_STRING;
    return 0;
}

int VGTableField::_parseVarBinary(const string &n)
{
    if (n.length() < 3 || n.at(0) != '(' || *(--n.end()) != ')')
        return 0;

    int nTmp = VGDBManager::str2int(n.substr(1, n.length() - 2));
    if (nTmp > 0 && nTmp<0xffff)
        return (nTmp << 16) | MYSQL_TYPE_VAR_STRING;
    return 0;
}
////////////////////////////////////////////////////////////////////////////////
//VGTableField
////////////////////////////////////////////////////////////////////////////////
VGTable::VGTable(const string &n/*=string()*/) : m_name(n)
, m_bForeignRef(false)
{
}

void VGTable::AddForeign(VGForeignKey *f)
{
    if (f)
    {
        if (VGTable *tb = VGDBManager::GetTableByName(f->GetGetForeignTable()))
            tb->m_bForeignRef = true;

        m_foreigns.push_back(f);
    }
}

const string &VGTable::GetName() const
{
    return m_name;
}

const list<VGTableField *> &VGTable::Fields() const
{
    return m_fields;
}

VGTableField *VGTable::FindFieldByName(const string &n)
{
    for (VGTableField *itr : m_fields)
    {
        if (n == itr->GetName())
            return itr;
    }
    return NULL;
}

string VGTable::ToCreateSQL() const
{
    string ret;
    for (VGTableField *itr : m_fields)
    {
        ret += ret.length() > 0 ? ", ":"(";
        ret += itr->ToString();
    }

    for (VGForeignKey *itr : m_foreigns)
    {
        ret += ", " + itr->ToString();
    }
    ret += ")";

    return string("CREATE TABLE IF NOT EXISTS ") + m_name + ret;
}

void VGTable::AddField(VGTableField *fd)
{
    if (!fd || !fd->IsValid())
        return;

    for (VGTableField *itr : m_fields)
    {
        if (fd == itr || fd->GetName() == itr->GetName())
            return;
    }
    m_fields.push_back(fd);
}

bool VGTable::IsForeignRef() const
{
    return m_bForeignRef;
}

VGTable *VGTable::ParseTable(const TiXmlElement &e)
{
    const char *tmp = e.Attribute("name");
    string name = tmp ? tmp : string();
    if (name.length() < 1)
        return NULL;

    VGTable *tb = new VGTable(name);
    const TiXmlNode *node = e.FirstChild("field");
    while (node)
    {
        if(VGTableField *fd = VGTableField::ParseFiled(*node->ToElement(), tb))
            tb->AddField(fd);

        node = node->NextSibling("field");
    }

    node = e.FirstChild("foreign");
    while (node)
    {
        if(VGForeignKey *fk = VGForeignKey::ParseForeignKey(*node->ToElement(), tb))
            tb->AddForeign(fk);

        node = node->NextSibling("foreign");
    }
    return tb;
}
