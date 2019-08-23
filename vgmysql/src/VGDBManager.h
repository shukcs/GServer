#ifndef __VG_DB_MANAGER_H__
#define __VG_DB_MANAGER_H__

#include "sqlconfig.h"
#include <list>
#include <string>

class ExecutItem;
class VGTable;
class VGTrigger;
class TiXmlElement;
class TiXmlNode;
class TiXmlDocument;
class MySqlStruct;

class VGDBManager
{
public:
    typedef std::list<std::string> StringList;
public:
    ~VGDBManager();

    SHARED_SQL std::string Load(const TiXmlDocument &doc, StringList &tbs);
    SHARED_SQL VGTable *GetTableByName(const std::string &name)const;
    SHARED_SQL ExecutItem *GetSqlByName(const std::string &name)const;
    SHARED_SQL VGTrigger *GetTriggerByName(const std::string &name)const;
    SHARED_SQL const char *GetMysqlHost(const std::string &db ="")const;
    SHARED_SQL int GetMysqlPort(const std::string &db="")const;
    SHARED_SQL const char *GetMysqlUser(const std::string &db="")const;
    SHARED_SQL const char *GetMysqlPswd(const std::string &db="")const;
    SHARED_SQL StringList GetDatabases()const;
    SHARED_SQL StringList GetTriggers()const;
public:
    static SHARED_SQL VGDBManager &Instance();
    static SHARED_SQL long str2int(const std::string &str, unsigned radix=10, bool *suc=NULL);
    static SHARED_SQL StringList SplitString(const std::string &str, const std::string &sp, bool bSkipEmpty = true);
    template<typename T, typename Contianer = std::list<T> >
    static bool IsContainsInList(const Contianer ls, const T &e)
    {
        for (const T &itr : ls)
        {
            if (itr == e)
                return true;
        }
        return false;
    }
private:
    VGDBManager();

    std::string parseDatabase(const TiXmlElement *e);
    StringList parseTables(const TiXmlNode *node);
    void parseSqls(const TiXmlNode *node);
    void parseTriggers(const TiXmlNode *node);
    MySqlStruct *_getDbStruct(const std::string &)const;
private:
    std::list<VGTable*>     m_tables;
    std::list<ExecutItem*>  m_sqls;
    std::list<MySqlStruct*> m_mysqls;
    std::list<VGTrigger*>   m_triggers;
};

#endif // __VG_DB_MANAGER_H__