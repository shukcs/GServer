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

    BindUavRes,
    ControlUavRes,
    SychMissionRes,
    PostORRes,
    PushUavSndInfo,
    ControlGs,
    QueryUavRes,
    UavAllocationRes,
};

class GSOrUavMessage : public IMessage
{
public:
    GSOrUavMessage(IObject *sender, const std::string &idRcv, int rcv);
    GSOrUavMessage(IObjectManager *sender, const std::string &idRcv, int rcv);
    ~GSOrUavMessage();

    void *GetContent() const;
    int GetContentLength() const;
    void AttachProto(google::protobuf::Message *msg);
    template<class E>
    void SetPBContentPB(const E &msg)
    {
        delete m_msg;
        m_msg = new E;
        _copyMsg(msg);
    }
public:
    static std::string GenCheckString();
    static bool IsGSUserValide(const std::string &user);
protected:
    virtual MessageType getMessageType(const google::protobuf::Message &msg) = 0;
    void _copyMsg(const google::protobuf::Message &msg);
protected:
    google::protobuf::Message  *m_msg;
};

class GSMessage : public GSOrUavMessage
{
public:
    GSMessage(IObject *sender, const std::string &idRcv);
    GSMessage(IObjectManager *sender, const std::string &idRcv);
protected:
    MessageType getMessageType(const google::protobuf::Message &msg);
private:
    google::protobuf::Message  *m_msg;
};

class UAVMessage : public GSOrUavMessage
{
public:
    UAVMessage(IObject *sender, const std::string &idRcv);
    UAVMessage(IObjectManager *sender, const std::string &idRcv);
protected:
    MessageType getMessageType(const google::protobuf::Message &msg);
private:
    google::protobuf::Message  *m_msg;
};

#endif//__MESSGES_H__

