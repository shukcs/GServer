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
    typedef std::map<google::protobuf::Message*, bool> MapMessage;
    typedef std::list<google::protobuf::Message*> MessageList;
public:
    ObjectAbsPB(const std::string &id);
    ~ObjectAbsPB();
public:
    static bool SendProtoBuffTo(ISocket *s, const google::protobuf::Message &ms);
protected:
    int ProcessReceive(void *buf, int len, uint64_t ms);
    void WaitSend(google::protobuf::Message *msg);
    virtual void PrcsProtoBuff(uint64_t) = 0;
    static int serialize(const google::protobuf::Message &ms, char*buf, int sz);
    IObject *GetParObject();
    ILink *GetLink();
    void CheckTimer(uint64_t ms, char *buf, int len);
    void CopyAndSend(const google::protobuf::Message &msg);
private:
    void send(char *buf, int len);
    void clearProto();
protected:
    ProtoMsg     *m_p;
    MapMessage   m_protosMap;
    MessageList  m_protosList;
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

