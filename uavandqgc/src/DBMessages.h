#ifndef  __DB_MESSGES_H__
#define __DB_MESSGES_H__

#include "IMessage.h"
#include "Varient.h"
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
    DBMessage(ObjectGS *sender, MessageType ack=IMessage::Unknown, const std::string &rcv="DBGS");
    DBMessage(ObjectUav *sender, MessageType ack=IMessage::Unknown, const std::string &rcv = "DBUav");
    DBMessage(GSManager *sender, MessageType ack=IMessage::Unknown, const std::string &rcv = "DBGS");
    DBMessage(UavManager *sender, MessageType ack= IMessage::Unknown, const std::string &rcv = "DBUav");
    DBMessage(ObjectDB *senderv, int tpMsg, int tpRcv, const std::string &idRc);

    void *GetContent() const;
    int GetContentLength() const;
    void SetWrite(const std::string &key, const Variant &v, int idx=0);
    const Variant &GetWrite(const std::string &key, int idx = 0)const;
    void SetRead(const std::string &key, const Variant &v, int idx = 0);
    void AddRead(const std::string &key, const Variant &v, int idx = 0);
    const Variant &GetRead(const std::string &key, int idx = 0)const;
    void SetCondition(const std::string &key, const Variant &v, int idx = 0);
    const Variant &GetCondition(const std::string &key, int idx = 0)const;
    void SetSql(const std::string &sql, bool bQuerylist = false);
    void AddSql(const std::string &sql);
    const StringList &GetSqls()const;
    void SetSeqNomb(int no);
    int GetSeqNomb()const;
    bool IsQueryList() const;

    DBMessage *GenerateAck(ObjectDB *db)const;
protected:
    std::string propertyKey(const std::string &key, int idx)const;
protected:
    int             m_seq;
    MessageType     m_ackTp;
    bool            m_bQueryList;
    StringList      m_sqls;
    VariantMap      m_writes;
    VariantMap      m_conditions;
    VariantMap      m_reads;
};

#ifdef SOCKETS_NAMESPACE
}
#endif

#endif//__DB_MESSGES_H__

