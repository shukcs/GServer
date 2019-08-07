#ifndef  __MESSGES_H__
#define __MESSGES_H__

#include "ObjectBase.h"

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

    BindUavRes,
    ControlUavRes,
    SychMissionRes,
    PushUavSndInfo,
    ControlGs,
    QueryUavRes,
};

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

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
private:
    google::protobuf::Message  *m_msg;
};

class GSMessage : public IMessage
{
public:
    GSMessage(IObject *sender, const std::string &idRcv);
    GSMessage(IObjectManager *sender, const std::string &idRcv);
    ~GSMessage();

    void *GetContent() const;
    void SetContent(const google::protobuf::Message &msg);
    void AttachProto(google::protobuf::Message *msg);
    int GetContentLength()const;
private:
    google::protobuf::Message  *m_msg;
};

#ifdef SOCKETS_NAMESPACE
}
#endif
#endif//__MESSGES_H__

