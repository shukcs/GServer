#ifndef  __DB_MESSGES_H__
#define __DB_MESSGES_H__

#include "IMessage.h"
#include "Varient.h"
#include <string>
#include <map>

#define INCREASEField "$increase"
#define EXECRSLT "$result"

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif
class ObjectGS;
class ObjectUav;
class GSManager;
class UavManager;
class ObjectDB;
class ExecutItem;

class DBMessage : public IMessage
{
    typedef std::map<std::string, Variant> VariantMap;
public:
    enum OBjectFlag
    {
        DB_GS,
        DB_Uav,
        DB_LOG,
    };
public:
    DBMessage(ObjectGS *sender, MessageType ack=IMessage::Unknown, OBjectFlag rcv = DB_GS);
    DBMessage(ObjectUav *sender, MessageType ack=IMessage::Unknown, OBjectFlag rcv = DB_Uav);
    DBMessage(GSManager *sender, MessageType ack=IMessage::Unknown, OBjectFlag rcv = DB_GS);
    DBMessage(UavManager *sender, MessageType ack=IMessage::Unknown, OBjectFlag rcv = DB_Uav);
    DBMessage(const std::string &sender, int tpSend, MessageType ack=IMessage::Unknown, OBjectFlag rcv=DB_Uav);
    DBMessage(ObjectDB *senderv, int tpMsg, int tpRcv, const std::string &idRc);
    DBMessage(IObjectManager *mgr);

    void *GetContent() const;
    int GetContentLength() const;
    void SetWrite(const std::string &key, const Variant &v, int idx=0);
    const Variant &GetWrite(const std::string &key, int idx = 0)const;
    void SetRead(const std::string &key, const Variant &v, int idx = 0);
    void AddRead(const std::string &key, const Variant &v, int idx = 0);
    const Variant &GetRead(const std::string &key, int idx = 0)const;
    void SetCondition(const std::string &key, const Variant &v, int idx = 0);
    const Variant &GetCondition(const std::string &key, int idx = 0, const std::string &ju="=")const;
    void SetSql(const std::string &sql, bool bQuerylist = false);
    void AddSql(const std::string &sql);
    const StringList &GetSqls()const;
    void SetSeqNomb(int no);
    int GetSeqNomb()const;
    bool IsQueryList() const;
   
    void SetRefFiled(const std::string &filed, int idx=2);
    const std::string &GetRefFiled(int idx)const;
    DBMessage *GenerateAck(ObjectDB *db)const;
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

