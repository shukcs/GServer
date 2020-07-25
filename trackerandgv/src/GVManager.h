#ifndef  __GS_MANAGER_H__
#define __GS_MANAGER_H__

#include "ObjectAbsPB.h"

namespace das {
    namespace proto {
        class RequestIVIdentityAuthentication;
    }
}

namespace google {
    namespace protobuf {
        class Message;
    }
}

class TiXmlDocument;

#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif

class ProtoMsg;
class ObjectGV;
class GVManager : public AbsPBManager
{
public:
    GVManager();
    ~GVManager();

    const StringList &OnLineTrackers()const;
public:
    static std::string CatString(const std::string &s1, const std::string &s2);
protected:
    int GetObjectType()const;
    IObject *PrcsProtoBuff(ISocket *s);

    bool PrcsPublicMsg(const IMessage &msg);

    IObject *prcsPBLogin(ISocket *s, const das::proto::RequestIVIdentityAuthentication *msg);
    void LoadConfig();
    bool IsHasReuest(const char *buf, int len)const;
private:
    StringList      m_trackers;
};

#endif // __OBJECT_UAV_H__

