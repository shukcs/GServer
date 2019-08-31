#include "Messages.h"
#include "das.pb.h"
#include "ProtoMsg.h"
#include "ObjectBase.h"

using namespace google::protobuf;
using namespace das::proto;
using namespace std;

/////////////////////////////////////////////////////////////////////////////
//GSMessage
/////////////////////////////////////////////////////////////////////////////
GSMessage::GSMessage(IObject *sender, const std::string &idRcv)
    :IMessage(sender, idRcv, IObject::GroundStation, Unknown), m_msg(NULL)
{
}

GSMessage::GSMessage(IObjectManager *sender, const std::string &idRcv)
    : IMessage(sender, idRcv, IObject::GroundStation, Unknown), m_msg(NULL)
{
}

GSMessage::~GSMessage()
{
    delete m_msg;
}

void * GSMessage::GetContent() const
{
    return m_msg;
}

void GSMessage::SetGSContent(const Message &msg)
{
    delete m_msg;
    m_msg = NULL;
    m_tpMsg = getMessageType(msg);
    switch (m_tpMsg)
    {
    case BindUavRes:
        m_msg = new AckRequestBindUav;
        break;
    case ControlUavRes:
        m_msg = new AckPostControl2Uav;
        break;
    case SychMissionRes:
        m_msg = new AckRequestUploadOperationRoutes;
        break;
    case PushUavSndInfo:
        m_msg = new PostOperationInformation;
        break;
    case ControlGs:
        m_msg = new PostStatus2GroundStation;
        break;
    case QueryUavRes:
        m_msg = new AckRequestUavStatus;
        break;
    case UavAllocationRes:
        m_msg = new AckIdentityAllocation;
        break;
    default:
        break;
    }

    if (m_msg && m_tpMsg != Unknown)
        m_msg->CopyFrom(msg);
}

void GSMessage::AttachProto(google::protobuf::Message *msg)
{
    delete m_msg;
    m_msg = NULL;
    if (!msg)
        return;

    m_tpMsg = getMessageType(*msg);
    m_msg = msg;
}

int GSMessage::GetContentLength() const
{
    return m_msg ? m_msg->ByteSize() : 0;
}

MessageType GSMessage::getMessageType(const Message &msg)
{
    MessageType ret = Unknown;
    const string &name = msg.GetTypeName();
    if (name == d_p_ClassName(AckRequestBindUav))
        ret = BindUavRes;
    else if (name == d_p_ClassName(AckPostControl2Uav))
        ret = ControlUavRes;
    else if (name == d_p_ClassName(AckRequestUploadOperationRoutes))
        ret = SychMissionRes;
    else if (name == d_p_ClassName(PostOperationInformation))
        ret = PushUavSndInfo;
    else if (name == d_p_ClassName(PostStatus2GroundStation))
        ret = ControlGs;
    else if (name == d_p_ClassName(AckRequestUavStatus))
        ret = QueryUavRes;
    else if (name == d_p_ClassName(AckIdentityAllocation))
        ret = UavAllocationRes;

    return ret;
}
/////////////////////////////////////////////////////////////////////////////
//UAVMessage
/////////////////////////////////////////////////////////////////////////////
UAVMessage::UAVMessage(IObject *sender, const std::string &idRcv)
    :IMessage(sender, idRcv, IObject::Plant, Unknown), m_msg(NULL)
{
}

UAVMessage::UAVMessage(IObjectManager *sender, const std::string &idRcv)
    : IMessage(sender, idRcv, IObject::Plant, Unknown), m_msg(NULL)
{
}

UAVMessage::~UAVMessage()
{
    delete m_msg;
}

void * UAVMessage::GetContent() const
{
    return m_msg;
}

void UAVMessage::SetContent(const google::protobuf::Message &msg)
{
    delete m_msg;
    m_msg = NULL;
    m_tpMsg = getMessageType(msg);
    switch (m_tpMsg)
    {
    case BindUav:
        m_msg = new RequestBindUav;
        break;
    case ControlUav:
        m_msg = new PostControl2Uav;
        break;
    case SychMission:
        m_msg = new RequestUploadOperationRoutes;
        break;
    case QueryUav:
        m_msg = new RequestUavStatus;
        break;
    case UavAllocation:
        m_msg = new RequestIdentityAllocation;
        break;
    default:
        break;
    }

    if (m_msg && m_tpMsg != Unknown)
        m_msg->CopyFrom(msg);
}

void UAVMessage::AttachProto(google::protobuf::Message *msg)
{
    delete m_msg;
    m_msg = NULL;
    if (!msg)
        return;
    
    m_tpMsg = getMessageType(*msg);
    m_msg = msg;
}

int UAVMessage::GetContentLength() const
{
    return m_msg ? m_msg->ByteSize() : 0;
}

MessageType UAVMessage::getMessageType(const google::protobuf::Message &msg)
{
    MessageType ret = Unknown;
    const string &name = msg.GetTypeName();
    if (name == d_p_ClassName(RequestBindUav))
        ret = BindUav;
    else if (name == d_p_ClassName(PostControl2Uav))
        ret = ControlUav;
    else if (name == d_p_ClassName(RequestUploadOperationRoutes))
        ret = SychMission;
    else if (name == d_p_ClassName(RequestUavStatus))
        ret = QueryUav;
    else if (name == d_p_ClassName(RequestIdentityAllocation))
        ret = UavAllocation;

    return ret;
}