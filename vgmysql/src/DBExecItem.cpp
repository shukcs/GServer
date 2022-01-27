#include "DBExecItem.h"

#if !defined _WIN32 && !defined _WIN64
#include <mysql/mysql.h>
#else
#include <mysql.h>
#endif

#include "MysqlDB.h"
#include "VGMysql.h"
#include <tinyxml.h>
#include <string.h>
#include <chrono>
#include "Utility.h"

#define LeftBraceCount(f) ((f&NumbLeftMask))
#define RightBraceCount(f) ((f&NumbRightMask)>>8)
using namespace std::chrono;

static const list<FiledVal*> lsFieldEmpty;
///////////////////////////////////////////////////////////////////////////////////////
//FiledVal
///////////////////////////////////////////////////////////////////////////////////////
FiledVal::FiledVal(int tp, const std::string &name, int len): m_type(tp), m_buff(NULL)
, m_exParam(NULL),m_lenMax(0), m_len(0), m_tpField(NoBuff), m_bEmpty(1), m_name(name)
, m_condition("=")
{
    InitBuff(len);
    transType(tp);
}

FiledVal::FiledVal(VGTableField *fild, bool bOth): m_type(-1), m_buff(NULL)
, m_exParam(NULL), m_lenMax(0), m_len(0), m_tpField(NoBuff), m_bEmpty(1)
, m_name(), m_condition("=")
{
    if (fild)
    {
        m_type = fild->GetType();
        m_name = fild->GetName(bOth);
        InitBuff(fild->GetLength());
        transType(m_type);
    }
}

FiledVal::~FiledVal()
{
    delete m_buff;
}

void FiledVal::transType(int sqlType)
{
    switch (sqlType)
    {
    case MYSQL_TYPE_TINY:
        m_tpField = Int8; break;
    case MYSQL_TYPE_SHORT:
        m_tpField = Int16; break;
    case MYSQL_TYPE_LONG:
        m_tpField = Int32; break;
    case MYSQL_TYPE_FLOAT:
        m_tpField = Float; break;
    case MYSQL_TYPE_DOUBLE:
        m_tpField = Double; break;
    case MYSQL_TYPE_LONGLONG:
        m_tpField = Int64; break;
    case MYSQL_TYPE_VAR_STRING:
        m_tpField = Buff; break;
    case MYSQL_TYPE_STRING:
        m_tpField = String; break;
    default:
        break;
    }
}

void FiledVal::SetFieldName(const string &name)
{
    m_name = name;
}

const string &FiledVal::GetFieldName() const
{
    return m_name;
}

const string & FiledVal::GetJudge() const
{
    return m_condition;
}

ExecutItem *FiledVal::ComplexSql() const
{
    return m_exParam;
}

void FiledVal::SetNotStringParam(const string &param)
{
    if (m_tpField & (StaticRef | StaticParam))
        return;

    if (param.length() > 0)
    {
        if (m_buff)
        {
            delete m_buff;
            m_buff = NULL;
        }
        m_param = param;
        m_bEmpty = 0;
        m_tpField = NoBuff;
    }
}

void FiledVal::SetParam(const string &param, FieldType tp)
{
    if (m_tpField & (StaticRef | StaticParam))
        return;

    if(param.length() > 0 && (NoBuff==tp || StaticRef==tp || StaticParam==tp))
    {
        m_len = 0;
        if (StaticParam==tp || NoBuff==tp)
            m_param = string("\'") + param + "\'";
        else
            m_param = param;

        m_bEmpty = 0;
        m_tpField = (FieldType)((m_tpField&~List)|tp);
    }
}

void FiledVal::SetParam(const list<string> &param)
{
    if (param.empty())
        return;

    m_bEmpty = false;
    m_tpField = (FieldType)(m_tpField|List);
    m_param.clear();
    for (const string &itr : param)
    {
        if (m_param.empty())
            m_param = "(" + m_name + m_condition + "'" + itr + "'";
        else
            m_param += " or " + m_name + m_condition + "'" + itr + "'";
    }
    m_param += ")";
}

void FiledVal::SetParam(const string &exFild, const string &param, FieldType tp /*= NoBuff*/)
{
    if (!m_exParam)
        return;

    if (auto *f = m_exParam->GetConditionItem(exFild))
        f->SetParam(param, tp);
}

void FiledVal::SetParam(const string &exFild, const list<string> &param)
{
    if (!m_exParam)
        return;

    if (auto *f = m_exParam->GetConditionItem(exFild))
        f->SetParam(param);
}

const string & FiledVal::GetParam() const
{
    return m_param;
}

void FiledVal::InitBuff(unsigned len, const void *buf)
{
    if (m_tpField & (StaticParam | StaticRef))
        return;

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

    m_tpField = (FieldType)(m_tpField&~List);
    m_bEmpty = 0;
}

string FiledVal::ToConditionString(const string &str)const
{
    string ret;
    if (m_exParam)
        ret += m_name + m_condition + "(" + m_exParam->GetSqlString() + ")";
    else if (List & m_tpField)
        ret = m_param;
    else
        ret += m_name + m_condition + (IsStringParam() ? GetParam() : "?");
   
    return finallyString(ret, getBraceFlag(str));
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

FiledVal::FieldType FiledVal::GetParamType() const
{
    return m_tpField;
}

int FiledVal::GetType() const
{
    return m_type;
}

unsigned long &FiledVal::ReadLength()
{
    return m_len;
}

bool FiledVal::IsStringParam() const
{
    return m_len<=0 && m_param.length()>0;
}

bool FiledVal::IsEmpty() const
{
    return m_bEmpty && m_exParam==NULL;
}

void FiledVal::SetEmpty()
{
    if (m_exParam)
    {
        m_exParam->ClearData();
    }
    else if (0 == ((StaticParam | StaticRef)&m_tpField))
    {
        m_param.clear();
        m_bEmpty = 1;
    }

    m_len = 0;
}

void FiledVal::parse(const TiXmlElement *e, ExecutItem *sql, const MysqlDB &db)
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

    if (FiledVal *item = parseFiled(db, *e, sql->ExecutTables().size() > 1))
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

FiledVal *FiledVal::parseFiled(const MysqlDB &db, const TiXmlElement &e, bool oth)
{
    const char *name = e.Attribute("name");

    if (const TiXmlNode *node = e.FirstChild("SQL"))
    {
        FiledVal *item = new FiledVal(NoBuff, name, 0);
        item->m_exParam = ExecutItem::parse(node->ToElement(), db);
        return item;
    }
    else if (const char *tmp = e.Attribute("ref"))
    {
        if (VGTable *tb = db.GetTableByName(tmp))
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
        FiledVal *item = new FiledVal(NoBuff, name, 0);
        if (tmp)
            item->SetParam(tmp, StaticRef);
        else if (const char *val = e.Attribute("paramVal"))
            item->SetParam(val, StaticParam);
        return item;
    }

    return NULL;
}

BraceFlag FiledVal::getBraceFlag(const string &str)
{
    int nLeft = 0;
    int nRight = 0;
    const char *tmp = str.c_str();
    
    for (uint32_t i = 0; i < str.size(); ++i)
    {
        if (tmp[i] == '(')
            ++nLeft;
        else if (tmp[i] == ')')
            ++nRight;
        else
            break;
    }
    return BraceFlag(nLeft | (nRight << 8));
}

string FiledVal::finallyString(const string &str, BraceFlag f)
{
    int nTmp = LeftBraceCount(f);
    string ret;
    while (nTmp-- > 0)
    {
        ret += "(";
    }
    ret += str;
    nTmp = RightBraceCount(f);
    while (nTmp-- > 0)
    {
        ret += ")";
    }
    return ret;
}
///////////////////////////////////////////////////////////////////////////////////////
//ExecutItem
///////////////////////////////////////////////////////////////////////////////////////
ExecutItem::ExecutItem(ExecutType tp, const std::string &name): m_type(tp)
, m_name(name), m_autoIncrement(NULL), m_bHasForeignRefTable(false)
, m_bRef(false), m_nlimit(200), m_limitBeg(0)
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

void ExecutItem::SetLimit(uint32_t beg, uint16_t limit)
{
    if (m_type == Select)
    {
        m_nlimit = limit;
        m_limitBeg = beg;
    }
}

const StringList &ExecutItem::ExecutTables() const
{
    return m_tables;
}

void ExecutItem::SetExecutTables(const StringList &tbs, const MysqlDB &db)
{
    for (const std::string &itr : tbs)
    {
        AddExecutTable(itr, db);
    }
}

void ExecutItem::AddExecutTable(const std::string &t, const MysqlDB &db)
{
    for (const std::string &itr:m_tables)
    {
        if (itr == t)
            return;
    }
    if (!m_bHasForeignRefTable)
    {
        VGTable *tb = db.GetTableByName(t);
        if (tb && tb->IsForeignRef())
            m_bHasForeignRefTable = true;
    }
    m_tables.push_back(t);
}

void ExecutItem::AddItem(FiledVal *item, int tp)
{
    if (!item)
        return;

    switch (tp)
    {
    case ExecutItem::Read:
        m_itemsRead.push_back(item); break;
    case ExecutItem::Write:
        m_itemsWrite.push_back(item); break;
    case ExecutItem::Condition:
        _addCondition(item); break;
    default:
        break;
    }
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

const list<FiledVal*> &ExecutItem::Fields(FiledType tp) const
{
    switch (tp)
    {
    case ExecutItem::Read:
        return m_itemsRead;
    case ExecutItem::Write:
        return m_itemsWrite;
    case ExecutItem::Condition:
        return m_itemsCondition;
    case ExecutItem::AutoIncrement:
        break;
    default:
        break;
    }
    return lsFieldEmpty;
}

bool ExecutItem::IsValid() const
{
    return (m_type>ExecutItem::Unknown && m_type <= Select && m_tables.size() > 0);
}

 FiledVal *ExecutItem::GetItem(const string &name, FiledType tp) const
{
     switch (tp)
     {
     case ExecutItem::Read:
         return GetReadItem(name);
     case ExecutItem::Write:
         return GetWriteItem(name);
     case ExecutItem::Condition:
         return GetConditionItem(name);
     case ExecutItem::AutoIncrement:
         return m_autoIncrement;
     default:
         break;
     }

     return NULL;
}

MYSQL_BIND *ExecutItem::GetParamBinds()
{
    int count = 0;
    for (FiledVal *itr : m_itemsWrite)
    {
        if (!itr->IsEmpty() && !itr->IsStringParam())
            count++;
    }
    for (FiledVal *itr : m_itemsCondition)
    {
        if (!itr->IsEmpty() && !itr->IsStringParam())
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
    MYSQL_BIND *binds = sz > 0 ? new MYSQL_BIND[sz] : NULL;
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
    return m_bHasForeignRefTable!=0;
}

void ExecutItem::SetRef(bool b)
{
    m_bRef = b;
}

bool ExecutItem::IsRef() const
{
    return m_bRef!=0;
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
            bind.is_null = (my_bool *)&item->m_bEmpty;
        }
    }
}

ExecutItem *ExecutItem::parse(const TiXmlElement *e, const MysqlDB &db)
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
        sql->m_conditions = VGMySql::SplitString(tmpC, ";");
    if (const char *tmpC = e->Attribute("group"))
        sql->m_group = tmpC;
    sql->SetExecutTables(VGMySql::SplitString(table, ";"), db);
    sql->_parseItems(e, db);

    return sql;
}

void ExecutItem::_parseItems(const TiXmlElement *e, const MysqlDB &db)
{
    if (!e)
        return;
    
    const char *tmp = e->Attribute("all");
    if (tmp && VGMySql::Str2int(tmp) != 0)
    {
        for (const string &itr : m_tables)
        {
            if (VGTable *tb = db.GetTableByName(itr))
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
            FiledVal::parse(node->ToElement(), this, db);
            node = node->NextSibling("item");
        }
    }
}

void ExecutItem::_addCondition(FiledVal *item)
{
    string tmp = item ? item->GetFieldName() : string();
    for (FiledVal *itr : m_itemsCondition)
    {
        if (itr == item || (itr->GetFieldName()==tmp && item->GetJudge()==itr->GetJudge()))
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

        bool bStatic = item->IsStringParam();  
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
    if (!m_group.empty())
        sql += " group by " + m_group;

    if (m_limitBeg > 0 || m_nlimit > 0)
    {
        if (m_limitBeg>0)
            sql += " limit " + Utility::l2string(m_limitBeg) + "," + Utility::l2string(m_nlimit);
        else
            sql += " limit " + Utility::l2string(m_nlimit);
    }

    return sql;
}

string ExecutItem::_conditionsString(MYSQL_BIND *bind, int &pos) const
{
    auto itrC = m_conditions.begin();
    string strL;
    string conditions;
    for (FiledVal *itr : m_itemsCondition)
    {
        string tmp = m_conditions.size() == 1 ? m_conditions.front() : (itrC != m_conditions.end() ? *itrC : "and");
        if (itrC != m_conditions.end())
            ++itrC;

        if(itr->IsEmpty())
        {
            strL = _getCondition(tmp);
            continue;
        }

        if (conditions.empty())
            conditions = string(" where ")+ itr->ToConditionString(tmp);
        else
            conditions += strL + itr->ToConditionString(tmp);

        strL = _getCondition(tmp);
        if(bind && !itr->IsStringParam())
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

        bool bStatic = item->IsStringParam();
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

std::string ExecutItem::_getCondition(const std::string &str)const
{
    if (str.empty())
        return " and ";
    //¿‡–Õ”–imp, ()imp£¨(imp, imp), (()imp, ())...
    const char *pStr = str.c_str();
    int beg = -1;
    int nLen = 0;
    for (uint32_t i = 0; i < str.size(); ++i)
    {
        if ('(' != pStr[i] && ')' != pStr[i])
        {
            if (beg < 0)
                beg = i;
            nLen++;
            continue;
        }

        if (beg>=0)
            break;
    }

    return beg >= 0 ? " " + str.substr(beg, nLen) + " " : string();
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
