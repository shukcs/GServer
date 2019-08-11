#include "VGDBManager.h"
#include "DBExecItem.h"
#include <tinyxml.h>

//////////////////////////////////////////////////////////////////////
//VGDBManager
//////////////////////////////////////////////////////////////////////
VGDBManager::VGDBManager()
{
}

VGDBManager &VGDBManager::Instance()
{
    static VGDBManager sIns;
    return sIns;
}

long VGDBManager::str2int(const std::string &str, unsigned radix, bool *suc)
{
    unsigned count = str.length();
    if ((radix != 8 && radix != 10 && radix != 16) || count < 1)
    {
        if (suc)
            *suc = false;
        return 0;
    }

    bool bSubMin = false;
    const char *c = str.c_str();
    unsigned i = 0;
    while (' ' == c[i] || '\t' == c[i])++i;

    if (i < count && (c[i] == '+' || c[i] == '-'))
    {
        if (c[i] == '-')
            bSubMin = true;
        ++i;
    }
    if (i >= count)
    {
        if (suc)
            *suc = false;
        return 0;
    }

    long nRet = 0;
    bool bS = true;
    for (; i < count; ++i)
    {
        int nTmp = c[i] - '0';
        if (nTmp > 10 && radix == 16)
            nTmp = 10 + (c[i] > 'F' ? c[i] - 'a' : c[i] - 'A');

        if (nTmp < 0 || nTmp >= int(radix))
        {
            if (' ' != c[i] && '\t' != c[i])
                bS = false;
            break;
        }
        nRet = nRet * radix + nTmp;
    }
    if (suc)
        *suc = bS;
    if (bSubMin && bS)
        nRet = -nRet;

    return bS ? nRet : 0;
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

bool VGDBManager::Load(const std::string &fileName)
{
    TiXmlDocument myDocument;
    myDocument.LoadFile(fileName.c_str());
    TiXmlElement *rootElement = myDocument.RootElement();
    if (!rootElement)
        return false;

    TiXmlNode *node = rootElement->FirstChild("Mysql");
    if (!node)
        return false;

    parseMysql(node->ToElement());

    node = rootElement->FirstChild("Tables");
    TiXmlElement *tables = node ? node->ToElement():NULL;
    TiXmlNode *table = tables ? tables->FirstChild("table"):NULL;
    while (table)
    {
        parseTable(table->ToElement());
        table = table->NextSibling("table");
    }

    node = rootElement->FirstChild("SQLs");
    TiXmlElement *sqls = node ? node->ToElement() : NULL;
    TiXmlNode *sql = sqls?sqls->FirstChild("SQL"):NULL;
    while (sql)
    {
        parseSql(sql->ToElement());
        sql = sql->NextSibling("SQL");
    }
    return true;
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
    if (name.length() < 1)
        return NULL;

    for (ExecutItem *itr : m_sqls)
    {
        if (itr->GetName() == name)
            return itr;
    }
    return NULL;
}

const std::list<VGTable*> &VGDBManager::Tables() const
{
    return m_tables;
}

const char *VGDBManager::GetMysqlHost(const std::string &db) const
{
    for (const MySqlStruct &itr : m_mysqls)
    {
        if (db.empty() || db == itr.m_database)
            return itr.m_host.c_str();
    }
    static string sDef = "127.0.0.1";
    return sDef.c_str();
}

int VGDBManager::GetMysqlPort(const std::string &db/*=""*/) const
{
    for (const MySqlStruct &itr : m_mysqls)
    {
        if (db.empty() || db == itr.m_database)
            return itr.m_port;
    }

    return 3306;
}

const char * VGDBManager::GetMysqlUser(const std::string &db/*=""*/) const
{
    for (const MySqlStruct &itr : m_mysqls)
    {
        if (db.empty() || db == itr.m_database)
            return itr.m_user.c_str();
    }

    static string sDef = "root";
    return sDef.c_str();
}

const char *VGDBManager::GetMysqlPswd(const std::string &db/*=""*/) const
{
    for (const MySqlStruct &itr : m_mysqls)
    {
        if (db.empty() || db == itr.m_database)
            return itr.m_pswd.c_str();
    }
    return NULL;
}

const char *VGDBManager::GetDatabase(const std::string &db/*=""*/) const
{
    for (const MySqlStruct &itr : m_mysqls)
    {
        if (db.empty() || db == itr.m_database)
            return itr.m_database.c_str();
    }

    return NULL;
}

list<string> VGDBManager::GetDatabases() const
{
    list<string> ret;
    for (const MySqlStruct &itr : m_mysqls)
    {
        ret.push_back(itr.m_database);
    }

    return ret;
}

void VGDBManager::parseTable(TiXmlElement *e)
{
    if (!e)
        return;

    if (VGTable *tb = VGTable::ParseTable(*e))
        m_tables.push_back(tb);
}

void VGDBManager::parseSql(TiXmlElement *e)
{
    if (ExecutItem *item = ExecutItem::parse(e))
    {
        if (item->GetName().length()>0 && NULL == GetTableByName(item->GetName()))
            m_sqls.push_back(item);
        else
            delete item;
    }
}

void VGDBManager::parseMysql(TiXmlElement *e)
{
    if (!e)
        return;
    MySqlStruct sqlSrv;

    const char *tmp = e->Attribute("user");
    if (!tmp)
        return;
    sqlSrv.m_user = tmp;
    tmp = e->Attribute("pswd");
    if(tmp)
        sqlSrv.m_pswd = tmp;

    tmp = e->Attribute("host");
    if (!tmp)
        return;
    sqlSrv.m_host = tmp;

    tmp = e->Attribute("port");
    if (!tmp)
        return;
    sqlSrv.m_port = str2int(tmp);

    tmp = e->Attribute("database");
    if (!tmp)
        return;
    sqlSrv.m_database = tmp;

    m_mysqls.push_back(sqlSrv);
}
