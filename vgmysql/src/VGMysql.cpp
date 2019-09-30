#include "VGMysql.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#if !defined _WIN32 && !defined _WIN64
#include <mysql/mysql.h>
#else
#include <mysql.h>
#endif
#include "DBExecItem.h"
#include "VGDBManager.h"
#include "VGTrigger.h"

using namespace std;

VGMySql::VGMySql() : m_bValid(false)
, m_binds(NULL) , m_stmt(NULL)
{
	m_mysql = mysql_init(NULL);
	if(!m_mysql)
		return;
}

VGMySql::VGMySql( const char *host, int port, const char *user, const char *pswd)
: m_bValid(false), m_binds(NULL), m_stmt(NULL)
{
	m_mysql = mysql_init(NULL);
	if(!m_mysql)
		return;

	ConnectMySql(host, port, user, pswd);
}

bool VGMySql::IsValid() const
{
	return m_mysql!=NULL && m_bValid;
}

VGMySql::~VGMySql()
{
    _checkPrepare();
	if (m_mysql)
		 mysql_close(m_mysql);
}

bool VGMySql::Execut(ExecutItem *item)
{
    if (!item->IsValid())
        return false;

    switch (item->GetType())
    {
    case ExecutItem::Insert:
    case ExecutItem::Delete:
    case ExecutItem::Update:
        return _changeItem(item);
    case ExecutItem::Select:
        return _selectItem(item);
    default:
        break;
    }
    return false;
}

ExecutItem *VGMySql::GetResult()
{
    if (!m_stmt || !m_execItem || !m_binds || mysql_stmt_fetch(m_stmt))
        _checkPrepare();

    return m_execItem;
}

bool VGMySql::EnterDatabase(const std::string &db, const char *cset)
{
    bool ret = false;
    if (db.empty())
        return ret;

    char tmp[128];
    const char *set = cset ? cset : "utf8";
    sprintf(tmp, "default character set %s collate %s_general_ci", set, set);
    string sql = string("create database if not exists ") + db + " " + tmp;
    if (MYSQL_RES *res = Query(sql))
    {
        my_ulonglong nNum = mysql_num_rows(res);
        mysql_free_result(res);
        ret = nNum > 0;
    }

    if (MYSQL_RES *res = Query(string("use ") + db))
        mysql_free_result(res);

    return ret;
}

bool VGMySql::ExistTable(const std::string &name)
{
    if (!_canOperaterDB() || name.empty())
        return false;

    string sql;
    sql.resize(name.length()+20);
    sprintf(&sql.at(0), "show tables like \'%s\'", name.c_str());

    if (MYSQL_RES *res = Query(sql.c_str()))
    {
        my_ulonglong nNum = mysql_num_rows(res);
        mysql_free_result(res);
        return nNum > 0;
    }
    return false;
}

bool VGMySql::CreateTable(VGTable *tb)
{
    if (!tb || !_canOperaterDB())
        return false;

    
    if (MYSQL_RES *res = Query(tb->ToCreateSQL()))
        mysql_free_result(res);

    return ExistTable(tb->GetName());
}

bool VGMySql::ExistTrigger(const std::string &name)
{
    if (!_canOperaterDB() || name.empty())
        return false;

    static string sTrigExFmt = "SELECT TRIGGER_NAME FROM information_schema.triggers ";
    string sql= sTrigExFmt+"where TRIGGER_NAME='" + name+"'";

    if (MYSQL_RES *res = Query(sql))
    {
        my_ulonglong nNum = mysql_num_rows(res);
        mysql_free_result(res);
        return nNum > 0;
    }
    return false;
}

bool VGMySql::CreateTrigger(VGTrigger *trigger)
{
    if (!trigger || ExistTrigger(trigger->GetName()))
        return false;

    if (MYSQL_RES *res = Query(trigger->ToSqlString()))
    {
        mysql_free_result(res);
        return true;
    }
    return false;
}

bool VGMySql::_canOperaterDB()
{
	if (m_bValid && mysql_ping(m_mysql))
	{
		if (mysql_real_connect(m_mysql, m_host.c_str(), m_user.c_str(), m_pswd.c_str(), NULL, m_nPort, NULL, 0))
			return false;
	}
	return m_bValid;
}

bool VGMySql::ConnectMySql( const char *host, int port, const char *user, const char *pswd)
{
    if (!m_mysql)
        return m_bValid = false;

    if (mysql_real_connect(m_mysql, host, user, pswd, NULL, port, NULL, 0))
    {
        m_host = host ? host : string();
        m_nPort = port;
        m_user = user ? user : string();
        m_pswd = pswd ? pswd : string();
        return m_bValid = true;
    }

    fprintf(stderr, " mysql_real_connect() failed %s\n", mysql_error(m_mysql));
    string strErr = mysql_error(m_mysql);
    return m_bValid = false;
}

bool VGMySql::_executChange(const string &sql, MYSQL_BIND *binds, FiledVal *i)
{
    MYSQL_STMT *stmt = _prepareMySql(sql, binds);
    if (!stmt)
        return false;

    bool ret = true;
    if (mysql_stmt_affected_rows(stmt) < 1)
        ret = false;

    if(i)
    {
        uint64_t n = mysql_insert_id(m_mysql);
        i->InitBuff(sizeof(n), &n);
    }
    mysql_stmt_close(stmt);
    return ret;
}

bool VGMySql::_changeItem(ExecutItem *item)
{
    if (!item || !item->IsValid())
        return false;

    if (!_canOperaterDB())
        return false;

    MYSQL_BIND *binds  = item->GetParamBinds();
    string sql = item->GetSqlString(binds);

    bool ret = false;
    if (sql.length() > 0)
    {
        if (item->HasForeignRefTable())
        {
            if (MYSQL_RES *res = Query("SET FOREIGN_KEY_CHECKS=0"))
                mysql_free_result(res);
        }
        ret = _executChange(sql, binds, item->GetIncrement());
        if (item->HasForeignRefTable())
        {
            if (MYSQL_RES *res = Query("SET FOREIGN_KEY_CHECKS=1"))
                mysql_free_result(res);
        }
    }

    delete binds;
    return ret;
}

bool VGMySql::_selectItem(ExecutItem *item)
{
    _checkPrepare();
    int count = item ? item->CountRead() : 0;
    if (count < 1 || !item->IsValid() || item->GetType() != ExecutItem::Select)
        return false;
    if (!_canOperaterDB())
        return false;

    MYSQL_BIND *params = item->GetParamBinds();
    string sql = item->GetSqlString(params);
    MYSQL_STMT *stmt = _prepareMySql(sql, params);
    if (!stmt)
    {
        delete params;
        return false;
    }
    MYSQL_BIND *binds = item->TransformRead();
    bool ret = false;
    if (mysql_stmt_bind_result(stmt, binds))
        fprintf(stderr, "mysql_stmt_bind_result() failed! %s\n", mysql_stmt_error(stmt));
    else if (mysql_stmt_store_result(stmt))
        fprintf(stderr, " mysql_stmt_store_result() failed %s\n", mysql_stmt_error(stmt));
    else if (mysql_stmt_fetch(stmt))
        fprintf(stderr, "no record fit SQL '%s'!\n", item->GetName().c_str());
    else
        ret = true;

    delete params;
    if (!ret)
    {
        delete binds;
        if(stmt)
        {
            mysql_stmt_free_result(stmt);
            mysql_stmt_close(stmt);
        }
        return false;
    }

    m_binds = binds;
    m_stmt = stmt;
    m_execItem = item;
    return true;
}

MYSQL_STMT *VGMySql::_prepareMySql(const string &strFormat, MYSQL_BIND *binds, int nRead)
{
    MYSQL_STMT *stmt = mysql_stmt_init(m_mysql);
	if (!stmt)
	{
		fprintf(stderr, " mysql_stmt_init(), out of memory\n");
        return NULL;
    }
    bool ret = false;
    if (mysql_stmt_prepare(stmt, strFormat.c_str(), strFormat.length()))
        fprintf(stderr, "mysql_stmt_prepare() failed! %s\n", mysql_stmt_error(stmt));
    else if (binds && mysql_stmt_bind_param(stmt, binds))
        fprintf(stderr, "mysql_stmt_bind_param() failed\n");
    else
        ret = true;

    if (ret && nRead>0)
    {
        MYSQL_RES *preRes = mysql_stmt_result_metadata(stmt);
        if (!preRes || nRead != (int)mysql_num_fields(preRes))
            ret = false;
    }
    if (ret && mysql_stmt_execute(stmt))
    {
        fprintf(stderr, "mysql_stmt_execute(), %s failed\n", mysql_error(m_mysql));
        ret = false;
    }

    if (!ret)
    {
        mysql_stmt_close(stmt);
        stmt = NULL;
    }

    return stmt;
}

void VGMySql::_checkPrepare()
{
    if (m_binds)
    {
        delete m_binds;
        m_binds = NULL;
    }
    if (m_stmt)
    {
        mysql_stmt_free_result(m_stmt);
        mysql_stmt_close(m_stmt);
        m_stmt = NULL;
    }

    m_execItem = NULL;
}

std::string VGMySql::_getTablesString(const ExecutItem &item)
{
    string ret;
    for (const string &table : item.ExecutTables())
    {
        ret += " " + table;
    }
    return ret;
}

MYSQL_RES *VGMySql::Query(const std::string &sql)
{
    if (mysql_real_query(m_mysql, sql.c_str(), sql.length()))
    {
        fprintf(stderr, "mysql_real_query() failed! %s\n", mysql_error(m_mysql));
        return NULL;
    }

    return mysql_store_result(m_mysql);
}
