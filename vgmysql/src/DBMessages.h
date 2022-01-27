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
    CLASS_INFO(DBMessage)
    typedef std::map<std::string, std::map<uint32_t, Variant> > VariantMap;
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
    SHARED_SQL void SetWrite(const std::string &key, const Variant &v, int idx=0);
    SHARED_SQL const Variant &GetWrite(const std::string &key, uint32_t idx = 0)const;
    SHARED_SQL void SetRead(const std::string &key, const Variant &v, int idx = 0);
    SHARED_SQL void AddRead(const std::string &key, const Variant &v, int idx = 0);
    SHARED_SQL const Variant &GetRead(const std::string &key, uint32_t idx = 0)const;
    SHARED_SQL void SetCondition(const std::string &key, const Variant &v, int idx = 0);
    SHARED_SQL void AddCondition(const std::string &key, const Variant &v, int idx = 0);
    SHARED_SQL const Variant &GetCondition(const std::string &key, uint32_t idx = 0, const std::string &ju="=")const;
    SHARED_SQL void SetSql(const std::string &sql, bool bQuerylist = false);
    SHARED_SQL void AddSql(const std::string &sql);
    SHARED_SQL const StringList &GetSqls()const;
    SHARED_SQL int IndexofSql(const std::string &sql)const;
    SHARED_SQL void SetSeqNomb(int no);
    SHARED_SQL int GetSeqNomb()const;
    SHARED_SQL bool IsQueryList() const;
    SHARED_SQL void SetRefFiled(const std::string &filed);
    SHARED_SQL void SetLimit(uint32_t beg=0, uint16_t limit=200);

    const std::string &GetRefFiled()const;
    DBMessage *GenerateAck(ObjectDB *db)const;
    uint32_t GetLimitBeg()const;
    uint16_t GetLimit()const;

    bool HasWrite(const std::string &key, uint32_t idx = 0)const;
    bool HasCondition(const std::string &key, uint32_t idx = 0)const;
protected:
    void *GetContent() const;
    int GetContentLength() const;
    const Variant &getCondition(const std::string &key, uint32_t idx = 0)const;
protected:
    int             m_seq;
    MessageType     m_ackTp;
    char            m_bQueryList;
    uint16_t        m_limit;
    uint32_t        m_begLimit;
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

