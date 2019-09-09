#ifndef  __OBJECT_GS_H__
#define __OBJECT_GS_H__

#include "ObjectBase.h"

namespace das {
    namespace proto {
        class RequestGSIdentityAuthentication;
        class PostHeartBeat;
        class RequestUavStatus;
        class RequestBindUav;
        class PostParcelDescription;
        class RequestParcelDescriptions;
        class DeleteParcelDescription;
        class OperationDescription;
        class PostOperationDescription;
        class RequestOperationDescriptions;
        class DeleteOperationDescription;
        class PostOperationRoute;
        class PostControl2Uav;
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

class ExecutItem;
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
protected:
    virtual int GetObjectType()const;
    virtual void ProcessMassage(const IMessage &msg);
    virtual int ProcessReceive(void *buf, int len);
    virtual int GetSenLength()const;
    virtual int CopySend(char *buf, int sz, unsigned form = 0);
    virtual void SetSended(int sended = -1);//-1,·¢ËÍÍê
private:
    void _prcsLogin(das::proto::RequestGSIdentityAuthentication *msg);
    void _prcsHeartBeat(das::proto::PostHeartBeat *msg);
    void _prcsReqUavs(das::proto::RequestUavStatus *msg);
    void _prcsReqBind(das::proto::RequestBindUav *msg);
    void _prcsControl2Uav(das::proto::PostControl2Uav *msg);
    void _prcsPostLand(das::proto::PostParcelDescription *msg);
    void _prcsReqLand(das::proto::RequestParcelDescriptions *msg);
    void _prcsDeleteLand(das::proto::DeleteParcelDescription *msg);
    void _prcsUavIDAllication(das::proto::RequestIdentityAllocation *msg);
    void _prcsPostPlan(das::proto::PostOperationDescription *msg);
    void _prcsReqPlan(das::proto::RequestOperationDescriptions *msg);
    void _prcsDelPlan(das::proto::DeleteOperationDescription *msg);
    void _prcsPostMission(das::proto::PostOperationRoute *msg);

    void _initialParcelDescription(das::proto::ParcelDescription *msg, const ExecutItem &item);
    uint64_t _saveContact(const das::proto::ParcelDescription &msg, ExecutItem &item, uint64_t id);
    uint64_t _saveLand(const das::proto::ParcelDescription &msg, ExecutItem &item, uint64_t id);
    das::proto::ParcelContracter *_transPC(const ExecutItem &item);
    int64_t _savePlan(ExecutItem *item, const das::proto::OperationDescription &msg);
    void _initialPlan(das::proto::OperationDescription *msg, const ExecutItem &item);

    void _send(const google::protobuf::Message &msg);
    int64_t _executeInsertSql(ExecutItem *item);
private:
    friend class GSManager;
    bool            m_bConnect;
    int             m_auth;
    std::string     m_pswd;
    ProtoMsg        *m_p;
};

#endif // __OBJECT_UAV_H__

