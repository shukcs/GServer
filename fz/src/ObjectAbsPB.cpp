#include "ObjectAbsPB.h"
#include "socketBase.h"
#include "das.pb.h"
#include "ProtoMsg.h"
#include "Messages.h"
#include "Utility.h"
#include "VGMysql.h"
#include "DBExecItem.h"
#include "DBMessages.h"
#include "Lock.h"

using namespace das::proto;
using namespace google::protobuf;
using namespace std;
////////////////////////////////////////////////////////////////////////////////
//ObjectUav
////////////////////////////////////////////////////////////////////////////////
ObjectAbsPB::ObjectAbsPB(const std::string &id): IObject(id)
, ILink(), m_p(new ProtoMsg)
{
}

ObjectAbsPB::~ObjectAbsPB()
{
    delete m_p;
}

bool ObjectAbsPB::SendProtoBuffTo(ISocket *s, const Message &msg)
{
    if (s)
    {
        char buf[256] = { 0 };
        int sz = serialize(msg, buf, 256);
        if (sz > 0)
            return sz == s->Send(sz, buf);
    }

    return false;
}

int ObjectAbsPB::ProcessReceive(void *buf, int len, uint64_t ms)
{
    int pos = 0;
    uint32_t l = len;
    while (m_p && l > 18 && m_p->Parse((char*)buf + pos, l))
    {
        pos += l;
        l = len - pos;
        PrcsProtoBuff(ms);
    }
    pos += l;
    return pos;
}

void ObjectAbsPB::send(char *buf, int len)
{
    int lenSnd = GetSendRemain();
    if (lenSnd < 1)
        return;

    for (auto itr = m_protosList.begin(); itr != m_protosList.end(); )
    {
        int sendSz = serialize(**itr, buf, len);
        lenSnd -= sendSz;
        if (lenSnd < 0)
            return;

        Send(buf, sendSz);
        delete *itr;
        itr = m_protosList.erase(itr);
    }
    WaitSin();
    for (auto itr = m_protosMap.begin(); itr != m_protosMap.end(); ++itr)
    {
        if (itr->second)
            continue;

        int sendSz = serialize(*itr->first, buf, len);
        lenSnd -= sendSz;
        if (lenSnd < 0)
            break;
        if (sendSz == Send(buf, sendSz))
            itr->second = true;
    }
    PostSin();
}

void ObjectAbsPB::clearProto()
{
    for (auto itr = m_protosList.begin(); itr != m_protosList.end(); )
    {
        delete *itr;
        itr = m_protosList.erase(itr);
    }

    WaitSin();
    for (auto itr = m_protosMap.begin(); itr != m_protosMap.end(); ++itr)
    {
        itr->second = true;
    }
    PostSin();
}

void ObjectAbsPB::WaitSend(google::protobuf::Message *msg)
{
    if (!msg)
        return;

    if (IsLinkThread())
    {
        m_protosList.push_back(msg);
    }
    else 
    {
        WaitSin();
        for (auto itr = m_protosMap.begin(); itr != m_protosMap.end(); )
        {
            if (itr->second)
            {
                delete itr->first;
                itr = m_protosMap.erase(itr);
            }
            else
            {
                ++itr;
            }
        }
        m_protosMap[msg] = false;
        PostSin();
    }
}

int ObjectAbsPB::serialize(const google::protobuf::Message &msg, char*buf, int sz)
{
    if (!buf)
        return 0;

    const string &name = msg.GetDescriptor()->full_name();
    if (name.length() < 1)
        return 0;
    int nameLen = name.length() + 1;
    int proroLen = msg.ByteSize();
    int len = nameLen + proroLen + 8;
    if (sz < len + 4)
        return 0;

    Utility::toBigendian(len, buf);
    Utility::toBigendian(nameLen, buf + 4);
    strcpy(buf + 8, name.c_str());
    msg.SerializeToArray(buf + nameLen + 8, proroLen);
    int crc = Utility::Crc32(buf + 4, len - 4);
    Utility::toBigendian(crc, buf + len);
    return len + 4;
}

IObject *ObjectAbsPB::GetParObject()
{
    return this;
}

ILink *ObjectAbsPB::GetLink()
{
    return this;
}

void ObjectAbsPB::CheckTimer(uint64_t ms, char *buf, int len)
{
    ILink::CheckTimer(ms, buf, len);
    if (!GetSocket())
    {
        clearProto();
        return;
    }
    send(buf, len);
}

void ObjectAbsPB::CopyAndSend(const Message &msg)
{
    if (!IsConnect())
        return;

    if (Message *ms = msg.New())
    {
        ms->CopyFrom(msg);
        WaitSend(ms);
    }
}
////////////////////////////////////////////////////////////////////////////////
//AbsPBManager
////////////////////////////////////////////////////////////////////////////////
AbsPBManager::AbsPBManager():m_p(new ProtoMsg)
{
}

AbsPBManager::~AbsPBManager()
{
    delete m_p;
}

IObject *AbsPBManager::PrcsNotObjectReceive(ISocket *s, const char *buf, int len)
{
    uint32_t pos = 0;
    uint32_t l = len;
    IObject *o = NULL;
    while (m_p->Parse(buf + pos, l))
    {
        pos += l;
        l = len - pos;
        o = PrcsProtoBuff(s);
        if (o)
            break;
    }

    return o;
}

void AbsPBManager::ToCurrntLog(int err, const std::string &obj, int evT, const std::string &dscb)
{
    if (auto msg = new DBMessage(this, "DBLog"))
    {
        msg->SetSql("insertLog");

        msg->SetWrite("event", evT);
        msg->SetWrite("error", err);
        msg->SetWrite("object", obj);
        msg->SetWrite("evntdesc", dscb);
        if (!SendMsg(msg))
            delete msg;
    }
}
