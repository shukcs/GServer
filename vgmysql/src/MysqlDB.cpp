#include "MysqlDB.h"
#include "DBExecItem.h"
#include "VGMysql.h"
#include "tinyxml.h"
#include "VGTrigger.h"
#include "Utility.h"

//////////////////////////////////////////////////////////////////////
//MysqlDB
//////////////////////////////////////////////////////////////////////
MysqlDB::MysqlDB():m_port(3306)
{
}

MysqlDB::~MysqlDB()
{
    for (VGTable *itr : m_tables)
    {
        delete itr;
    }
    for (ExecutItem *itr : m_sqls)
    {
        delete itr;
    }
    for (VGTrigger *itr : m_triggers)
    {
        delete itr;
    }
}

string MysqlDB::Parse(const TiXmlNode &node)
{
    parseTables(node.FirstChild("Tables"));
    parseSqls(node.FirstChild("SQLs"));
    parseTriggers(node.FirstChild("Triggers"));

    return parseDatabase(node.ToElement());
}

VGTable *MysqlDB::GetTableByName(const std::string &name)const
{
    if (name.length() < 1)
        return NULL;

    for (VGTable *itr : m_tables)
    {
        if (itr->GetName() == name)
            return itr;
    }
    return NULL;
}

ExecutItem *MysqlDB::GetSqlByName(const std::string &name)const
{
    if (name.empty())
        return NULL;

    for (ExecutItem *itr : m_sqls)
    {
        if (itr->GetName() == name)
            return itr;
    }
    return NULL;
}

VGTrigger *MysqlDB::GetTriggerByName(const std::string &name)const
{
    if (name.empty())
        return NULL;

    for (VGTrigger *itr : m_triggers)
    {
        if (itr->GetName() == name)
            return itr;
    }
    return NULL;
}

const char *MysqlDB::GetDBHost()const
{
    return m_ip.empty() ? NULL : m_ip.c_str();
}

int MysqlDB::GetDBPort()const
{
    return m_port;
}

const char *MysqlDB::GetDBUser()const
{
    return m_user.empty() ? NULL : m_user.c_str();
}

const char *MysqlDB::GetDBPswd()const
{
    return m_pswd.empty() ? NULL : m_pswd.c_str();
}

const char *MysqlDB::GetDBCharSet() const
{
    return m_charSet.empty() ? NULL : m_charSet.c_str();
}

const char * MysqlDB::GetDBName() const
{
    return m_dataBase.empty() ? NULL : m_dataBase.c_str();
}

void MysqlDB::parseTables(const TiXmlNode *node)
{
    if (!node)
        return;

    const TiXmlNode *table = node ? node->FirstChild("table") : NULL;
    while (table)
    {
        if (VGTable *tb = VGTable::ParseTable(*table->ToElement(), *this))
            m_tables.push_back(tb);

        table = table->NextSibling("table");
    }
}

void MysqlDB::parseSqls(const TiXmlNode *node)
{
    const TiXmlNode *sql = node ? node->FirstChild("SQL") : NULL;
    while (sql)
    {
        if (ExecutItem *item = ExecutItem::parse(sql->ToElement(), *this))
        {
            if (item->GetName().length() > 0 && NULL == GetTableByName(item->GetName()))
                m_sqls.push_back(item);
            else
                delete item;
        }

        sql = sql->NextSibling("SQL");
    }
}

void MysqlDB::parseTriggers(const TiXmlNode *node)
{
    const TiXmlNode *tgNode = node ? node->FirstChild("Trigger") : NULL;
    while (tgNode)
    {
        if (VGTrigger *trg = VGTrigger::Parse(*tgNode->ToElement(), *this))
            m_triggers.push_back(trg);

        tgNode = tgNode->NextSibling("Trigger");
    }
}

string MysqlDB::parseDatabase(const TiXmlElement *e)
{
    if (!e)
        return string();

    const char *tmp = e->Attribute("user");
    m_user = tmp ? tmp : "root";

    tmp = e->Attribute("pswd");
    m_pswd = tmp ? tmp : "root";

    tmp = e->Attribute("host");
    m_ip = tmp ? tmp : "127.0.0.1";

    tmp = e->Attribute("port");
    m_port = tmp ? Utility::str2int(tmp):3306;

    if (const char *tmpS = e->Attribute("charSet"))
        m_charSet = tmpS;

    tmp = e->Attribute("database");
    m_dataBase = tmp ? tmp : "gsuav";
    tmp = e->Attribute("name");
    return tmp ? tmp : string();
}