#include "VGDBManager.h"
#include "DBExecItem.h"
#include <tinyxml.h>
#include <VGTrigger.h>

//////////////////////////////////////////////////////////////////////
//VGDBManager
//////////////////////////////////////////////////////////////////////
class MySqlStruct
{
public:
    MySqlStruct() :m_port(3306) {}
    MySqlStruct(const MySqlStruct &oth) :m_port(oth.m_port)
        , m_host(oth.m_host), m_database(oth.m_host)
        , m_user(oth.m_host), m_pswd(oth.m_host) {}
    int                     m_port;
    std::string             m_host;
    std::string             m_database;
    std::string             m_user;
    std::string             m_pswd;
};
//////////////////////////////////////////////////////////////////////
//VGDBManager
//////////////////////////////////////////////////////////////////////
VGDBManager::VGDBManager()
{
}

VGDBManager::~VGDBManager()
{
    for (VGTable *itr : m_tables)
    {
        delete itr;
    }

    for (ExecutItem *itr : m_sqls)
    {
        delete itr;
    }

    for (MySqlStruct *itr : m_mysqls)
    {
        delete itr;
    }

    for (VGTrigger *itr : m_triggers)
    {
        delete itr;
    }
}

VGDBManager &VGDBManager::Instance()
{
    static VGDBManager sIns;
    return sIns;
}

long VGDBManager::str2int(const std::string &str, unsigned radix, bool *suc)
{
    unsigned count = str.length();
    bool bSuc = false;
    bool bSubMin = false;
    const char *c = str.c_str();
    long nRet = 0;
    if ((8==radix || 10==radix || 16== radix) && count >0)
    {
        unsigned i = 0;
        while (' ' == c[i] || '\t' == c[i])++i;
        if (i < count && (c[i] == '+' || c[i] == '-'))
        {
            if (c[i] == '-')
                bSubMin = true;
            ++i;
        }
        if (i < count)
            bSuc = true;
        for (; i < count; ++i)
        {
            int nTmp = c[i] - '0';
            if (nTmp > 10 && radix == 16)
                nTmp = 10 + (c[i] > 'F' ? c[i] - 'a' : c[i] - 'A');

            if (nTmp < 0 || nTmp >= int(radix))
            {
                if (' ' != c[i] && '\t' != c[i])
                    bSuc = false;
                break;
            }
            nRet = nRet * radix + (bSubMin ? -nTmp : nTmp);
        }
    }
    if (suc)
        *suc = bSuc;

    return bSuc ? nRet : 0;
}

list<string> VGDBManager::SplitString(const std::string &str, const std::string &sp, bool bSkipEmpty /*= true*/)
{
    list<string> strLsRet;
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

string VGDBManager::Load(const TiXmlDocument &doc, StringList &tbs)
{
    if (const TiXmlElement *rootElement = doc.RootElement())
    {
        const TiXmlNode *node = rootElement->FirstChild("Database");
        if (!node)
            return string();

        tbs = parseTables(rootElement->FirstChild("Tables"));
        parseSqls(rootElement->FirstChild("SQLs"));
        parseTriggers(rootElement->FirstChild("Triggers"));

        return parseDatabase(node->ToElement());
    }

    return string();
}

VGTable *VGDBManager::GetTableByName(const std::string &name) const
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

ExecutItem *VGDBManager::GetSqlByName(const std::string &name) const
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

VGTrigger *VGDBManager::GetTriggerByName(const std::string &name) const
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

const char *VGDBManager::GetMysqlHost(const std::string &db) const
{
    if (MySqlStruct *dbs = _getDbStruct(db))
        return dbs->m_host.c_str();

    static string sDef = "127.0.0.1";
    return sDef.c_str();
}

int VGDBManager::GetMysqlPort(const std::string &db/*=""*/) const
{
    if (MySqlStruct *dbs = _getDbStruct(db))
        return dbs->m_port;

    return 3306;
}

const char * VGDBManager::GetMysqlUser(const std::string &db/*=""*/) const
{
    if (MySqlStruct *dbs = _getDbStruct(db))
        return dbs->m_user.c_str();

    static string sDef = "root";
    return sDef.c_str();
}

const char *VGDBManager::GetMysqlPswd(const std::string &db/*=""*/) const
{
    if (MySqlStruct *dbs = _getDbStruct(db))
        return dbs->m_pswd.c_str();

    return NULL;
}

list<string> VGDBManager::GetDatabases() const
{
    list<string> ret;
    for (MySqlStruct *itr : m_mysqls)
    {
        if(itr)
            ret.push_back(itr->m_database);
    }

    return ret;
}

StringList VGDBManager::GetTriggers() const
{
    StringList ret;
    for (VGTrigger *itr : m_triggers)
    {
        ret.push_back(itr->GetName());
    }

    return ret;
}

StringList VGDBManager::parseTables(const TiXmlNode *node)
{
    StringList ret;
    if (!node)
        return ret;

    const TiXmlNode *table = node ? node->FirstChild("table") : NULL;
    while (table)
    {
        if (VGTable *tb = VGTable::ParseTable(*table->ToElement()))
        {
            m_tables.push_back(tb);
            ret.push_back(tb->GetName());
        }

        table = table->NextSibling("table");
    }

    return ret;
}

void VGDBManager::parseSqls(const TiXmlNode *node)
{
    const TiXmlNode *sql = node ? node->FirstChild("SQL") : NULL;
    while (sql)
    {
        if (ExecutItem *item = ExecutItem::parse(sql->ToElement()))
        {
            if (item->GetName().length() > 0 && NULL == GetTableByName(item->GetName()))
                m_sqls.push_back(item);
            else
                delete item;
        }

        sql = sql->NextSibling("SQL");
    }
}

void VGDBManager::parseTriggers(const TiXmlNode *node)
{
    const TiXmlNode *tgNode = node ? node->FirstChild("Trigger") : NULL;
    while (tgNode)
    {
        if (VGTrigger *trg = VGTrigger::Parse(*tgNode->ToElement()))
            m_triggers.push_back(trg);

        tgNode = tgNode->NextSibling("Trigger");
    }
}

string VGDBManager::parseDatabase(const TiXmlElement *e)
{
    if (!e)
        return string();

    MySqlStruct sqlSrv;

    const char *tmp = e->Attribute("user");
    sqlSrv.m_user = tmp ? tmp : "root";

    tmp = e->Attribute("pswd");
    sqlSrv.m_pswd = tmp ? tmp : "root";

    tmp = e->Attribute("host");
    sqlSrv.m_host = tmp ? tmp : "127.0.0.1";

    tmp = e->Attribute("port");
    sqlSrv.m_port = tmp ? str2int(tmp):3306;

    tmp = e->Attribute("database");
    if (!tmp)
        return string();

    sqlSrv.m_database = tmp;
    m_mysqls.push_back(new MySqlStruct(sqlSrv));
    return sqlSrv.m_database;
}

MySqlStruct *VGDBManager::_getDbStruct(const std::string &db)const
{
    for (MySqlStruct *itr : m_mysqls)
    {
        if (itr && (db.empty() || db == itr->m_database))
            return itr;
    }
    return NULL;
}
