#include "Messages.h"
#include <math.h>
#include "das.pb.h"
#include "ProtoMsg.h"
#include "ObjectBase.h"
#include "ObjectGS.h"
#include "ObjectUav.h"
#include "GXClient.h"
#include "Utility.h"

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

class GSOrUav : public MessageData
{
public:
    GSOrUav(IObject *sender, int tpMs)
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
GSOrUavMessage::GSOrUavMessage(IObject *sender, const std::string &idRcv, int rcv)
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
//Uav2GSMessage
/////////////////////////////////////////////////////////////////////////////
Uav2GSMessage::Uav2GSMessage(ObjectUav *sender, const std::string &idRcv)
    :GSOrUavMessage(sender, idRcv, IObject::GroundStation)
{
}

Uav2GSMessage::Uav2GSMessage(IObjectManager *sender, const std::string &idRcv)
    : GSOrUavMessage(sender, idRcv, IObject::GroundStation)
{
}

int Uav2GSMessage::getMessageType(const Message &msg)
{
    MessageType ret = Unknown;
    const string &name = msg.GetDescriptor()->full_name();
    if (name == d_p_ClassName(AckRequestBindUav))
        ret = BindUavRslt;
    else if (name == d_p_ClassName(AckPostControl2Uav))
        ret = ControlDeviceRslt;
    else if (name == d_p_ClassName(SyscOperationRoutes))
        ret = SychMissionRslt;
    else if (name == d_p_ClassName(AckPostOperationRoute))
        ret = PostORRslt;
    else if (name == d_p_ClassName(PostOperationInformation))
        ret = PushUavSndInfo;
    else if (name == d_p_ClassName(PostStatus2GroundStation))
        ret = ControlUser;
    else if (name == d_p_ClassName(AckRequestUavStatus))
        ret = QueryDeviceRslt;
    else if (name == d_p_ClassName(AckIdentityAllocation))
        ret = DeviceAllocationRslt;

    return ret;
}
/////////////////////////////////////////////////////////////////////////////
//GS2UavMessage
/////////////////////////////////////////////////////////////////////////////
GS2UavMessage::GS2UavMessage(ObjectGS *sender, const std::string &idRcv)
    :GSOrUavMessage(sender, idRcv, IObject::Plant), m_auth(0)
{
    if (sender)
        m_auth = sender->Authorize();
}

GS2UavMessage::GS2UavMessage(IObjectManager *sender, const std::string &idRcv)
    : GSOrUavMessage(sender, idRcv, IObject::Plant), m_auth(0)
{
}

int GS2UavMessage::GetAuth() const
{
    return m_auth;
}

int GS2UavMessage::getMessageType(const google::protobuf::Message &msg)
{
    MessageType ret = Unknown;
    const string &name = msg.GetDescriptor()->full_name();
    if (name == d_p_ClassName(RequestBindUav))
        ret = BindUav;
    else if (name == d_p_ClassName(PostOperationRoute))
        ret = PostOR;
    else if (name == d_p_ClassName(PostControl2Uav))
        ret = ControlDevice;
    else if (name == d_p_ClassName(RequestRouteMissions))
        ret = SychMission;
    else if (name == d_p_ClassName(RequestUavStatus))
        ret = QueryDevice;
    else if (name == d_p_ClassName(RequestIdentityAllocation))
        ret = DeviceAllocation;
    else if (name == d_p_ClassName(RequestIdentityAllocation))
        ret = DeviceAllocation;
    else if (name == d_p_ClassName(NotifyProgram))
        ret = NotifyFWUpdate;

    return ret;
}

/////////////////////////////////////////////////////////////////////////////
//GS2UavMessage
/////////////////////////////////////////////////////////////////////////////
Gs2GsMessage::Gs2GsMessage(ObjectGS *sender, const std::string &idRcv)
: GSOrUavMessage(sender, idRcv, IObject::GroundStation)
{
}

Gs2GsMessage::Gs2GsMessage(IObjectManager *sender, const std::string &idRcv)
    : GSOrUavMessage(sender, idRcv, IObject::GroundStation)
{
}

int Gs2GsMessage::getMessageType(const google::protobuf::Message &msg)
{
    MessageType ret = Unknown;
    const string &name = msg.GetDescriptor()->full_name();
    if (name == d_p_ClassName(GroundStationsMessage))
        ret = User2User;
    if (name == d_p_ClassName(AckGroundStationsMessage))
        ret = User2UserAck;

    return ret;
}
/////////////////////////////////////////////////////////////////////////
//Uav2GXMessage
/////////////////////////////////////////////////////////////////////////
Uav2GXMessage::Uav2GXMessage(ObjectUav *sender)
: GSOrUavMessage(sender, string(), GXClient::GXClientType())
{
}

int Uav2GXMessage::getMessageType(const google::protobuf::Message &msg)
{
    int ret = Unknown;
    const string &name = msg.GetDescriptor()->full_name();
    if (name == d_p_ClassName(RequestUavIdentityAuthentication))
        ret = Authentication;
    else if (name == d_p_ClassName(PostOperationInformation))
        ret = PushUavSndInfo;

    return ret;
}
/////////////////////////////////////////////////////////////////////////
//GX2UavMessage
/////////////////////////////////////////////////////////////////////////
GX2UavMessage::GX2UavMessage(const std::string &sender, int st)
:IMessage(new MessageData(sender, GXClient::GXClientType(), GXClinetStat), sender, ObjectUav::UAVType())
, m_stat(st)
{
}

int GX2UavMessage::GetStat() const
{
    return m_stat;
}

void *GX2UavMessage::GetContent() const
{
    return NULL;
}

int GX2UavMessage::GetContentLength() const
{
    return 0;
}
