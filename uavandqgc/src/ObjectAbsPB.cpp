#include "ObjectAbsPB.h"
#include "socketBase.h"
#include "das.pb.h"
#include "ProtoMsg.h"
#include "Messages.h"
#include "Utility.h"
#include "VGMysql.h"
#include "DBExecItem.h"

using namespace das::proto;
using namespace google::protobuf;
using namespace std;
#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif
////////////////////////////////////////////////////////////////////////////////
//ObjectUav
////////////////////////////////////////////////////////////////////////////////
ObjectAbsPB::ObjectAbsPB(const std::string &id)
: IObject(NULL, id), m_bConnect(false)
, m_p(NULL)
{
}

ObjectAbsPB::~ObjectAbsPB()
{
    delete m_p;
}

bool ObjectAbsPB::IsConnect() const
{
    return m_bConnect;
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

void ObjectAbsPB::OnConnected(bool bConnected)
{
    m_bConnect = bConnected;
    if (bConnected)
    {
        if (!m_p)
            m_p = new ProtoMsg;

        return;
    }

    ClearRead();
    IObjectManager *mgr = GetManager();
    if (mgr)
    {
        mgr->Log(0, GetObjectID(), 0, "disconnect");
        if (m_sock)
        {
            m_sock->Close();
            m_sock = NULL;
        }
    }
}

bool ObjectAbsPB::send(const google::protobuf::Message &msg)
{
    if (!m_sock)
        return false;

    char *buf = (char *)getThreadBuff();
    int sendSz = serialize(msg, buf, getThreadBuffLen());
    if (sendSz > 0)
    {
        m_sock->Send(sendSz, buf);
        return true;
    }

    return false;
}

int ObjectAbsPB::serialize(const google::protobuf::Message &msg, char*buf, int sz)
{
    if (!buf)
        return 0;

    string name = msg.GetTypeName();
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
