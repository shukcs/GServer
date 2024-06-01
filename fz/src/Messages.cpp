#include "Messages.h"
#include <math.h>
#include "das.pb.h"
#include "ProtoMsg.h"
#include "ObjectBase.h"
#include "ObjectVgFZ.h"
#include "common/Utility.h"

using namespace google::protobuf;
using namespace das::proto;
using namespace std;
#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif

enum 
{
    Numb_Min = '0',
    Numb_Max = '9',
    Char_Space = '_',
    Char_Min = 'a',
    Char_Max = 'z',
};

const map<string, int> &getProtoTypes(const string &className)
{
    static map<string, map<string, int>> sProtoType;
    if (sProtoType.empty())
    {
        map<string, int> &sGs2GsProtoType = sProtoType[SymbolString(FZ2FZMessage)];
        sGs2GsProtoType[d_p_ClassName(FZUserMessage)] = IMessage::User2User;
        sGs2GsProtoType[d_p_ClassName(AckFZUserMessage)] = IMessage::User2UserAck;
    }

    return sProtoType[className];
}
//////////////////////////////////////////////////////////////////
//GSOrUav
//////////////////////////////////////////////////////////////////
class GSOrUav : public MessageData
{
public:
    GSOrUav(const IObject &sender, int tpMs)
        : MessageData(sender, tpMs), m_msg(NULL)
    {
    }
    GSOrUav(IObjectManager *sender, int tpMs)
        : MessageData(sender, tpMs), m_msg(NULL)
    {
    }
    ~GSOrUav()
    {
        delete m_msg;
    }
    Message *GetProtoBuf()const
    {
        return m_msg;
    }
    void SetProtoBuf(Message *pb)
    {
        delete m_msg;
        m_msg = pb;
    }
private:
    Message  *m_msg;
};
////////////////////////////////////////////////////////////////////////////////////////
//GSOrUavMessage
////////////////////////////////////////////////////////////////////////////////////////
GSOrUavMessage::GSOrUavMessage(const IObject &sender, const std::string &idRcv, int rcv)
    :IMessage(new GSOrUav(sender, Unknown), idRcv, rcv)
{
}

GSOrUavMessage::GSOrUavMessage(IObjectManager *sender, const std::string &idRcv, int rcv)
    : IMessage(new GSOrUav(sender, Unknown), idRcv, rcv)
{
}

GSOrUavMessage::~GSOrUavMessage()
{
}

void *GSOrUavMessage::GetContent() const
{
    return GetProtobuf();
}

int GSOrUavMessage::GetContentLength() const
{
    return m_data ? ((GSOrUav*)m_data)->GetProtoBuf()->ByteSize() : 0;
}

void GSOrUavMessage::AttachProto(google::protobuf::Message *msg)
{
    if (!msg || !m_data)
        return;

    ((GSOrUav*)m_data)->SetProtoBuf(msg);
    SetMessgeType(getMessageType(*msg));
}

Message *GSOrUavMessage::GetProtobuf()const
{
    return m_data ? ((GSOrUav*)m_data)->GetProtoBuf() : NULL;
}

void GSOrUavMessage::SetPBContent(const Message &msg)
{
    Message *msgTmp = msg.New();
    _copyMsg(msgTmp, msg);
}

bool GSOrUavMessage::IsGSUserValide(const std::string &user)
{
    size_t count = user.length();
    if (count < 3 || count > 24)
        return false;

    const uint8_t *tmp = (uint8_t *)user.c_str();
    for (size_t i = 0; i < count; ++i)
    {
        if (0==i)
        {
            if (tmp[i] < Char_Min || tmp[i]>Char_Max)
                return false;
        }
        else if ( tmp[i] != Char_Space
               && (tmp[i] < Char_Min || tmp[i] > Char_Max)
               && (tmp[i] < Numb_Min || tmp[i] > Numb_Max) )
        {
            return false;
        }
    }
    return true;
}

int GSOrUavMessage::getMessageType(const google::protobuf::Message &msg)
{
    const map<string, int> &protoType = getProtoTypes(_className());
    auto itr = protoType.find(msg.GetDescriptor()->full_name());
    if (itr != protoType.end())
        return itr->second;

    return Unknown;
}

void GSOrUavMessage::_copyMsg(Message *c, const Message &msg)
{
    SetMessgeType(getMessageType(msg));
    if (c && Unknown!=GetMessgeType())
    {
        c->CopyFrom(msg);
        ((GSOrUav*)m_data)->SetProtoBuf(c);
        return;
    }

    ReleasePointer(c);
}

/////////////////////////////////////////////////////////////////////////////
//GS2UavMessage
/////////////////////////////////////////////////////////////////////////////
FZ2FZMessage::FZ2FZMessage(const ObjectVgFZ &sender, const std::string &idRcv)
: GSOrUavMessage(sender, idRcv, IObject::VgFZ)
{
}

FZ2FZMessage::FZ2FZMessage(IObjectManager *sender, const std::string &idRcv)
    : GSOrUavMessage(sender, idRcv, IObject::VgFZ)
{
}
