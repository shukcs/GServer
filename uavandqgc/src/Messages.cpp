#include "Messages.h"
#include "das.pb.h"
#include "ProtoMsg.h"

#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif
using namespace google::protobuf;
using namespace das::proto;
using namespace std;
////////////////////////////////////////////////////////////////////////////////
//UAVMessage
////////////////////////////////////////////////////////////////////////////////
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

void *UAVMessage::GetContent() const
{
    return m_msg;
}

void UAVMessage::SetContent(const google::protobuf::Message &msg)
{
    delete m_msg;
    m_msg = NULL;
    m_tpMsg = Unknown;
    const string &name = msg.GetTypeName();
    if (name == d_p_ClassName(RequestBindUav))
    {
        m_msg = new RequestBindUav;
        m_tpMsg = BindUav;
    }
    else if (name == d_p_ClassName(PostControl2Uav))
    {
        m_msg = new PostControl2Uav;
        m_tpMsg = ControlUav;
    }
    else if (name == d_p_ClassName(RequestUploadOperationRoutes))
    {
        m_msg = new RequestUploadOperationRoutes;
        m_tpMsg = SychMission;
    }
    else if (name == d_p_ClassName(RequestUavStatus))
    {
        m_msg = new RequestUavStatus;
        m_tpMsg = QueryUav;
    }

    if (m_tpMsg != Unknown)
        m_msg->CopyFrom(msg);
}

void UAVMessage::AttachProto(google::protobuf::Message *msg)
{
    delete m_msg;
    m_msg = NULL;
    if (!msg)
        return;
    const string &name = msg->GetTypeName();
    if (name == d_p_ClassName(RequestBindUav))
        m_tpMsg = BindUav;
    else if (name == d_p_ClassName(PostControl2Uav))
        m_tpMsg = ControlUav;
    else if (name == d_p_ClassName(RequestUploadOperationRoutes))
        m_tpMsg = SychMission;
    else if (name == d_p_ClassName(RequestUavStatus))
        m_tpMsg = QueryUav;
    else
        m_tpMsg = Unknown;

    m_msg = msg;
}

int UAVMessage::GetContentLength() const
{
    return m_msg ? m_msg->ByteSize() : 0;
}

////////////////////////////////////////////////////////////////////////////////
//GSMessage
////////////////////////////////////////////////////////////////////////////////
GSMessage::GSMessage(IObject *sender, const std::string &idRcv)
:IMessage(sender, idRcv, IObject::GroundStation, Unknown), m_msg(NULL)
{
}

GSMessage::GSMessage(IObjectManager *sender, const std::string &idRcv)
:IMessage(sender, idRcv, IObject::GroundStation, Unknown), m_msg(NULL)
{
}

GSMessage::~GSMessage()
{
    delete m_msg;
}

void *GSMessage::GetContent() const
{
    return m_msg;
}

void GSMessage::SetContent(const google::protobuf::Message &msg)
{
    delete m_msg;
    m_msg = NULL;
    m_tpMsg = Unknown;
    const string &name = msg.GetTypeName();
    if (name == d_p_ClassName(AckRequestBindUav))
    {
        m_msg = new AckRequestBindUav;
        m_tpMsg = BindUavRes;
    }
    else if (name == d_p_ClassName(AckPostControl2Uav))
    {
        m_msg = new AckPostControl2Uav;
        m_tpMsg = ControlUavRes;
    }
    else if (name == d_p_ClassName(AckRequestUploadOperationRoutes))
    {
        m_msg = new AckRequestUploadOperationRoutes;
        m_tpMsg = SychMissionRes;
    }
    else if (name == d_p_ClassName(OperationInformation))
    {
        m_msg = new OperationInformation;
        m_tpMsg = PushUavSndInfo;
    }
    else if (name == d_p_ClassName(PostStatus2GroundStation))
    {
        m_msg = new OperationInformation;
        m_tpMsg = ControlGs;
    }
    else if (name == d_p_ClassName(AckRequestUavStatus))
    {
        m_msg = new AckRequestUavStatus;
        m_tpMsg = QueryUavRes;
    }

    if (m_tpMsg != Unknown)
        m_msg->CopyFrom(msg);
}

void GSMessage::AttachProto(google::protobuf::Message *msg)
{
    delete m_msg;
    m_msg = NULL;
    if (!msg)
        return;
    const string &name = msg->GetTypeName();
    if (name == d_p_ClassName(AckRequestBindUav))
        m_tpMsg = BindUavRes;
    else if (name == d_p_ClassName(AckPostControl2Uav))
        m_tpMsg = ControlUavRes;
    else if (name == d_p_ClassName(AckRequestUploadOperationRoutes))
        m_tpMsg = SychMissionRes;
    else if (name == d_p_ClassName(OperationInformation))
        m_tpMsg = PushUavSndInfo;
    else if (name == d_p_ClassName(PostStatus2GroundStation))
        m_tpMsg = ControlGs;
    else if (name == d_p_ClassName(AckRequestUavStatus))
        m_tpMsg = QueryUavRes;
    else
        m_tpMsg = Unknown;

    m_msg = msg;
}

int GSMessage::GetContentLength() const
{
    return m_msg ? m_msg->ByteSize() : 0;
}
