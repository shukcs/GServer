#ifndef  __GS_MANAGER_H__
#define __GS_MANAGER_H__

#include "ObjectBase.h"

namespace das {
    namespace proto {
        class RequestGSIdentityAuthentication;
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
using namespace SOCKETS_NAMESPACE;
#endif

class ProtoMsg;

class GSManager : public IObjectManager
{
public:
    GSManager();
    ~GSManager();
    
    VGMySql *GetMySql()const;
protected:
    int GetObjectType()const;
    IObject *PrcsReceiveByMrg(ISocket *s, const char *buf, int &len);

    bool PrcsRemainMsg(const IMessage &msg);
private:
    IObject *_checkLogin(ISocket *s, const das::proto::RequestGSIdentityAuthentication &rgi);
    void _parseConfig();
    void _parseMySql(const TiXmlDocument &doc);
private:
    VGMySql         *m_sqlEng;
    ProtoMsg        *m_p;
};

#endif // __OBJECT_UAV_H__

