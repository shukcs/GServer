#ifndef  __OBJECT_ABS_PB_H__
#define __OBJECT_ABS_PB_H__

#include "ObjectBase.h"

namespace google {
    namespace protobuf {
        class Message;
    }
}

#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif

class ObjectAbsPB : public IObject, public ILink
{
    typedef std::map<google::protobuf::Message*, bool> MapMessage;
    typedef std::list<google::protobuf::Message*> MessageList;
public:
    ObjectAbsPB(const std::string &id);
    ~ObjectAbsPB();
protected:
    void WaitSend(google::protobuf::Message *msg);
    const IObject *GetParObject()const override;
    ILink *GetLink()override;
    void CopyAndSend(const google::protobuf::Message &msg);
protected:
};

class AbsPBManager : public IObjectManager
{
public:
    AbsPBManager();
    virtual ~AbsPBManager();
public:
    virtual void ToCurrntLog(int err, const std::string &obj, int evT, const std::string &dscb);
protected:
};
#endif // __OBJECT_ABS_PB_H__

