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
    ControlUav,
    SychMission,
    QueryUav,
    UavAllocation,

    BindUavRes,
    ControlUavRes,
    SychMissionRes,
    PushUavSndInfo,
    ControlGs,
    QueryUavRes,
    UavAllocationRes,
};
class GSMessage : public IMessage
{
public:
    GSMessage(IObject *sender, const std::string &idRcv);
    GSMessage(IObjectManager *sender, const std::string &idRcv);
    ~GSMessage();
    void *GetContent() const;
    void SetGSContent(const google::protobuf::Message &msg);
    void AttachProto(google::protobuf::Message *msg);
    int GetContentLength() const;
protected:
    MessageType getMessageType(const google::protobuf::Message &msg);
private:
    google::protobuf::Message  *m_msg;
};

class UAVMessage : public IMessage
{
public:
    UAVMessage(IObject *sender, const std::string &idRcv);
    UAVMessage(IObjectManager *sender, const std::string &idRcv);
    ~UAVMessage();
    void *GetContent() const;
    void SetContent(const google::protobuf::Message &msg);
    void AttachProto(google::protobuf::Message *msg);
    int GetContentLength() const;
protected:
    MessageType getMessageType(const google::protobuf::Message &msg);
private:
    google::protobuf::Message  *m_msg;
};

#endif//__MESSGES_H__

