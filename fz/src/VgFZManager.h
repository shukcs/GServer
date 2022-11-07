#ifndef  __GS_MANAGER_H__
#define __GS_MANAGER_H__

#include "ObjectAbsPB.h"

namespace das {
    namespace proto {
        class RequestFZUserIdentity;
        class RequestNewFZUser;
        class PostGetFZPswd;
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
class ObjectVgFZ;
class FZ2FZMessage;
class VgFZManager : public IObjectManager
{
public:
    VgFZManager();
    ~VgFZManager();

    int AddDatabaseUser(const std::string &user, const std::string &pswd, ObjectVgFZ *gs=NULL, int seq=0, int auth = 1);
    const StringList &Uavs()const;
public:
    static std::string CatString(const std::string &s1, const std::string &s2);
protected:
    int GetObjectType()const override;
    bool PrcsPublicMsg(const IMessage &msg)override;
    void ToCurrntLog(int err, const std::string &obj, int evT, const std::string &dscb)override;
    void LoadConfig(const TiXmlElement *root)override;
    bool IsHasReuest(const char *buf, int len)const override;
private:
    IObject *prcsPBLogin(ISocket *s, const das::proto::RequestFZUserIdentity *msg);
    IObject *PrcsProtoBuff(ISocket *s, const google::protobuf::Message *proto);
    IObject *prcsPBNewGs(ISocket *s, const das::proto::RequestNewFZUser *msg);
    IObject *prcsPostGetFZPswd(ISocket *s, const das::proto::PostGetFZPswd *msg);
    void processGSMessage(const FZ2FZMessage &gsM);
private:
    bool                    m_bInit;
    StringList              m_uavs;
    std::list<ObjectVgFZ *>   m_mgrs;
};

#endif // __OBJECT_UAV_H__

