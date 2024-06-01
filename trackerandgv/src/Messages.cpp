#include "Messages.h"
#include <math.h>
#include "das.pb.h"
#include "ProtoMsg.h"
#include "ObjectBase.h"
#include "ObjectGV.h"
#include "GXClient.h"
#include "ObjectTracker.h"
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
//TrackerMessage
////////////////////////////////////////////////////////////////////////////////////////
TrackerMessage::TrackerMessage(const IObject &sender, const std::string &idRcv, int rcv)
    :IMessage(new GSOrUav(sender, Unknown), idRcv, rcv)
{
}

TrackerMessage::TrackerMessage(IObjectManager *sender, const std::string &idRcv, int rcv)
    : IMessage(new GSOrUav(sender, Unknown), idRcv, rcv)
{
}

TrackerMessage::~TrackerMessage()
{
}

void *TrackerMessage::GetContent() const
{
    return GetProtobuf();
}

int TrackerMessage::GetContentLength() const
{
    return m_data ? ((GSOrUav*)m_data)->GetProtoBuf()->ByteSize() : 0;
}

void TrackerMessage::AttachProto(google::protobuf::Message *msg)
{
    if (!msg || !m_data)
        return;

    ((GSOrUav*)m_data)->SetProtoBuf(msg);
    SetMessgeType(getMessageType(*msg));
}

Message *TrackerMessage::GetProtobuf()const
{
    return m_data ? ((GSOrUav*)m_data)->GetProtoBuf() : NULL;
}

void TrackerMessage::SetPBContent(const Message &msg)
{
    Message *msgTmp = msg.New();
    _copyMsg(msgTmp, msg);
}

bool TrackerMessage::IsGSUserValide(const std::string &user)
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

void TrackerMessage::_copyMsg(Message *c, const Message &msg)
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
////////////////////////////////////////////////////////////////////////////////////
//Tracker2GVMessage
////////////////////////////////////////////////////////////////////////////////////
Tracker2GVMessage::Tracker2GVMessage(const ObjectTracker &sender, const std::string &idRcv)
    :TrackerMessage(sender, idRcv, ObjectGV::GVType())
{
}

Tracker2GVMessage::Tracker2GVMessage(IObjectManager *sender, const std::string &idRcv)
    : TrackerMessage(sender, idRcv, ObjectGV::GVType())
{
}

IMessage::MessageType Tracker2GVMessage::getMessageType(const Message &msg)
{
    MessageType ret = Unknown;
    const string &name = msg.GetDescriptor()->full_name();
    if (name == d_p_ClassName(PostOperationInformation))
        ret = PushUavSndInfo;
    else if (name == d_p_ClassName(AckIdentityAllocation))
        ret = DeviceAllocationRslt;
    else if (name == d_p_ClassName(AckSyncDeviceList))
        ret = IMessage::SyncDeviceisRslt;
    else if (name == d_p_ClassName(AckQueryParameters) || name == d_p_ClassName(AckConfigurParameters))
        ret = IMessage::ControlUser;

    return ret;
}
/////////////////////////////////////////////////////////////////////////////
//GV2TrackerMessage
/////////////////////////////////////////////////////////////////////////////
GV2TrackerMessage::GV2TrackerMessage(const ObjectGV &sender, const std::string &idRcv)
    :TrackerMessage(sender, idRcv, ObjectTracker::TrackerType())
{
}

GV2TrackerMessage::GV2TrackerMessage(IObjectManager *sender, const std::string &idRcv)
    : TrackerMessage(sender, idRcv, ObjectTracker::TrackerType())
{
}

IMessage::MessageType GV2TrackerMessage::getMessageType(const google::protobuf::Message &msg)
{
    MessageType ret = Unknown;
    const string &name = msg.GetDescriptor()->full_name();
   
    if (name == d_p_ClassName(RequestIdentityAllocation))
        ret = DeviceAllocation;
    else if (name == d_p_ClassName(QueryParameters) || name == d_p_ClassName(ConfigureParameters))
        ret = IMessage::ControlDevice;
    else if (name == d_p_ClassName(SyncDeviceList))
        ret = IMessage::SyncDeviceis;

    return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//Tracker2GXMessage
/////////////////////////////////////////////////////////////////////////////////////////////
Tracker2GXMessage::Tracker2GXMessage(const ObjectTracker &sender)
: TrackerMessage(sender, string(), GXClient::GXClientType())
{
}

Tracker2GXMessage::Tracker2GXMessage(IObjectManager *sender)
: TrackerMessage(sender, string(), GXClient::GXClientType())
{
}

IMessage::MessageType Tracker2GXMessage::getMessageType(const google::protobuf::Message &msg)
{
    MessageType ret = Unknown;
    const string &name = msg.GetDescriptor()->full_name();
    if (name == d_p_ClassName(PostOperationInformation))
        ret = PushUavSndInfo;

    return ret;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//GX2TrackerMessage
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
GX2TrackerMessage::GX2TrackerMessage(const std::string &sender, int st)
:IMessage(new MessageData(sender, GXClient::GXClientType(), GXClinetStat), sender, ObjectTracker::TrackerType())
, m_stat(st)
{
}

int GX2TrackerMessage::GetStat() const
{
    return m_stat;
}

void * GX2TrackerMessage::GetContent() const
{
    return NULL;
}

int GX2TrackerMessage::GetContentLength() const
{
    return 0;
}
