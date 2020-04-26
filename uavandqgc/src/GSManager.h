#ifndef  __GS_MANAGER_H__
#define __GS_MANAGER_H__

#include "ObjectAbsPB.h"

namespace das {
    namespace proto {
        class RequestGSIdentityAuthentication;
        class RequestNewGS;
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
class GSManager : public AbsPBManager
{
public:
    GSManager();
    ~GSManager();

    int AddDatabaseUser(const std::string &user, const std::string &pswd, ObjectGS *gs=NULL, int seq=0, int auth = 1);
public:
    static std::string CatString(const std::string &s1, const std::string &s2);
protected:
    int GetObjectType()const;
    IObject *PrcsProtoBuff(ISocket *s);

    bool PrcsPublicMsg(const IMessage &msg);

    IObject *prcsPBLogin(ISocket *s, const das::proto::RequestGSIdentityAuthentication *msg);
    IObject *prcsPBNewGs(ISocket *s, const das::proto::RequestNewGS *msg);
    void LoadConfig();
    bool InitManager();
private:
    bool            m_bInit;
};

#endif // __OBJECT_UAV_H__

