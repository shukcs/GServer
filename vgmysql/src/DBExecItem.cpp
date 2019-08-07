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
//FiledValueItem
///////////////////////////////////////////////////////////////////////////////////////
FiledValueItem::FiledValueItem(int tp, const std::string &name, int len)
: m_type(tp), m_bEmpty(true), m_bStatic(false), m_buff(NULL)
, m_lenMax(0), m_len(0), m_name(name)
{
    InitBuff(len);
}

FiledValueItem::FiledValueItem(VGTableField *fild, bool bOth)
: m_type(-1), m_bEmpty(true), m_bStatic(false), m_buff(NULL)
, m_lenMax(0), m_len(0)
{
    if (fild)
    {
        m_type = fild->GetType();
        m_name = fild->GetName(bOth);
        InitBuff(fild->GetLength());
    }
}

FiledValueItem::~FiledValueItem()
{
    delete m_buff;
}

void FiledValueItem::SetFieldName(const std::string &name)
{
    m_name = name;
}

const std::string & FiledValueItem::GetFieldName() const
{
    return m_name;
}

void FiledValueItem::SetParam(const string &param, bool val, bool bStatic)
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

const string & FiledValueItem::GetParam() const
{
    return m_param;
}

void FiledValueItem::InitBuff(unsigned len, const void *buf)
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

unsigned FiledValueItem::GetMaxLen() const
{
    return m_buff ? m_lenMax : 0;
}

unsigned FiledValueItem::GetValidLen() const
{
    return m_buff ? m_len : 0;
}

void *FiledValueItem::GetBuff() const
{
    return m_buff;
}

int FiledValueItem::GetType() const
{
    return m_type;
}

unsigned long &FiledValueItem::ReadLength()
{
    return m_len;
}

bool FiledValueItem::IsStaticParam() const
{
    return m_len<=0 && m_param.length()>0;
}

bool FiledValueItem::IsEmpty() const
{
    return m_bEmpty;
}

void FiledValueItem::SetEmpty()
{
    if(!IsStaticParam() || !m_bStatic)
        m_bEmpty = true;
}

void FiledValueItem::parse(TiXmlElement *e, ExecutItem *sql)
{
    if (!e || !sql)
        return;

    int tp = _transToType(e->Attribute("type"));
    if (tp == ExecutItem::AutoIncrement)
    {
        if(!sql->GetIncrement())
            sql->SetIncrement(new FiledValueItem(-1, "", sizeof(int64_t)));

        return;
    }
    const char *name = e->Attribute("name");
    if (!name || tp > ExecutItem::Condition || tp < ExecutItem::Read)
        return;

    const char *tmp = e->Attribute("ref");
    if (tmp)
    {
        VGTable *tb = VGDBManager::Instance().GetTableByName(tmp);
        if (tb)
        {
            if (VGTableField *fd = tb->FindFieldByName(name))
                sql->AddItem(new FiledValueItem(fd, sql->ExecutTables().size() > 1), tp);
        }
    }
    else if (const char *tmpTp = e->Attribute("sqlTp"))
    {
        int tpSql = tmpTp ? VGDBManager::str2int(tmpTp) : -1;
        tmpTp = e->Attribute("length");
        int len = tmpTp ? VGDBManager::str2int(tmpTp) : 0;
        sql->AddItem(new FiledValueItem(tpSql, name, len), tp);
    }
    else
    {
        tmp = e->Attribute("param");
        FiledValueItem *vf = new FiledValueItem(-1, name, 0);
        if (tmp)
            vf->SetParam(tmp, false, true);
        else if (tmp = e->Attribute("paramVal"))
            vf->SetParam(tmp, true, true);

        sql->AddItem(vf, tp);
    }
}

int FiledValueItem::_transToType(const char *pro)
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
///////////////////////////////////////////////////////////////////////////////////////
//ExecutItem
///////////////////////////////////////////////////////////////////////////////////////
ExecutItem::ExecutItem(ExecutType tp, const std::string &name)
: m_type(tp), m_name(name), m_autoIncrement(NULL)
{
}

ExecutItem::~ExecutItem()
{
    for (FiledValueItem *itr : m_itemsRead)
    {
        delete itr;
    }

    for (FiledValueItem *itr : m_itemsWrite)
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

void ExecutItem::AddExecutTable(const std::string &t)
{
    for (const std::string &itr:m_tables)
    {
        if (itr == t)
            return;
    }
    m_tables.push_back(t);
}

void ExecutItem::AddItem(FiledValueItem *item, int tp)
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

void ExecutItem::SetIncrement(FiledValueItem *item)
{
    if (m_autoIncrement && item)
        return;

    if (m_autoIncrement)
        delete m_autoIncrement;
    
    m_autoIncrement = item;
}

FiledValueItem *ExecutItem::GetReadItem(const string &name)const
{
    for (FiledValueItem *itr : m_itemsRead)
    {
        if (name == itr->GetFieldName())
            return itr;
    }
    return NULL;
}

FiledValueItem *ExecutItem::GetWriteItem(const string &name) const
{
    for (FiledValueItem *itr : m_itemsWrite)
    {
        if (name == itr->GetFieldName())
            return itr;
    }

    return NULL;
}

FiledValueItem *ExecutItem::GetConditionItem(const string &name) const
{
    for (FiledValueItem *itr : m_itemsCondition)
    {
        if (name == itr->GetFieldName())
            return itr;
    }
    return NULL;
}

FiledValueItem *ExecutItem::GetIncrement() const
{
    return m_autoIncrement;
}

bool ExecutItem::IsValid() const
{
    return (m_type > Unknown && m_type <= Select && m_tables.size() > 0);
}

 FiledValueItem *ExecutItem::GetItem(const string &name, int tp) const
{
     if (ExecutItem::Read == tp)
         return GetReadItem(name);
     else if (ExecutItem::Write == tp)
         return GetWriteItem(name);
     else if (ExecutItem::Condition == tp)
         return GetConditionItem(name);

     return NULL;
}

int ExecutItem::CountParam()const
{
    int ret = 0;
    for (FiledValueItem *itr : m_itemsWrite)
    {
        if (!itr->IsStaticParam())
            ret++;
    }
    for (FiledValueItem *itr : m_itemsCondition)
    {
        if (!itr->IsStaticParam())
            ret++;
    }
    return ret;
}

int ExecutItem::CountRead() const
{
    return m_itemsRead.size();
}

string ExecutItem::GetSqlString(MYSQL_BIND *bind, int &pos) const
{
    switch (m_type)
    {
    case ExecutItem::Insert:
        return _toInsert(bind, pos);
    case ExecutItem::Delete:
        return _toDelete(bind, pos);
    case ExecutItem::Update:
        return _toUpdate(bind, pos);
    case ExecutItem::Select:
        return _toSelect(bind, pos);
    default:
        break;
    }
    return string();
}

void ExecutItem::ClearData()
{
    for (FiledValueItem *itr : m_itemsRead)
    {
        itr->InitBuff(0);
    }

    for (FiledValueItem *itr : m_itemsWrite)
    {
        itr->SetEmpty();
    }

    for (FiledValueItem *itr : m_itemsCondition)
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
    for (FiledValueItem *item : m_itemsRead)
    {
        ExecutItem::transformBind(item, binds[pos++], true);
    }
    return binds;
}

void ExecutItem::transformBind(FiledValueItem *item, MYSQL_BIND &bind, bool bRead)
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

ExecutItem *ExecutItem::parse(TiXmlElement *e)
{
    if (!e)
        return NULL;

    const char *tmp = e->Attribute("name");
    string name = tmp ? tmp : string();
    tmp = e->Attribute("table");
    string table = tmp ? tmp : string();
    ExecutType tp = _transToType(e->Attribute("type"));
    if (name.length() < 1 || table.length() < 0 || tp==ExecutItem::Unknown)
        return NULL;

    ExecutItem *sql = new ExecutItem(tp, name);
    if (!sql)
        return sql;
    tmp = e->Attribute("all");
    bool bAll = tmp && VGDBManager::str2int(tmp) != 0;
    sql->m_tables = VGDBManager::SplitString(table, ";");
    for (const string &itr : sql->m_tables)
    {
        if (!bAll)
            continue;

        if (VGTable *tb = VGDBManager::Instance().GetTableByName(itr.c_str()))
        {
            for (VGTableField *f : tb->Fields())
            {
                sql->AddItem(new FiledValueItem(f, sql->m_tables.size() > 1), tp == ExecutItem::Select ? ExecutItem::Read : ExecutItem::Write);
            }
        }
    }
    if (!bAll)
        sql->_parseItems(e);

    return sql;
}

void ExecutItem::_parseItems(TiXmlElement *e)
{
    if (!e)
        return;

    TiXmlNode *node = e->FirstChild("item");
    while (node)
    {
        FiledValueItem::parse(node->ToElement(), this);
        node = node->NextSibling("item");
    }
}

void ExecutItem::_addCondition(FiledValueItem *item)
{
    string tmp = item ? item->GetFieldName() : string();
    for (FiledValueItem *itr : m_itemsCondition)
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

    string fields;
    string values;
    for (FiledValueItem *item : m_itemsWrite)
    {
        if(item->IsEmpty())
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
        if(!bStatic && binds)
            transformBind(item, binds[pos++]);
    }
    fields += ")";
    values += ")";
    sql += fields + values;
    sql += _conditionsString(binds, pos);

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
    for (FiledValueItem *item : m_itemsWrite)
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
    for (FiledValueItem *item : m_itemsRead)
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
    for (FiledValueItem *itr : m_itemsCondition)
    {
        if(itr->IsEmpty())
            continue;

        bool bStatic = itr->IsStaticParam();
        string tmp = itr->GetFieldName() + "=" + (bStatic ? itr->GetParam() : "?");

        if (conditions.length() < 1)
            conditions = string(" where ")+tmp;
        else
            conditions += string(" and ")+tmp;

        if(!bStatic && bind)
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

ExecutItem::ExecutType ExecutItem::_transToType(const char *pro)
{
    if (!pro)
        return ExecutItem::Unknown;

    if (0 == strnicmp(pro, "Insert", 7))
        return ExecutItem::Insert;
    else if (0 == strnicmp(pro, "Delete", 7))
        return ExecutItem::Delete;
    else if (0 == strnicmp(pro, "Update", 7))
        return ExecutItem::Update;
    else if (0 == strnicmp(pro, "Select", 7))
        return ExecutItem::Select;

    return ExecutItem::Unknown;
}
