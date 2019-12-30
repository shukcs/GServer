#ifndef  __DB_MANAGER_H__
#define __DB_MANAGER_H__

#include "ObjectBase.h"
#include "MysqlDB.h"

namespace google {
    namespace protobuf {
        class Message;
    }
}

class VGMySql;
class FiledVal;
class ExecutItem;
class Variant;
class TiXmlNode;
class TiXmlElement;

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
    void OnConnected(bool bConnected);
    void InitObject();

    void ProcessMessage(IMessage *msg);
    int ProcessReceive(void *buf, int len);
    void CheckTimer(uint64_t ms);
private:
    void _initSqlByMsg(ExecutItem &sql, const DBMessage &msg, int idx);
    void _initRefField(ExecutItem &sql, const std::string &field, uint64_t idx);
    int64_t _executeSql(ExecutItem *sql, DBMessage *msg, int idx);
    static void _initFieldByVarient(FiledVal &fd, const Variant &v);
    static void _save2Message(const FiledVal &fd, DBMessage &msg);
private:
    VGMySql *m_sqlEngine;
};

class DBManager : public IObjectManager
{
public:
    DBManager();
    ~DBManager();

protected:
    bool InitManager(void);
    int GetObjectType()const;
    bool PrcsPublicMsg();
    IObject *PrcsNotObjectReceive(ISocket *s, const char *buf, int len);
    void LoadConfig();
private:
};

#ifdef SOCKETS_NAMESPACE
}
#endif
#endif//__DB_MANAGER_H__

