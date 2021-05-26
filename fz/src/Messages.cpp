#include "Messages.h"
#include <math.h>
#include "das.pb.h"
#include "ProtoMsg.h"
#include "ObjectBase.h"
#include "ObjectVgFZ.h"
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

const map<string, int> &getProtoTypes(const string &className)
{
    static map<string, map<string, int>> sProtoType;
    if (sProtoType.empty())
    {
        map<string, int> &sGsProtoType = sProtoType[SymbolString(GS2UavMessage)];
        /*sGsProtoType[d_p_ClassName(RequestBindUav)] = IMessage::BindUav;
        sGsProtoType[d_p_ClassName(PostOperationRoute)] = IMessage::PostOR;
        sGsProtoType[d_p_ClassName(PostControl2Uav)] = IMessage::ControlDevice;
        sGsProtoType[d_p_ClassName(RequestRouteMissions)] = IMessage::SychMission;
        sGsProtoType[d_p_ClassName(RequestUavStatus)] = IMessage::QueryDevice;
        sGsProtoType[d_p_ClassName(RequestIdentityAllocation)] = IMessage::DeviceAllocation;
        sGsProtoType[d_p_ClassName(RequestIdentityAllocation)] = IMessage::DeviceAllocation;
        sGsProtoType[d_p_ClassName(NotifyProgram)] = IMessage::NotifyFWUpdate;
        sGsProtoType[d_p_ClassName(RequestOperationAssist)] = IMessage::ControlDevice2;
        sGsProtoType[d_p_ClassName(RequestABPoint)] = IMessage::ControlDevice2;
        sGsProtoType[d_p_ClassName(RequestOperationReturn)] = IMessage::ControlDevice2;

        map<string, int> &sUavProtoType = sProtoType[SymbolString(Uav2GSMessage)];
        sUavProtoType[d_p_ClassName(AckRequestBindUav)] = IMessage::BindUavRslt;
        sUavProtoType[d_p_ClassName(AckPostControl2Uav)] = IMessage::ControlDeviceRslt;
        sUavProtoType[d_p_ClassName(SyscOperationRoutes)] = IMessage::SychMissionRslt;
        sUavProtoType[d_p_ClassName(AckPostOperationRoute)] = IMessage::PostORRslt;
        sUavProtoType[d_p_ClassName(PostOperationInformation)] = IMessage::PushUavSndInfo;
        sUavProtoType[d_p_ClassName(PostStatus2GroundStation)] = IMessage::ControlUser;
        sUavProtoType[d_p_ClassName(PostOperationAssist)] = IMessage::ControlUser;
        sUavProtoType[d_p_ClassName(PostABPoint)] = IMessage::ControlUser;
        sUavProtoType[d_p_ClassName(PostOperationReturn)] = IMessage::ControlUser;
        sUavProtoType[d_p_ClassName(AckRequestUavStatus)] = IMessage::QueryDeviceRslt;
        sUavProtoType[d_p_ClassName(AckIdentityAllocation)] = IMessage::DeviceAllocationRslt;
        sUavProtoType[d_p_ClassName(PostBlocks)] = IMessage::ControlUser;

        map<string, int> &sGs2GsProtoType = sProtoType[SymbolString(Gs2GsMessage)];
        sGs2GsProtoType[d_p_ClassName(GroundStationsMessage)] = IMessage::User2User;
        sGs2GsProtoType[d_p_ClassName(AckGroundStationsMessage)] = IMessage::User2UserAck;

        map<string, int> &sGs2GXProtoType = sProtoType[SymbolString(Uav2GXMessage)];
        sGs2GXProtoType[d_p_ClassName(RequestUavIdentityAuthentication)] = IMessage::Authentication;
        sGs2GXProtoType[d_p_ClassName(PostOperationInformation)] = IMessage::PushUavSndInfo;*/
    }

    return sProtoType[className];
}
//////////////////////////////////////////////////////////////////
//GSOrUav
//////////////////////////////////////////////////////////////////
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
Gs2GsMessage::Gs2GsMessage(ObjectVgFZ *sender, const std::string &idRcv)
: GSOrUavMessage(sender, idRcv, IObject::GroundStation)
{
}

Gs2GsMessage::Gs2GsMessage(IObjectManager *sender, const std::string &idRcv)
    : GSOrUavMessage(sender, idRcv, IObject::GroundStation)
{
}
