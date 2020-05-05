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
using namespace SOCKETS_NAMESPACE;
#endif

class ProtoMsg;
class ObjectAbsPB : public IObject, public ILink
{
public:
    ObjectAbsPB(const std::string &id);
    ~ObjectAbsPB();

    bool IsConnect()const;
public:
    static void SendProtoBuffTo(ISocket *s, const google::protobuf::Message &ms);
protected:
    int ProcessReceive(void *buf, int len);
    void OnConnected(bool bConnected);
    void send(google::protobuf::Message *msg, bool bWait=false);
    virtual void WaitSend(google::protobuf::Message *msg);
    virtual void PrcsProtoBuff() = 0;
    static int serialize(const google::protobuf::Message &ms, char*buf, int sz);
    IObject *GetParObject();
    ILink *GetHandle();
protected:
    ProtoMsg                              *m_p;
    std::list<google::protobuf::Message*> m_protosSend;
};

class AbsPBManager : public IObjectManager
{
public:
    AbsPBManager();
    virtual ~AbsPBManager();
public:
    virtual void ToCurrntLog(int err, const std::string &obj, int evT, const std::string &dscb);
protected:
    IObject *PrcsNotObjectReceive(ISocket *s, const char *buf, int len);
    virtual IObject *PrcsProtoBuff(ISocket *s) = 0;
protected:
    ProtoMsg    *m_p;
};
#endif // __OBJECT_ABS_PB_H__

