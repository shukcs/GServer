#ifndef __VG_DB_MANAGER_H__
#define __VG_DB_MANAGER_H__

#include "sqlconfig.h"
#include <list>
#include <string>

class ExecutItem;
class VGTable;
class TiXmlElement;

class VGDBManager {
    struct MySqlStruct{
        std::string             m_host;
        int                     m_port;
        std::string             m_database;
        std::string             m_user;
        std::string             m_pswd;
    };
public:
    SHARED_SQL bool Load(const std::string &fileName);
    SHARED_SQL VGTable *GetTableByName(const std::string &name)const;
    SHARED_SQL ExecutItem *GetSqlByName(const std::string &name)const;
    SHARED_SQL const std::list<VGTable*> &Tables()const;
    SHARED_SQL const char *GetMysqlHost(const std::string &db ="")const;
    SHARED_SQL int GetMysqlPort(const std::string &db="")const;
    SHARED_SQL const char *GetMysqlUser(const std::string &db="")const;
    SHARED_SQL const char *GetMysqlPswd(const std::string &db="")const;
    SHARED_SQL const char *GetDatabase(const std::string &db = "")const;
    SHARED_SQL std::list<std::string> GetDatabases()const;
public:
    static SHARED_SQL VGDBManager &Instance();
    static SHARED_SQL long str2int(const std::string &str, unsigned radix=10, bool *suc=NULL);
    static SHARED_SQL std::list<std::string> SplitString(const std::string &str, const std::string &sp, bool bSkipEmpty = true);
private:
    VGDBManager();

    void parseTable(TiXmlElement *e);
    void parseSql(TiXmlElement *e);
    void parseMysql(TiXmlElement *e);
private:
    std::list<VGTable*>     m_tables;
    std::list<ExecutItem*>  m_sqls;
    std::list<MySqlStruct>  m_mysqls;
};

#endif // __VG_DB_MANAGER_H__