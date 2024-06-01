#ifndef  __MESSGES_H__
#define __MESSGES_H__

#include "IMessage.h"
#include <map>

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
class GXClient;

class GSOrUavMessage : public IMessage
{
public:
    GSOrUavMessage(const IObject &sender, const std::string &idRcv, int rcv);
    GSOrUavMessage(IObjectManager *sender, const std::string &idRcv, int rcv);
    ~GSOrUavMessage();

    void *GetContent() const;
    int GetContentLength() const;
    void AttachProto(google::protobuf::Message *msg);
    google::protobuf::Message *GetProtobuf()const;
    void SetPBContent(const google::protobuf::Message &msg);
public:
    static bool IsGSUserValide(const std::string &user);
protected:
    int getMessageType(const google::protobuf::Message &msg);
    void _copyMsg(google::protobuf::Message *c, const google::protobuf::Message &msg);
protected:
};

class Uav2GSMessage : public GSOrUavMessage
{
    CLASS_INFO(Uav2GSMessage)
public:
    Uav2GSMessage(const ObjectUav &sender, const std::string &idRcv);
    Uav2GSMessage(IObjectManager *sender, const std::string &idRcv);
private:
};

class GS2UavMessage : public GSOrUavMessage
{
    CLASS_INFO(GS2UavMessage)
public:
    GS2UavMessage(const ObjectGS &sender, const std::string &idRcv);
    GS2UavMessage(IObjectManager *sender, const std::string &idRcv);

    int GetAuth()const;
private:
    int m_auth;
};

class Gs2GsMessage : public GSOrUavMessage
{
    CLASS_INFO(Gs2GsMessage)
public:
    Gs2GsMessage(const ObjectGS &sender, const std::string &idRcv);
    Gs2GsMessage(IObjectManager *sender, const std::string &idRcv);
private:
};

class Uav2GXMessage : public GSOrUavMessage
{
    CLASS_INFO(Uav2GXMessage)
public:
    Uav2GXMessage(const ObjectUav &sender);
private:
};

class GX2UavMessage : public IMessage
{
    CLASS_INFO(GX2UavMessage)
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

