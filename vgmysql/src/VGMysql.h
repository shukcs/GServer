#ifndef __VG_MYSQL_H__
#define __VG_MYSQL_H__

#include <string>
#include <list>
#include <map>
#include "sqlconfig.h"

typedef struct st_mysql MYSQL;
typedef struct st_mysql_stmt MYSQL_STMT;
typedef struct st_mysql_bind MYSQL_BIND;
typedef struct st_mysql_res MYSQL_RES;

class FiledVal;
class ExecutItem;
class VGTable;
class VGTrigger;

class VGMySql
{
public:
    SHARED_SQL VGMySql();
    SHARED_SQL VGMySql(const char *host, int port, const char *user, const char *pswd);
    SHARED_SQL virtual ~VGMySql();

    SHARED_SQL bool ConnectMySql(const char *host, int port, const char *user, const char *pswd, const char *db=NULL);
    SHARED_SQL bool IsValid() const;

    SHARED_SQL bool Execut(ExecutItem *item);
    SHARED_SQL bool Execut(const std::string &sql);
    ExecutItem *GetResult();
    bool EnterDatabase(const std::string &db=std::string(), const char *cset=NULL);
    bool ExistTable(const std::string &name);
    bool CreateTable(VGTable *tb);
    bool ExistTrigger(const std::string &name);
    bool CreateTrigger(VGTrigger *trigger);
public:
    MYSQL_RES *Query(const std::string &sql);
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
protected:
	bool _canOperaterDB();
	bool _executChange(const std::string &sql, MYSQL_BIND *binds, FiledVal *i=NULL);
    bool _changeItem(ExecutItem *item);
    bool _selectItem(ExecutItem *item);
    std::string _getTablesString(const ExecutItem &item);
    bool _reconnect();
    MYSQL_STMT *_prepareMySql(const std::string &fmt, MYSQL_BIND *binds, int nRead=0);
    void _checkPrepare();
private:
	MYSQL				*m_mysql;
	std::string			m_host;
	int					m_nPort;
	std::string			m_user;
    std::string			m_pswd;
    std::string			m_database;
	bool				m_bValid;
    ExecutItem          *m_execItem;
    MYSQL_BIND          *m_binds;
    MYSQL_STMT          *m_stmt;
};
#endif // __KD_MYSQL_H__