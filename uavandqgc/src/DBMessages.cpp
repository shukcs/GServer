#include "DBMessages.h"
#include "ObjectBase.h"
#include "Utility.h"
#include "ObjectGS.h"
#include "ObjectUav.h"
#include "GSManager.h"
#include "UavManager.h"
#include "DBManager.h"

using namespace std;
#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif

static Variant sVarEmpty;
static const string sStrEmpty;
static const char *rcvId(DBMessage::OBjectFlag f)
{
    switch (f)
    {
    case DBMessage::DB_GS:
        return "DBGS";
    case DBMessage::DB_Uav:
        return "DBUav";
    default:
        break;
    }
    return "";
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//DBMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DBMessage::DBMessage(ObjectGS *sender, MessageType ack, OBjectFlag rcv)
: IMessage(new MessageData(sender, DBExec), rcvId(rcv), IObject::DBMySql)
, m_seq(0), m_ackTp(ack), m_bQueryList(false), m_idxRefSql(0)
{
}

DBMessage::DBMessage(ObjectUav *sender, MessageType ack, OBjectFlag rcv)
: IMessage(new MessageData(sender, DBExec), rcvId(rcv), IObject::DBMySql)
, m_seq(0), m_ackTp(ack), m_bQueryList(false), m_idxRefSql(0)
{
}

DBMessage::DBMessage(GSManager *sender, MessageType ack, OBjectFlag rcv)
: IMessage(new MessageData(sender, DBExec), rcvId(rcv), IObject::DBMySql)
, m_seq(0), m_ackTp(ack), m_bQueryList(false), m_idxRefSql(0)
{
}

DBMessage::DBMessage(UavManager *sender, MessageType ack, OBjectFlag rcv)
: IMessage(new MessageData(sender, DBExec), rcvId(rcv), IObject::DBMySql)
, m_seq(0), m_ackTp(ack), m_bQueryList(false), m_idxRefSql(0)
{
}

DBMessage::DBMessage(ObjectDB *sender, int tpMsg, int tpRcv, const std::string &idRcv)
: IMessage(new MessageData(sender, tpMsg), idRcv, tpRcv), m_seq(0), m_ackTp(Unknown)
, m_bQueryList(false), m_idxRefSql(0)
{
}

void *DBMessage::GetContent() const
{
    return NULL;
}

int DBMessage::GetContentLength() const
{
    return 0;
}

void DBMessage::SetWrite(const std::string &key, const Variant &v, int idx)
{
    if (!key.empty() && !v.IsNull())
        m_writes[propertyKey(key, idx)] = v;
}

const Variant &DBMessage::GetWrite(const std::string &key, int idx)const
{
    VariantMap::const_iterator itr = m_writes.find(propertyKey(key, idx));
    if (itr == m_writes.end())
        return sVarEmpty;

    return itr->second;
}

void DBMessage::SetRead(const std::string &key, const Variant &v, int idx)
{
    if (!key.empty() && !v.IsNull())
        m_reads[propertyKey(key, idx)] = v;
}

void DBMessage::AddRead(const std::string &key, const Variant &v, int idx)
{
    if (!key.empty() && !v.IsNull())
        m_reads[propertyKey(key, idx)].Add(v);
}

const Variant &DBMessage::GetRead(const std::string &key, int idx) const
{
    VariantMap::const_iterator itr = m_reads.find(propertyKey(key, idx));
    if (itr == m_reads.end())
        return sVarEmpty;

    return itr->second;
}

void DBMessage::SetCondition(const std::string &key, const Variant &v, int idx)
{
    if (!key.empty() && !v.IsNull())
        m_conditions[propertyKey(key, idx)] = v;
}

const Variant &DBMessage::GetCondition(const std::string &key, int idx) const
{
    VariantMap::const_iterator itr = m_conditions.find(propertyKey(key, idx));
    if (itr == m_conditions.end())
        return sVarEmpty;

    return itr->second;
}

void DBMessage::SetSql(const std::string &sql, bool bQuerylist)
{
    m_sqls.clear();
    m_bQueryList = bQuerylist;
    AddSql(sql);
}

void DBMessage::AddSql(const std::string &sql)
{
    //不能弄>1个查询
    m_sqls.push_back(sql);
}

const StringList&DBMessage::GetSqls() const
{
    return m_sqls;
}

void DBMessage::SetSeqNomb(int no)
{
    m_seq = no;
}

int DBMessage::GetSeqNomb() const
{
    return m_seq;
}

bool DBMessage::IsQueryList() const
{
    return m_bQueryList;
}

void DBMessage::SetRefFiled(const std::string &filed, int idx)
{
    m_refFiled = filed;
    m_idxRefSql = idx;
}

const std::string &DBMessage::GetRefFiled(int idx) const
{
    if (m_idxRefSql == idx)
        return m_refFiled;

    return sStrEmpty;
}

DBMessage *DBMessage::GenerateAck(ObjectDB *db) const
{
    if (db && m_ackTp >= DBAckBeging && m_ackTp < DBAckEnd)
    {
        DBMessage *ret = new DBMessage(db, m_ackTp, GetSenderType(), GetSenderID());
        if (!ret)
            return NULL;
        ret->SetSeqNomb(m_seq);
        ret->m_bQueryList = m_bQueryList;
        int count = m_sqls.size();
        if (count == 1)
        {
            const Variant &v = GetRead(INCREASEField);
            if (!v.IsNull())
                ret->SetRead(INCREASEField, v);
        }
        else
        { 
            for (int i = 1; i <= count; ++i)
            {
                const Variant &v = GetRead(INCREASEField, i);
                if (!v.IsNull())
                    ret->SetRead(INCREASEField, GetRead(INCREASEField), i);
            }
        }

        return ret;
    }

    return NULL;
}

string DBMessage::propertyKey(const string &key, int idx) const
{
    return (idx<1 || idx>(int)m_sqls.size()) ? key : Utility::l2string(idx) + "." + key;
}
