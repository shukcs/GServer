#ifndef  __GS_MANAGER_H__
#define __GS_MANAGER_H__

#include "ObjectBase.h"

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

class VGMySql;
class ExecutItem;
class TiXmlDocument;

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

class ProtoMsg;

class GSManager : public IObjectManager
{
public:
    GSManager();
    ~GSManager();

    VGMySql *GetMySql()const;
    int AddDatabaseUser(const std::string &user, const std::string &pswd, int auth = 1);
    void AddDBFriend(const std::string &user1, const std::string &user2);
    void RemoveDBFriend(const std::string &user1, const std::string &user2);
    std::list<std::string> GetDBFriend(const std::string &user);
public:
    static int ExecutNewGsSql(GSManager *mgr, const std::string &gs);
protected:
    int GetObjectType()const;
    IObject *PrcsReceiveByMgr(ISocket *s, const char *buf, int &len);

    bool PrcsPublicMsg(const IMessage &msg);

    IObject *prcsPBLogin(ISocket *s, const das::proto::RequestGSIdentityAuthentication *msg);
    IObject *prcsPBNewGs(ISocket *s, const das::proto::RequestNewGS *msg);
    void LoadConfig();
private:
    void _parseMySql(const TiXmlDocument &doc);
private:
    VGMySql         *m_sqlEng;
    ProtoMsg        *m_p;
    bool            m_bInit;
};

#ifdef SOCKETS_NAMESPACE
}
#endif

#endif // __OBJECT_UAV_H__

