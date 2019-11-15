#include "DBExecItem.h"

#if !defined _WIN32 && !defined _WIN64
#include <mysql/mysql.h>
#else
#include <mysql.h>
#endif

#include "VGDBManager.h"
#include <tinyxml.h>
#include <string.h>

///////////////////////////////////////////////////////////////////////////////////////
//FiledVal
///////////////////////////////////////////////////////////////////////////////////////
FiledVal::FiledVal(int tp, const std::string &name, int len)
: m_type(tp), m_bEmpty(true), m_bStatic(false), m_buff(NULL)
, m_lenMax(0), m_len(0), m_name(name), m_condition("=")
{
    InitBuff(len);
}

FiledVal::FiledVal(VGTableField *fild, bool bOth)
: m_type(-1), m_bEmpty(true), m_bStatic(false), m_buff(NULL)
, m_lenMax(0), m_len(0), m_condition("=")
{
    if (fild)
    {
        m_type = fild->GetType();
        m_name = fild->GetName(bOth);
        InitBuff(fild->GetLength());
    }
}

FiledVal::~FiledVal()
{
    delete m_buff;
}

void FiledVal::SetFieldName(const std::string &name)
{
    m_name = name;
}

const std::string & FiledVal::GetFieldName() const
{
    return m_name;
}

void FiledVal::SetParam(const string &param, bool val, bool bStatic)
{
    if(param.length() > 0)
    {
        if(m_buff)
        {
            delete m_buff;
            m_buff = NULL;
        }
        m_len = 0;
        if (val)
            m_param = string("\'") + param + "\'";
        else
            m_param = param;

        m_bEmpty = false;
        m_bStatic = bStatic;
    }
}

const string & FiledVal::GetParam() const
{
    return m_param;
}

void FiledVal::InitBuff(unsigned len, const void *buf)
{
    if (len > m_lenMax)
    {
        m_lenMax = len;
        delete m_buff;
        m_buff = new char[len];
    }

    m_param = string();
    m_len = len;
    if (buf && m_buff)
        memcpy(m_buff, buf, len);

    m_bEmpty = false;
}

string FiledVal::ToConditionString()const
{
    return GetFieldName() + m_condition + (IsStaticParam() ? GetParam() : "?");
}

unsigned FiledVal::GetMaxLen() const
{
    return m_buff ? m_lenMax : 0;
}

unsigned FiledVal::GetValidLen() const
{
    return m_buff ? m_len : 0;
}

void *FiledVal::GetBuff() const
{
    return m_buff;
}

int FiledVal::GetType() const
{
    return m_type;
}

unsigned long &FiledVal::ReadLength()
{
    return m_len;
}

bool FiledVal::IsStaticParam() const
{
    return m_len<=0 && m_param.length()>0;
}

bool FiledVal::IsEmpty() const
{
    return m_bEmpty;
}

void FiledVal::SetEmpty()
{
    if(!IsStaticParam() || !m_bStatic)
        m_bEmpty = true;
}

void FiledVal::parse(const TiXmlElement *e, ExecutItem *sql)
{
    if (!e || !sql)
        return;

    int tp = transToType(e->Attribute("type"));
    if (tp == ExecutItem::AutoIncrement)
    {
        if(!sql->GetIncrement())
            sql->SetIncrement(new FiledVal(-1, "", sizeof(int64_t)));

        return;
    }
    const char *name = e->Attribute("name");
    if (!name || tp > ExecutItem::Condition || tp < ExecutItem::Read)
        return;

    if (FiledVal *item = parseFiled(*e, sql->ExecutTables().size() > 1))
    {
        if (const char *tmp = e->Attribute("condition"))
            item->m_condition = tmp;
        sql->AddItem(item, tp);
    }
}

int FiledVal::transToType(const char *pro)
{
    int ret = -1;

    if (!pro)
        return ret;

    if (0 == strnicmp(pro, "Read", 5))
        return ExecutItem::Read;
    else if (0 == strnicmp(pro, "Write", 6))
        return ExecutItem::Write;
    else if (0 == strnicmp(pro, "Condition", 10))
        return ExecutItem::Condition;
    else if (0 == strnicmp(pro, "AutoIncrement", 14))
        return ExecutItem::AutoIncrement;

    return ret;
}

FiledVal *FiledVal::parseFiled(const TiXmlElement &e, bool oth)
{
    const char *name = e.Attribute("name");
    if (const char *tmp = e.Attribute("ref"))
    {
        if (VGTable *tb = VGDBManager::GetTableByName(tmp))
        {
            if (VGTableField *fd = tb->FindFieldByName(name))
                return new FiledVal(fd, oth);
        }
    }
    else if (const char *tmpTp = e.Attribute("sqlTp"))
    {
        int tpSql = VGTableField::GetTypeByName(tmpTp);
        if (tpSql)
             return new FiledVal(VGTableField::TransSqlType(tpSql), name, VGTableField::TransBuffLength(tpSql));
    }
    else
    {
        const char *tmp = e.Attribute("param");
        FiledVal *item = new FiledVal(-1, name, 0);
        if (tmp)
            item->SetParam(tmp, false, true);
        else if (const char *val = e.Attribute("paramVal"))
            item->SetParam(val, true, true);
        return item;
    }

    return NULL;
}
///////////////////////////////////////////////////////////////////////////////////////
//ExecutItem
///////////////////////////////////////////////////////////////////////////////////////
ExecutItem::ExecutItem(ExecutType tp, const std::string &name)
: m_type(tp), m_name(name), m_condition(" and "), m_autoIncrement(NULL)
, m_bHasForeignRefTable(false), m_bRef(false)
{
}

ExecutItem::~ExecutItem()
{
    for (FiledVal *itr : m_itemsRead)
    {
        delete itr;
    }

    for (FiledVal *itr : m_itemsWrite)
    {
        delete itr;
    }
    delete m_autoIncrement;
}

ExecutItem::ExecutType ExecutItem::GetType() const
{
    return m_type;
}

const std::string & ExecutItem::GetName() const
{
    return m_name;
}

const StringList &ExecutItem::ExecutTables() const
{
    return m_tables;
}

void ExecutItem::SetExecutTables(const StringList &tbs)
{
    for (const std::string &itr : tbs)
    {
        AddExecutTable(itr);
    }
}

void ExecutItem::AddExecutTable(const std::string &t)
{
    for (const std::string &itr:m_tables)
    {
        if (itr == t)
            return;
    }
    if (!m_bHasForeignRefTable)
    {
        VGTable *tb = VGDBManager::GetTableByName(t);
        if (tb && tb->IsForeignRef())
            m_bHasForeignRefTable = true;
    }
    m_tables.push_back(t);
}

void ExecutItem::AddItem(FiledVal *item, int tp)
{
    if (!item)
        return;

    if (tp == ExecutItem::Read)
        m_itemsRead.push_back(item);
    else if (tp == ExecutItem::Write)
        m_itemsWrite.push_back(item);
    else if (tp == ExecutItem::Condition)
        _addCondition(item);
}

void ExecutItem::SetIncrement(FiledVal *item)
{
    if (m_autoIncrement && item)
        return;

    if (m_autoIncrement)
        delete m_autoIncrement;
    
    m_autoIncrement = item;
}

FiledVal *ExecutItem::GetReadItem(const string &name)const
{
    for (FiledVal *itr : m_itemsRead)
    {
        if (name == itr->GetFieldName())
            return itr;
    }
    return NULL;
}

FiledVal *ExecutItem::GetWriteItem(const string &name) const
{
    for (FiledVal *itr : m_itemsWrite)
    {
        if (name == itr->GetFieldName())
            return itr;
    }

    return NULL;
}

FiledVal *ExecutItem::GetConditionItem(const string &name) const
{
    for (FiledVal *itr : m_itemsCondition)
    {
        if (name == itr->GetFieldName())
            return itr;
    }
    return NULL;
}

FiledVal *ExecutItem::GetIncrement() const
{
    return m_autoIncrement;
}

bool ExecutItem::IsValid() const
{
    return (m_type > Unknown && m_type <= Select && m_tables.size() > 0);
}

 FiledVal *ExecutItem::GetItem(const string &name, int tp) const
{
     if (ExecutItem::Read == tp)
         return GetReadItem(name);
     else if (ExecutItem::Write == tp)
         return GetWriteItem(name);
     else if (ExecutItem::Condition == tp)
         return GetConditionItem(name);

     return NULL;
}

MYSQL_BIND *ExecutItem::GetParamBinds()
{
    int count = 0;
    for (FiledVal *itr : m_itemsWrite)
    {
        if (!itr->IsEmpty() && !itr->IsStaticParam())
            count++;
    }
    for (FiledVal *itr : m_itemsCondition)
    {
        if (!itr->IsEmpty() && !itr->IsStaticParam())
            count++;
    }
    if (count > 0)
    {
        MYSQL_BIND *paramBinds = new MYSQL_BIND[count];
        memset(paramBinds, 0, count * sizeof(MYSQL_BIND));
        return paramBinds;
    }
    return NULL;
}

int ExecutItem::CountRead() const
{
    return m_itemsRead.size();
}

string ExecutItem::GetSqlString(MYSQL_BIND *paramBinds) const
{
    int pos = 0;
    switch (m_type)
    {
    case ExecutItem::Insert:
        return _toInsert(paramBinds, pos);
    case ExecutItem::Replace:
        return _toReplace(paramBinds, pos);
    case ExecutItem::Delete:
        return _toDelete(paramBinds, pos);
    case ExecutItem::Update:
        return _toUpdate(paramBinds, pos);
    case ExecutItem::Select:
        return _toSelect(paramBinds, pos);
    default:
        break;
    }
    return string();
}

void ExecutItem::ClearData()
{
    for (FiledVal *itr : m_itemsRead)
    {
        itr->InitBuff(0);
    }

    for (FiledVal *itr : m_itemsWrite)
    {
        itr->SetEmpty();
    }

    for (FiledVal *itr : m_itemsCondition)
    {
        itr->SetEmpty();
    }
}

MYSQL_BIND *ExecutItem::TransformRead()
{
    unsigned sz = m_itemsRead.size();
    if (sz < 0)
        return NULL;

    MYSQL_BIND *binds = new MYSQL_BIND[sz];
    if (!binds)
        return NULL;
    memset(binds, 0, sz * sizeof(MYSQL_BIND));
    int pos = 0;
    for (FiledVal *item : m_itemsRead)
    {
        ExecutItem::transformBind(item, binds[pos++], true);
    }
    return binds;
}

bool ExecutItem::HasForeignRefTable() const
{
    return m_bHasForeignRefTable;
}

void ExecutItem::SetRef(bool b)
{
    m_bRef = b;
}

bool ExecutItem::IsRef() const
{
    return m_bRef;
}

void ExecutItem::transformBind(FiledVal *item, MYSQL_BIND &bind, bool bRead)
{
    if (item)
    {
        bind.buffer_type = (enum_field_types)item->GetType();
        bind.buffer = item->GetBuff();
        bind.buffer_length = item->GetValidLen();
        if(bRead)
        {
            bind.length = (unsigned long *)&item->ReadLength();
            bind.buffer_length = item->GetMaxLen();
        }
    }
}

ExecutItem *ExecutItem::parse(const TiXmlElement *e)
{
    if (!e)
        return NULL;

    const char *tmp = e->Attribute("name");
    string name = tmp ? tmp : string();
    tmp = e->Attribute("table");
    string table = tmp ? tmp : string();
    ExecutType tp = transToSqlType(e->Attribute("type"));
    if (name.length() < 1 || table.length() < 0 || tp==ExecutItem::Unknown)
        return NULL;

    ExecutItem *sql = new ExecutItem(tp, name);
    if (!sql)
        return sql;

    if (const char *tmpC = e->Attribute("condition"))
        sql->m_condition = string(" ") + tmpC + " ";

    sql->SetExecutTables(VGDBManager::SplitString(table, ";"));
    sql->_parseItems(e);

    return sql;
}

void ExecutItem::_parseItems(const TiXmlElement *e)
{
    if (!e)
        return;
    
    const char *tmp = e->Attribute("all");
    if (tmp && VGDBManager::str2int(tmp) != 0)
    {
        for (const string &itr : m_tables)
        {
            if (VGTable *tb = VGDBManager::GetTableByName(itr.c_str()))
            {
                for (VGTableField *f : tb->Fields())
                {
                    int tp = m_type == ExecutItem::Select ? ExecutItem::Read : ExecutItem::Write;
                    AddItem(new FiledVal(f, m_tables.size() > 1), tp);
                }
            }
        }
    }
    else
    {
        const TiXmlNode *node = e->FirstChild("item");
        while (node)
        {
            FiledVal::parse(node->ToElement(), this);
            node = node->NextSibling("item");
        }
    }

}

void ExecutItem::_addCondition(FiledVal *item)
{
    string tmp = item ? item->GetFieldName() : string();
    for (FiledVal *itr : m_itemsCondition)
    {
        if (itr == item || itr->GetFieldName() == tmp)
            return;
    }
    m_itemsCondition.push_back(item);
}

string ExecutItem::_getTablesString() const
{
    string ret;
    for (const string &table : m_tables)
    {
        if (ret.length() > 0)
            ret += ",";
        ret += " " + table;
    }
    return ret;
}

string ExecutItem::_toInsert(MYSQL_BIND *binds, int &pos)const
{
    string sql = "insert";
    sql += _getTablesString();
    sql += _genWrite(binds, pos) + _conditionsString(binds, pos);

    return sql;
}

std::string ExecutItem::_toReplace(MYSQL_BIND *binds, int &pos) const
{
    string sql = "replace";
    sql += _getTablesString();
    sql +=_genWrite(binds, pos) + _conditionsString(binds, pos);

    return sql;
}

std::string ExecutItem::_toDelete(MYSQL_BIND *bind, int &pos) const
{
    string sql = _deleteBegin();
    sql += _getTablesString();
    sql += _conditionsString(bind, pos);
    return sql;
}

string ExecutItem::_toUpdate(MYSQL_BIND *bind, int &pos) const
{
    string sql = "update";
    sql += _getTablesString();

    string updates;
    for (FiledVal *item : m_itemsWrite)
    {
        if (item->IsEmpty())
            continue;

        string field = item->GetFieldName();
        if (field.length() < 1)
            return string();

        bool bStatic = item->IsStaticParam();  
        if (updates.length() < 1)
        {
            if (bStatic)
                updates = string(" set ") + field + "=" + item->GetParam();
            else
                updates = string(" set ") + field + "=?";
        }
        else
        {
            if (bStatic)
                updates += string(", ") + field + "=" + item->GetParam();
            else
                updates += string(", ") + field + "=?";
        }
        if(!bStatic && bind)
            transformBind(item, bind[pos++]);
    }
    sql += updates;
    sql += _conditionsString(bind, pos);
    return sql;
}

std::string ExecutItem::_toSelect(MYSQL_BIND *param, int &pos) const
{
    string fields;
    for (FiledVal *item : m_itemsRead)
    {
        if (fields.length() > 0)
            fields += ",";
        fields += " " + item->GetFieldName();
    }
    string sql = string("select") + fields+ " from" + _getTablesString() + _conditionsString(param, pos);
    return sql;
}

string ExecutItem::_conditionsString(MYSQL_BIND *bind, int &pos) const
{
    string conditions;
    for (FiledVal *itr : m_itemsCondition)
    {
        if(itr->IsEmpty())
            continue;

        string tmp = itr->ToConditionString();
        if (conditions.length() < 1)
            conditions = string(" where ")+tmp;
        else
            conditions += m_condition + tmp;

        if(bind && !itr->IsStaticParam())
            transformBind(itr, bind[pos++]);
    }
    return conditions;
}

std::string ExecutItem::_deleteBegin() const
{
    if(m_tables.size()<2)
        return "delete from";

    string ret;
    for (const string &itr : m_tables)
    {
        if (ret.length() > 0)
            ret += " ," + itr + ".*";
        else
            ret = string("delete ") + itr + ".*";
    }
    return ret + " from";
}

std::string ExecutItem::_genWrite(MYSQL_BIND *binds, int &pos)const
{
    string fields;
    string values;
    for (FiledVal *item : m_itemsWrite)
    {
        if (item->IsEmpty())
            continue;

        bool bStatic = item->IsStaticParam();
        string field = item->GetFieldName();
        if (field.length() < 1)
            return string();

        if (fields.length() < 1)
        {
            fields = "(";
            fields += field;
            if (bStatic)
                values = string(" value(") + item->GetParam();
            else
                values = " value(?";
        }
        else
        {
            fields += ",";
            fields += field;
            if (bStatic)
                values += string(", ") + item->GetParam();
            else
                values += ", ?";
        }
        if (!bStatic && binds)
            transformBind(item, binds[pos++]);
    }
    fields += ")";
    values += ")";
    return fields + values;
}

ExecutItem::ExecutType ExecutItem::transToSqlType(const char *pro)
{
    if (!pro)
        return ExecutItem::Unknown;

    if (0 == strnicmp(pro, "Insert", 7))
        return ExecutItem::Insert;
    if (0 == strnicmp(pro, "Replace", 8))
        return ExecutItem::Replace;
    else if (0 == strnicmp(pro, "Delete", 7))
        return ExecutItem::Delete;
    else if (0 == strnicmp(pro, "Update", 7))
        return ExecutItem::Update;
    else if (0 == strnicmp(pro, "Select", 7))
        return ExecutItem::Select;

    return ExecutItem::Unknown;
}
