#ifndef  __OBJECT_UAV_H__
#define __OBJECT_UAV_H__

#include "ObjectBase.h"

namespace das {
    namespace proto {
        class RequestGSIdentityAuthentication;
        class RequestUavStatus;
        class ParcelDescription;
        class ParcelContracter;
        class RequestIdentityAllocation;
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
class ObjectGS : public IObject
{
public:
    ObjectGS(const std::string &id);
    ~ObjectGS();
    bool IsConnect()const;

    virtual void OnConnected(bool bConnected);
    void SetPswd(const std::string &pswd);
    const std::string &GetPswd()const;
    void SetAuth(int);
    void RespondLogin(int seqno, int res);
protected:
    virtual int GetObjectType()const;
    virtual void ProcessMassage(const IMessage &msg);
    virtual int ProcessReceive(void *buf, int len);
    virtual int GetSenLength()const;
    virtual int CopySend(char *buf, int sz, unsigned form = 0);
    virtual void SetSended(int sended = -1);//-1,·¢ËÍÍê
private:
    void _prcsReqUavs(const das::proto::RequestUavStatus &msg);
    void _prcsPostLand(const das::proto::ParcelDescription &msg, int ack);
    void _prcsDeleteLand(const std::string &id, bool del, int ack);
    void _prcsUavIDAllication(das::proto::RequestIdentityAllocation *msg);
    void _ackLandOfGs(const std::string &user, int ack);
    void _initialParcelDescription(das::proto::ParcelDescription *msg, const ExecutItem &item);
    uint64_t _saveContact(const das::proto::ParcelDescription &msg, ExecutItem &item, uint64_t id);
    uint64_t _saveLand(const das::proto::ParcelDescription &msg, ExecutItem &item, uint64_t id);

    das::proto::ParcelContracter *_transPC(const ExecutItem &item);
    void _send(const google::protobuf::Message &msg);
protected:
    friend class GSManager;
    bool            m_bConnect;
    int             m_auth;
    std::string     m_pswd;
    ProtoMsg        *m_p;
};

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

