#include "Messages.h"
#include <math.h>
#include "das.pb.h"
#include "ProtoMsg.h"
#include "ObjectBase.h"
#include "Utility.h"

using namespace google::protobuf;
using namespace das::proto;
using namespace std;

static const char *sRandStrTab = "QWERTYUIOPASDFGHJKLZXCVBNMqwertyuiopasdfghjklzxcvbnm0123456789";
////////////////////////////////////////////////////////////////////////////////////////
//GSOrUavMessage
////////////////////////////////////////////////////////////////////////////////////////
GSOrUavMessage::GSOrUavMessage(IObject *sender, const std::string &idRcv, int rcv)
    :IMessage(sender, idRcv, rcv, Unknown), m_msg(NULL)
{
}

GSOrUavMessage::GSOrUavMessage(IObjectManager *sender, const std::string &idRcv, int rcv)
    : IMessage(sender, idRcv, rcv, Unknown), m_msg(NULL)
{
}

GSOrUavMessage::~GSOrUavMessage()
{
    delete m_msg;
}

void *GSOrUavMessage::GetContent() const
{
    return m_msg;
}

int GSOrUavMessage::GetContentLength() const
{
    return m_msg ? m_msg->ByteSize() : 0;
}

void GSOrUavMessage::AttachProto(google::protobuf::Message *msg)
{
    delete m_msg;
    m_msg = NULL;
    if (!msg)
        return;

    m_tpMsg = getMessageType(*msg);
    m_msg = msg;
}

string GSOrUavMessage::GenCheckString(int len)
{
    if (len > 16)
        return string();

    int64_t tm = rand()+Utility::usTimeTick();
    char ret[17] = { 0 };
    uint32_t n = tm / 1023;
    char c = '0' + n % 10;
    ret[n % 6] = c;
    n = tm / 0xffff;
    c = '0' + n % 10;
    n %= 6;
    if (ret[n] == 0)
        ret[n] = c;
    else if(n<5)
        ret[n+1] = c;
    else
        ret[n-1] = c;

    for (int i = 0; i < len; ++i)
    {
        if (ret[i] == 0)
            ret[i] = sRandStrTab[tm / (19 + i) % 62];
    }
    return ret;
}

bool GSOrUavMessage::IsGSUserValide(const std::string &user)
{
    size_t count = user.length();
    if (count==0 || count > 24)
        return false;

    const uint8_t *tmp = (uint8_t *)user.c_str();
    for (size_t i = 0; i < count; ++i)
    {
        if (tmp[i] >= 'A' && tmp[i] <= 'Z')
            continue;
        if (tmp[i] >= 'a' && tmp[i] <= 'z')
            continue;
        if (tmp[i] >= '0' && tmp[i] <= '9')
            continue;

        if(i == 0 || tmp[i] != '_')
            return false;
    }
    return true;
}

void GSOrUavMessage::_copyMsg(const Message &msg)
{
    m_tpMsg = getMessageType(msg);
    if (m_msg && Unknown != m_tpMsg)
        m_msg->CopyFrom(msg);
    else if (m_tpMsg)
        ReleasePointer(m_msg);
}
/////////////////////////////////////////////////////////////////////////////
//GSMessage
/////////////////////////////////////////////////////////////////////////////
GSMessage::GSMessage(IObject *sender, const std::string &idRcv)
    :GSOrUavMessage(sender, idRcv, IObject::GroundStation)
{
}

GSMessage::GSMessage(IObjectManager *sender, const std::string &idRcv)
    : GSOrUavMessage(sender, idRcv, IObject::GroundStation)
{
}

MessageType GSMessage::getMessageType(const Message &msg)
{
    MessageType ret = Unknown;
    const string &name = msg.GetTypeName();
    if (name == d_p_ClassName(AckRequestBindUav))
        ret = BindUavRes;
    else if (name == d_p_ClassName(AckPostControl2Uav))
        ret = ControlUavRes;
    else if (name == d_p_ClassName(SyscOperationRoutes))
        ret = SychMissionRes;
    else if (name == d_p_ClassName(AckPostOperationRoute))
        ret = PostORRes;
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
    :GSOrUavMessage(sender, idRcv, IObject::Plant)
{
}

UAVMessage::UAVMessage(IObjectManager *sender, const std::string &idRcv)
    : GSOrUavMessage(sender, idRcv, IObject::Plant)
{
}

MessageType UAVMessage::getMessageType(const google::protobuf::Message &msg)
{
    MessageType ret = Unknown;
    const string &name = msg.GetTypeName();
    if (name == d_p_ClassName(RequestBindUav))
        ret = BindUav;
    if (name == d_p_ClassName(PostOperationRoute))
        ret = PostOR;
    else if (name == d_p_ClassName(PostControl2Uav))
        ret = ControlUav;
    else if (name == d_p_ClassName(RequestRouteMissions))
        ret = SychMission;
    else if (name == d_p_ClassName(RequestUavStatus))
        ret = QueryUav;
    else if (name == d_p_ClassName(RequestIdentityAllocation))
        ret = UavAllocation;

    return ret;
}