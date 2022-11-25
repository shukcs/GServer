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

protected:
    bool WaitSend(google::protobuf::Message *msg);
    void CopyAndSend(const google::protobuf::Message &msg);

    const IObject *GetParObject()const override;
    ILink *GetLink()override;
protected:
};

#endif // __OBJECT_ABS_PB_H__

