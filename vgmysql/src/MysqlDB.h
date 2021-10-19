#ifndef __DB_MYSQL_H__
#define __DB_MYSQL_H__

#include <list>
#include <string>
#include "sqlconfig.h"

class ExecutItem;
class VGTable;
class VGTrigger;
class TiXmlNode;
class TiXmlElement;
/********************************
*数据库管理类
********************************/
class MysqlDB
{
public:
    typedef std::list<std::string> StringList;
public:
    MysqlDB();
    virtual ~MysqlDB();
    std::string Parse(const TiXmlNode &);
    VGTable *GetTableByName(const std::string &name)const;
    ExecutItem *GetSqlByName(const std::string &name)const;
    VGTrigger *GetTriggerByName(const std::string &name)const;
    int GetDBPort()const;
    const char *GetDBHost()const;
    const char *GetDBUser()const;
    const char *GetDBPswd()const;
    const char *GetDBCharSet()const;
    const char *GetDBName()const;
protected:
    std::string parseDatabase(const TiXmlElement *e);
    void parseTables(const TiXmlNode *node);
    void parseSqls(const TiXmlNode *node);
    void parseTriggers(const TiXmlNode *node);
protected:
    int                     m_port;
    std::string             m_ip;
    std::string             m_user;
    std::string             m_pswd;
    std::string             m_charSet;
    std::string             m_dataBase;
    std::list<VGTable*>     m_tables;
    std::list<ExecutItem*>  m_sqls;
    std::list<VGTrigger*>   m_triggers;
};

#endif // __DB_MYSQL_H__