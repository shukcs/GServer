#include "ObjectAbsPB.h"
#include "socketBase.h"
#include "das.pb.h"
#include "ProtoMsg.h"
#include "Messages.h"
#include "Utility.h"
#include "VGMysql.h"
#include "DBExecItem.h"
#include "DBMessages.h"

using namespace das::proto;
using namespace google::protobuf;
using namespace std;
////////////////////////////////////////////////////////////////////////////////
//ObjectUav
////////////////////////////////////////////////////////////////////////////////
ObjectAbsPB::ObjectAbsPB(const std::string &id): IObject(id)
, ILink(), m_p(NULL)
{
}

ObjectAbsPB::~ObjectAbsPB()
{
    delete m_p;
}

bool ObjectAbsPB::IsConnect() const
{
    return m_bLogined;
}

void ObjectAbsPB::SendProtoBuffTo(ISocket *s, const Message &msg)
{
    if (!s)
        return;

    char buf[256] = { 0 };
    int sz = serialize(msg, buf, 256);
    if (sz>0)
        s->Send(sz + 4, buf);
}

int ObjectAbsPB::ProcessReceive(void *buf, int len)
{
    int pos = 0;
    int l = len;
    while (m_p && l > 18 && m_p->Parse((char*)buf + pos, l))
    {
        pos += l;
        l = len - pos;
        PrcsProtoBuff();
    }
    pos += l;
    return pos;
}

void ObjectAbsPB::OnConnected(bool bConnected)
{
    if (bConnected)
    {
        if (!m_p)
            m_p = new ProtoMsg;
        return;
    }
    ClearRecv();
    m_sock = NULL;
}

void ObjectAbsPB::send(google::protobuf::Message *msg, bool bWait)
{
    if (!bWait)
    {
        char *buf = GetThreadBuff();
        int sendSz = serialize(*msg, buf, GetThreadBuffLength());
        if (sendSz == Send(buf, sendSz))
            delete msg;
        else
            bWait = true;
    }

    if (bWait)
        WaitSend(msg);
}

void ObjectAbsPB::WaitSend(google::protobuf::Message *msg)
{
    if(msg)
        m_protosSend.Push(msg);
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

ILink *ObjectAbsPB::GetHandle()
{
    return this;
}

void ObjectAbsPB::CheckTimer(uint64_t ms)
{
    ILink::CheckTimer(ms);
    if (!m_protosSend.IsEmpty() && m_sock && m_sock->IsNoWriteData())
    {
        Message *msg = m_protosSend.Pop();
        send(msg);
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
    int pos = 0;
    int l = len;
    IObject *o = NULL;
    while (m_p->Parse(buf + pos, l))
    {
        pos += l;
        l = len - pos;
        o = PrcsProtoBuff(s);
    }
    return o;
}

void AbsPBManager::ToCurrntLog(int err, const std::string &obj, int evT, const std::string &dscb)
{
    if (auto msg = new DBMessage(this))
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
