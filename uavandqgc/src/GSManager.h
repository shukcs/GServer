#ifndef  __GS_MANAGER_H__
#define __GS_MANAGER_H__

#include "ObjectAbsPB.h"

namespace das {
    namespace proto {
        class RequestGSIdentityAuthentication;
        class RequestNewGS;
        class GroundStationsMessage;
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
class ObjectGS;
class Gs2GsMessage;
class GSManager : public AbsPBManager
{
public:
    GSManager();
    ~GSManager();

    int AddDatabaseUser(const std::string &user, const std::string &pswd, ObjectGS *gs=NULL, int seq=0, int auth = 1);
    const StringList &Uavs()const;
public:
    static std::string CatString(const std::string &s1, const std::string &s2);
protected:
    int GetObjectType()const;
    IObject *PrcsProtoBuff(ISocket *s);

    bool PrcsPublicMsg(const IMessage &msg);
    void processDeviceLogin(int tp, const std::string &dev, bool bLogin);
    void processGSMessage(const Gs2GsMessage &gsM);

    IObject *prcsPBLogin(ISocket *s, const das::proto::RequestGSIdentityAuthentication *msg);
    IObject *prcsPBNewGs(ISocket *s, const das::proto::RequestNewGS *msg);
    void LoadConfig();
    bool IsHasReuest(const char *buf, int len)const;
private:
    bool                    m_bInit;
    StringList              m_uavs;
    std::list<ObjectGS *>   m_mgrs;
};

#endif // __OBJECT_UAV_H__

