#ifndef  __DB_MANAGER_H__
#define __DB_MANAGER_H__

#include "sqlconfig.h"
#include "ObjectBase.h"
#include "MysqlDB.h"

class VGMySql;
class FiledVal;
class ExecutItem;
class Variant;
class TiXmlNode;

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

class DBMessage;
class ObjectDB : public IObject, MysqlDB
{
public:
    ObjectDB(const std::string &id);
    ~ObjectDB();
public:
    static ObjectDB *ParseMySql(const TiXmlElement &e);
protected:
    int GetObjectType()const;
    void InitObject();

    void ProcessMessage(const IMessage *msg);
private:
    void _initSqlByMsg(ExecutItem &sql, const DBMessage &msg, int idx);
    void _initCmpSqlByMsg(ExecutItem &sql, const DBMessage &msg, int idx);
    void _initRefField(ExecutItem &sql, const std::string &field, uint64_t idx);
    int64_t _executeSql(ExecutItem *sql, DBMessage *msg, int idx);
    static void _initFieldByVarient(FiledVal &fd, const Variant &v);
    static void _save2Message(const FiledVal &fd, DBMessage &msg, int idx);
private:
    VGMySql *m_sqlEngine;
};

class DBManager : public IObjectManager
{
public:
    DBManager();
    ~DBManager();

protected:
    int GetObjectType()const;
    bool PrcsPublicMsg(const IMessage &msg);
    IObject *PrcsNotObjectReceive(ISocket *s, const char *buf, int len);
    void LoadConfig(const TiXmlElement *root);
    bool IsReceiveData()const;
private:
};

#ifdef SOCKETS_NAMESPACE
}
#endif
#endif//__DB_MANAGER_H__

