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
class ObjectVgFZ;
class GXClient;

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
    static bool IsGSUserValide(const std::string &user);
protected:
    int getMessageType(const google::protobuf::Message &msg);
    void _copyMsg(google::protobuf::Message *c, const google::protobuf::Message &msg);
protected:
};


class Gs2GsMessage : public GSOrUavMessage
{
    CLASS_INFO(Gs2GsMessage)
public:
    Gs2GsMessage(ObjectVgFZ *sender, const std::string &idRcv);
    Gs2GsMessage(IObjectManager *sender, const std::string &idRcv);
private:
};

class Uav2GXMessage : public GSOrUavMessage
{
    CLASS_INFO(Uav2GXMessage)
public:
    Uav2GXMessage(ObjectUav *sender);
private:
};

#ifdef SOCKETS_NAMESPACE
}
#endif

#endif//__MESSGES_H__

