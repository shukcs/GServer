﻿#ifndef  __MESSGES_H__
#define __MESSGES_H__

#include "IMessage.h"

namespace google {
    namespace protobuf {
        class Message;
    }
}

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

class ObjectTracker;
class ObjectGV;
class GXClient;

class TrackerMessage : public IMessage
{
    CLASS_INFO(TrackerMessage)
public:
    TrackerMessage(const IObject &sender, const std::string &idRcv, int rcv);
    TrackerMessage(IObjectManager *sender, const std::string &idRcv, int rcv);
    ~TrackerMessage();

    void *GetContent() const;
    int GetContentLength() const;
    void AttachProto(google::protobuf::Message *msg);
    google::protobuf::Message *GetProtobuf()const;
    void SetPBContent(const google::protobuf::Message &msg);
public:
    static bool IsGSUserValide(const std::string &user);
protected:
    virtual MessageType getMessageType(const google::protobuf::Message &msg) = 0;
    void _copyMsg(google::protobuf::Message *c, const google::protobuf::Message &msg);
protected:
};

class Tracker2GVMessage : public TrackerMessage
{
    CLASS_INFO(Tracker2GVMessage)
public:
    Tracker2GVMessage(const ObjectTracker &sender, const std::string &idRcv);
    Tracker2GVMessage(IObjectManager *sender, const std::string &idRcv);
protected:
    MessageType getMessageType(const google::protobuf::Message &msg);
private:
};

class GV2TrackerMessage : public TrackerMessage
{
    CLASS_INFO(GV2TrackerMessage)
public:
    GV2TrackerMessage(const ObjectGV &sender, const std::string &idRcv);
    GV2TrackerMessage(IObjectManager *sender, const std::string &idRcv);
protected:
    MessageType getMessageType(const google::protobuf::Message &msg);
private:
};

class Tracker2GXMessage : public TrackerMessage
{
    CLASS_INFO(Tracker2GXMessage)
public:
    Tracker2GXMessage(const ObjectTracker &sender);
    Tracker2GXMessage(IObjectManager *sender);
protected:
    MessageType getMessageType(const google::protobuf::Message &msg);
private:
};

class GX2TrackerMessage : public IMessage
{
    CLASS_INFO(GX2TrackerMessage)
public:
    GX2TrackerMessage(const std::string &sender, int st);
    int GetStat()const;
protected:
    void *GetContent() const;
    int GetContentLength() const;
private:
    int m_stat;
};

#ifdef SOCKETS_NAMESPACE
}
#endif

#endif//__MESSGES_H__

