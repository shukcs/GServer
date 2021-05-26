#ifndef  __DB_MESSGES_H__
#define __DB_MESSGES_H__

#include "IMessage.h"
#include "Varient.h"
#include <string>
#include <map>
#include "sqlconfig.h"

#define INCREASEField "$increase"
#define EXECRSLT "$result"

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif
class IObject;
class IObjectManager;
class ObjectDB;
class ExecutItem;

class DBMessage : public IMessage
{
    typedef std::map<std::string, Variant> VariantMap;
public:
    enum OBjectFlag
    {
        DB_Unknow = -1,
        DB_GS,
        DB_Uav,
        DB_LOG,
        DB_FZ,
    };
public:
    SHARED_SQL DBMessage(IObject *sender, MessageType ack=IMessage::Unknown, OBjectFlag rcv = DB_Unknow);
    SHARED_SQL DBMessage(IObjectManager *sender, MessageType ack=IMessage::Unknown, OBjectFlag rcv = DB_Unknow);
    SHARED_SQL DBMessage(const std::string &sender, int tpSend, MessageType ack=IMessage::Unknown, OBjectFlag rcv= DB_Unknow);
    SHARED_SQL DBMessage(ObjectDB *senderv, int tpMsg, int tpRcv, const std::string &idRc);
    SHARED_SQL DBMessage(IObjectManager *mgr, const std::string &rcvDB);

    SHARED_SQL void *GetContent() const;
    SHARED_SQL int GetContentLength() const;
    SHARED_SQL void SetWrite(const std::string &key, const Variant &v, int idx=0);
    SHARED_SQL const Variant &GetWrite(const std::string &key, int idx = 0)const;
    SHARED_SQL void SetRead(const std::string &key, const Variant &v, int idx = 0);
    SHARED_SQL void AddRead(const std::string &key, const Variant &v, int idx = 0);
    SHARED_SQL const Variant &GetRead(const std::string &key, int idx = 0)const;
    SHARED_SQL void SetCondition(const std::string &key, const Variant &v, int idx = 0);
    SHARED_SQL const Variant &GetCondition(const std::string &key, int idx = 0, const std::string &ju="=")const;
    SHARED_SQL void SetSql(const std::string &sql, bool bQuerylist = false);
    SHARED_SQL void AddSql(const std::string &sql);
    SHARED_SQL const StringList &GetSqls()const;
    SHARED_SQL void SetSeqNomb(int no);
    SHARED_SQL int GetSeqNomb()const;
    SHARED_SQL bool IsQueryList() const;
   
    SHARED_SQL void SetRefFiled(const std::string &filed, int idx=2);
    SHARED_SQL const std::string &GetRefFiled(int idx)const;
    SHARED_SQL DBMessage *GenerateAck(ObjectDB *db)const;
protected:
    std::string propertyKey(const std::string &key, int idx)const;
protected:
    int             m_seq;
    MessageType     m_ackTp;
    bool            m_bQueryList;
    int             m_idxRefSql;
    StringList      m_sqls;
    VariantMap      m_writes;
    VariantMap      m_conditions;
    VariantMap      m_reads;
    std::string     m_refFiled;
};

#ifdef SOCKETS_NAMESPACE
}
#endif

#endif//__DB_MESSGES_H__

