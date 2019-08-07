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

class VGMySql
{
public:
    SHARED_SQL VGMySql();
    SHARED_SQL VGMySql(const char *host, int port, const char *user, const char *pswd, const char *db);
    SHARED_SQL virtual ~VGMySql();

    SHARED_SQL bool ConnectMySql(const char *host, int port, const char *user, const char *pswd, const char *db);
    SHARED_SQL bool IsValid() const;

    SHARED_SQL bool Execut(ExecutItem *item);
    SHARED_SQL ExecutItem *GetResult();
    SHARED_SQL bool ExitTable(const std::string &name);
    SHARED_SQL bool CreateTable(VGTable *tb);
protected:
	bool _canOperaterDB();
	bool _executChange(const std::string &sql, MYSQL_BIND *binds, FiledValueItem *i=NULL);
    bool _changeItem(ExecutItem *item);
    bool _selectItem(ExecutItem *item);
    std::string _getTablesString(const ExecutItem &item);
    MYSQL_STMT *_prepareMySql(const std::string &fmt, MYSQL_BIND *binds, int nRead=0);
    void _checkPrepare();
    MYSQL_RES *_query(const std::string &sql);
    MYSQL_BIND *_getParamBind(const ExecutItem &item);
private:
	MYSQL				*m_mysql;
	std::string			m_host;
	int					m_nPort;
	std::string			m_user;
	std::string			m_pswd;
	std::string			m_db;
	bool				m_bValid;
    ExecutItem          *m_execItem;
    MYSQL_BIND          *m_binds;
    MYSQL_STMT          *m_stmt;
};
#endif // __KD_MYSQL_H__