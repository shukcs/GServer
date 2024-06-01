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


class FZ2FZMessage : public GSOrUavMessage
{
    CLASS_INFO(FZ2FZMessage)
public:
    FZ2FZMessage(const ObjectVgFZ &sender, const std::string &idRcv);
    FZ2FZMessage(IObjectManager *sender, const std::string &idRcv);
private:
};

#ifdef SOCKETS_NAMESPACE
}
#endif

#endif//__MESSGES_H__

