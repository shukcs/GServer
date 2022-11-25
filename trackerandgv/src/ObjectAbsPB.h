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

class ObjectAbsPB : public IObject, public ILink
{
public:
    ObjectAbsPB(const std::string &id);
    ~ObjectAbsPB();
protected:
    void WaitSend(google::protobuf::Message *msg);
    void CopyAndSend(const google::protobuf::Message &msg);

    const IObject *GetParObject()const override;
    ILink *GetLink()override;
protected:
};

class AbsPBManager : public IObjectManager
{
public:
    AbsPBManager();
    virtual ~AbsPBManager();
protected:
    IObject *PrcsNotObjectReceive(ISocket *s, const char *buf, int len);
protected:
};
#endif // __OBJECT_ABS_PB_H__

