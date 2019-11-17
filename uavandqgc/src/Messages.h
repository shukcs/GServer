#ifndef  __MESSGES_H__
#define __MESSGES_H__

#include "IMessage.h"

namespace google {
    namespace protobuf {
        class Message;
    }
}

enum MessageType
{
    Unknown,
    BindUav,
    PostOR,
    ControlUav,
    SychMission,
    QueryUav,
    UavAllocation,
    Gs2UavEnd,

    BindUavRes,
    ControlUavRes,
    SychMissionRes,
    PostORRes,
    PushUavSndInfo,
    ControlGs,
    QueryUavRes,
    UavAllocationRes,

    Uav2GsEnd,
    Gs2GsBeging = Uav2GsEnd,
    Gs2GsMsg,
    Gs2GsAck,
};

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

class ObjectUav;
class ObjectGS;

class GSOrUavMessage : public IMessage
{
public:
    GSOrUavMessage(IObject *sender, const std::string &idRcv, int rcv);
    GSOrUavMessage(IObjectManager *sender, const std::string &idRcv, int rcv);
    ~GSOrUavMessage();

    void *GetContent() const;
    int GetContentLength() const;
    void AttachProto(google::protobuf::Message *msg);
    google::protobuf::Message *GetProtobufMsg()const;
    template<class E>
    void SetPBContent(const E &msg)
    {
        delete m_msg;
        m_msg = new E;
        _copyMsg(msg);
    }
public:
    static std::string GenCheckString(int len = 6);
    static bool IsGSUserValide(const std::string &user);
protected:
    virtual MessageType getMessageType(const google::protobuf::Message &msg) = 0;
    void _copyMsg(const google::protobuf::Message &msg);
protected:
    google::protobuf::Message  *m_msg;
};

class Uav2GSMessage : public GSOrUavMessage
{
public:
    Uav2GSMessage(ObjectUav *sender, const std::string &idRcv);
    Uav2GSMessage(IObjectManager *sender, const std::string &idRcv);
protected:
    MessageType getMessageType(const google::protobuf::Message &msg);
private:
};

class GS2UavMessage : public GSOrUavMessage
{
public:
    GS2UavMessage(ObjectGS *sender, const std::string &idRcv);
    GS2UavMessage(IObjectManager *sender, const std::string &idRcv);
protected:
    MessageType getMessageType(const google::protobuf::Message &msg);
private:
};

class Gs2GsMessage : public GSOrUavMessage
{
public:
    Gs2GsMessage(ObjectGS *sender, const std::string &idRcv);
    Gs2GsMessage(IObjectManager *sender, const std::string &idRcv);
protected:
    MessageType getMessageType(const google::protobuf::Message &msg);
private:
};

#ifdef SOCKETS_NAMESPACE
}
#endif

#endif//__MESSGES_H__

