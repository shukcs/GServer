#ifndef  __OBJECT_ABS_PB_H__
#define __OBJECT_ABS_PB_H__

#include "ObjectBase.h"

namespace google {
    namespace protobuf {
        class Message;
    }
}

class ExecutItem;
class VGMySql;
#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

class ProtoMsg;
class ObjectAbsPB : public IObject
{
public:
    ObjectAbsPB(const std::string &id);
    ~ObjectAbsPB();

    bool IsConnect()const;
public:
    static void SendProtoBuffTo(ISocket *s, const google::protobuf::Message &ms);
protected:
    void OnConnected(bool bConnected);
    void send(google::protobuf::Message *msg, bool bRm=false);
    virtual void WaitSend(google::protobuf::Message *msg);
    static int serialize(const google::protobuf::Message &ms, char*buf, int sz);
protected:
    bool            m_bConnect;
    ProtoMsg        *m_p;
};

#ifdef SOCKETS_NAMESPACE
}
#endif
#endif // __OBJECT_ABS_PB_H__

