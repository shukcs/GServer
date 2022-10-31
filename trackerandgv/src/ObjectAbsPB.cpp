#include "ObjectAbsPB.h"
#include "socketBase.h"
#include "das.pb.h"
#include "ProtoMsg.h"
#include "Messages.h"
#include "Utility.h"
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
    if (!s)
        return false;

    char buf[256] = { 0 };
    int sz = serialize(msg, buf, 256);
    if (sz > 0)
        return sz==s->Send(sz, buf);

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

void ObjectAbsPB::send(google::protobuf::Message *msg, char *buf, int len)
{
    int sendSz = serialize(*msg, buf, len);
    if (sendSz == Send(buf, sendSz))
        delete msg;
}

void ObjectAbsPB::WaitSend(google::protobuf::Message *msg)
{
    if (msg)
    {
        WaitSin();
        m_protosSend.push(msg);
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
    Message *msg = NULL;

    WaitSin();
    if (GetSendRemain()>0 && Utility::Pop(m_protosSend, msg))
        send(msg, buf, len);
    PostSin();
}

void ObjectAbsPB::CopyAndSend(const google::protobuf::Message &msg)
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
