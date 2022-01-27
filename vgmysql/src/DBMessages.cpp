#include "DBMessages.h"
#include "ObjectBase.h"
#include "Utility.h"
#include "DBManager.h"

using namespace std;
#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif

static Variant sVarEmpty;
static const char *rcvId(DBMessage::OBjectFlag f, int objTp)
{
    if (f == DBMessage::DB_Unknow)
    {
        switch (objTp)
        {
        case IObject::Plant:
            f = DBMessage::DB_Uav; break;
        case IObject::GroundStation:
            f = DBMessage::DB_GS; break;
        case IObject::VgFZ:
            f = DBMessage::DB_FZ; break;
        default:
            break;
        }
    }
    switch (f)
    {
    case DBMessage::DB_GS:
        return "DBGS";
    case DBMessage::DB_Uav:
        return "DBUav";
    case DBMessage::DB_LOG:
        return "DBLog";
    case DBMessage::DB_FZ:
        return "DBFz";
    default:
        break;
    }
    return "";
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//DBMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DBMessage::DBMessage(IObject *sender, MessageType ack, OBjectFlag rcv)
: IMessage(new MessageData(sender, DBExec), rcvId(rcv, sender->GetObjectType()), IObject::DBMySql)
, m_seq(0), m_ackTp(ack), m_bQueryList(false), m_limit(200),m_begLimit(0)
{
}

DBMessage::DBMessage(IObjectManager *sender, MessageType ack, OBjectFlag rcv)
: IMessage(new MessageData(sender, DBExec), rcvId(rcv, sender->GetObjectType()), IObject::DBMySql)
, m_seq(0), m_ackTp(ack), m_bQueryList(false), m_limit(200),m_begLimit(0)
{
}

DBMessage::DBMessage(ObjectDB *sender, int tpMsg, int tpRcv, const std::string &idRcv)
: IMessage(new MessageData(sender, tpMsg), idRcv, tpRcv), m_seq(0), m_ackTp(Unknown)
, m_bQueryList(false), m_limit(200), m_begLimit(0)
{
}

DBMessage::DBMessage(IObjectManager *mgr, const string &str)
: IMessage(new MessageData(mgr, DBExec), str, IObject::DBMySql)
, m_limit(200), m_begLimit(0)
{
}

DBMessage::DBMessage(const string &sender, int tpSend, MessageType ack, OBjectFlag rcv)
: IMessage(new MessageData(sender, tpSend, DBExec), rcvId(rcv, tpSend), IObject::DBMySql)
, m_seq(0), m_ackTp(ack), m_bQueryList(false), m_limit(200), m_begLimit(0)
{
}

void DBMessage::SetWrite(const std::string &key, const Variant &v, int idx)
{
    if (key.empty())
        return;

    if (idx < 0)
        idx = m_writes[key].size();

    m_writes[key][idx] = v;
}

const Variant &DBMessage::GetWrite(const std::string &key, uint32_t idx)const
{
    auto itr = m_writes.find(key);
    if (itr == m_writes.end())
        return sVarEmpty;

    auto itr2 = itr->second.find(idx);
    if (itr2 == itr->second.end())
        return sVarEmpty;

    return itr2->second;
}

void DBMessage::SetRead(const std::string &key, const Variant &v, int idx)
{
    if (key.empty())
        return;

    if (idx < 0)
        idx = m_reads[key].size();

    m_reads[key][idx] = v;
}

void DBMessage::AddRead(const std::string &key, const Variant &v, int idx)
{
    if (key.empty())
        return;

    if (idx < 0)
        idx = m_reads[key].size();

    m_reads[key][idx].Add(v);
}

const Variant &DBMessage::GetRead(const std::string &key, uint32_t idx) const
{
    auto itr = m_reads.find(key);
    if (itr == m_reads.end())
        return sVarEmpty;

    auto itr2 = itr->second.find(idx);
    if (itr2 == itr->second.end())
        return sVarEmpty;

    return itr2->second;
}

void DBMessage::SetCondition(const std::string &key, const Variant &v, int idx)
{
    if (key.empty())
        return;

    if (idx < 0)
        idx = m_conditions[key].size();

    m_conditions[key][idx] = v;
}

void DBMessage::AddCondition(const std::string &key, const Variant &v, int idx)
{
    if (key.empty() || v.IsNull())
        return;

    if (idx < 0)
        idx = m_conditions[key].size();

    m_conditions[key][idx].Add(v);
}

const Variant &DBMessage::GetCondition(const string &key, uint32_t idx, const std::string &ju) const
{
    const Variant & var = getCondition(key, idx);
    if (var.IsNull())
        return getCondition(key+":"+ju, idx);

    return var;
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

int DBMessage::IndexofSql(const std::string &sql) const
{
    int ret = 0;
    for (const string &itr : m_sqls)
    {
        if (sql == itr)
            return ret;

        ret++;
    }
    return -1;
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
    return m_bQueryList!=0;
}

void DBMessage::SetRefFiled(const std::string &filed)
{
    m_refFiled = filed;
}

void DBMessage::SetLimit(uint32_t beg, uint16_t limit)
{
    m_limit = limit;
    m_begLimit = beg;
}

const std::string &DBMessage::GetRefFiled() const
{
    return m_refFiled;
}

DBMessage *DBMessage::GenerateAck(ObjectDB *db) const
{
    if (db && m_ackTp >= DBAckBeging && m_ackTp < DBAckEnd)
    {
        DBMessage *ret = new DBMessage(db, m_ackTp, GetSenderType(), GetSenderID());
        if (!ret)
            return NULL;
        ret->SetSeqNomb(m_seq);
        ret->m_sqls = m_sqls;
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
            for (int i = 0; i < count; ++i)
            {
                const Variant &v = GetRead(INCREASEField, i);
                if (!v.IsNull())
                    ret->SetRead(INCREASEField, v, i);

            }
        }
        ret->m_reads = m_reads;
        return ret;
    }

    return NULL;
}

uint32_t DBMessage::GetLimitBeg() const
{
    return IsQueryList() ? m_begLimit : 0;
}

uint16_t DBMessage::GetLimit() const
{
    return IsQueryList() ? m_limit : 1;
}

bool DBMessage::HasWrite(const std::string &key, uint32_t idx /*= 0*/) const
{
    auto itr = m_writes.find(key);
    if (itr == m_writes.end())
        return false;

    auto itr2 = itr->second.find(idx);
    if (itr2 == itr->second.end())
        return false;

    return true;
}

bool DBMessage::HasCondition(const std::string &key, uint32_t idx /*= 0*/) const
{
    auto itr = m_conditions.find(key);
    if (itr == m_conditions.end())
        return false;

    auto itr2 = itr->second.find(idx);
    if (itr2 == itr->second.end())
        return false;

    return true;
}

void *DBMessage::GetContent() const
{
    return NULL;
}

int DBMessage::GetContentLength() const
{
    return 0;
}

const Variant & DBMessage::getCondition(const std::string &key, uint32_t idx) const
{
    auto itr = m_conditions.find(key);
    if (itr == m_conditions.end())
        return sVarEmpty;

    auto itr2 = itr->second.find(idx);
    if (itr2 == itr->second.end())
        return sVarEmpty;

    return itr2->second;
}