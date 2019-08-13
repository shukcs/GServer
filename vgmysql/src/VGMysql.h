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

class FiledValueItem;
class ExecutItem;
class VGTable;
class VGTrigger;

class VGMySql
{
public:
    SHARED_SQL VGMySql();
    SHARED_SQL VGMySql(const char *host, int port, const char *user, const char *pswd);
    SHARED_SQL virtual ~VGMySql();

    SHARED_SQL bool ConnectMySql(const char *host, int port, const char *user, const char *pswd);
    SHARED_SQL bool IsValid() const;

    SHARED_SQL bool Execut(ExecutItem *item);
    SHARED_SQL ExecutItem *GetResult();
    SHARED_SQL bool EnterDatabase(const std::string &db);
    SHARED_SQL bool ExistTable(const std::string &name);
    SHARED_SQL bool CreateTable(VGTable *tb);
    SHARED_SQL bool ExistTrigger(const std::string &name);
    SHARED_SQL bool CreateTrigger(VGTrigger *trigger);

    MYSQL_RES *Query(const std::string &sql);
protected:
	bool _canOperaterDB();
	bool _executChange(const std::string &sql, MYSQL_BIND *binds, FiledValueItem *i=NULL);
    bool _changeItem(ExecutItem *item);
    bool _selectItem(ExecutItem *item);
    std::string _getTablesString(const ExecutItem &item);
    MYSQL_STMT *_prepareMySql(const std::string &fmt, MYSQL_BIND *binds, int nRead=0);
    void _checkPrepare();
private:
	MYSQL				*m_mysql;
	std::string			m_host;
	int					m_nPort;
	std::string			m_user;
	std::string			m_pswd;
	bool				m_bValid;
    ExecutItem          *m_execItem;
    MYSQL_BIND          *m_binds;
    MYSQL_STMT          *m_stmt;
};
#endif // __KD_MYSQL_H__