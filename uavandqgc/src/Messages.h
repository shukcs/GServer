#ifndef  __MESSGES_H__
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

class ObjectUav;
class ObjectGS;
class ObjectGXClinet;

class GSOrUavMessage : public IMessage
{
public:
    GSOrUavMessage(IObject *sender, const std::string &idRcv, int rcv);
    GSOrUavMessage(IObjectManager *sender, const std::string &idRcv, int rcv);
    ~GSOrUavMessage();

    void *GetContent() const;
    int GetContentLength() const;
    void AttachProto(google::protobuf::Message *msg);
    google::protobuf::Message *GetProtobuf()const;
    void SetPBContent(const google::protobuf::Message &msg);
public:
    static std::string GenCheckString(int len = 6);
    static bool IsGSUserValide(const std::string &user);
protected:
    virtual int getMessageType(const google::protobuf::Message &msg) = 0;
    void _copyMsg(google::protobuf::Message *c, const google::protobuf::Message &msg);
protected:
};

class Uav2GSMessage : public GSOrUavMessage
{
public:
    Uav2GSMessage(ObjectUav *sender, const std::string &idRcv);
    Uav2GSMessage(IObjectManager *sender, const std::string &idRcv);
protected:
    int getMessageType(const google::protobuf::Message &msg);
private:
};

class GS2UavMessage : public GSOrUavMessage
{
public:
    GS2UavMessage(ObjectGS *sender, const std::string &idRcv);
    GS2UavMessage(IObjectManager *sender, const std::string &idRcv);

    int GetAuth()const;
protected:
    int getMessageType(const google::protobuf::Message &msg);
private:
    int m_auth;
};

class Gs2GsMessage : public GSOrUavMessage
{
public:
    Gs2GsMessage(ObjectGS *sender, const std::string &idRcv);
    Gs2GsMessage(IObjectManager *sender, const std::string &idRcv);
protected:
    int getMessageType(const google::protobuf::Message &msg);
private:
};

class Uav2GXMessage : public GSOrUavMessage
{
public:
    Uav2GXMessage(ObjectUav *sender);
protected:
    int getMessageType(const google::protobuf::Message &msg);
private:
};

class GX2UavMessage : public IMessage
{
public:
    GX2UavMessage(const std::string &sender, int st);
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

