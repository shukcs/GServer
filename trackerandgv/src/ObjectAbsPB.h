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

public:
    static bool SendProtoBuffTo(ISocket *s, const google::protobuf::Message &ms);
protected:
    int ProcessReceive(void *buf, int len, uint64_t ms);
    void WaitSend(google::protobuf::Message *msg);
    virtual void PrcsProtoBuff(uint64_t) = 0;
    IObject *GetParObject();
    ILink *GetLink();
    void CopyAndSend(const google::protobuf::Message &msg);
protected:
    static int serialize(const google::protobuf::Message *ms, char*buf, int sz);
    static void releaseProtobuf(google::protobuf::Message *msg);
protected:
    ProtoMsg                              *m_p;
    std::queue<google::protobuf::Message*> m_protosSend;
};

class AbsPBManager : public IObjectManager
{
public:
    AbsPBManager();
    virtual ~AbsPBManager();
protected:
    IObject *PrcsNotObjectReceive(ISocket *s, const char *buf, int len);
    virtual IObject *PrcsProtoBuff(ISocket *s) = 0;
protected:
    ProtoMsg    *m_p;
};
#endif // __OBJECT_ABS_PB_H__

