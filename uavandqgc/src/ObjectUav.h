#ifndef  __OBJECT_UAV_H__
#define __OBJECT_UAV_H__

#include "ObjectBase.h"

namespace google {
    namespace protobuf {
        class Message;
    }
}
class TiXmlDocument;
namespace das {
    namespace proto {
        class RequestBindUav;
        class PostOperationInformation;
        class OperationRoute;
        class PostOperationRoute;
        class PostStatus2GroundStation;
        class PostControl2Uav;
        class RequestUavIdentityAuthentication;
        class RequestUavStatus;
        class RequestRouteMissions;
        class UavStatus;
    }
}

class ExecutItem;

#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif

class ProtoMsg;
class ObjectUav : public IObject
{
public:
    bool IsConnect()const;
protected:
    ObjectUav(const std::string &id);
    ~ObjectUav();

    void InitBySqlResult(const ExecutItem &sql);
    void transUavStatus(das::proto::UavStatus &us);

    void OnConnected(bool bConnected);
    virtual int GetObjectType()const;
    virtual void ProcessMassage(const IMessage &msg);
    virtual int ProcessReceive(void *buf, int len);
    virtual int GetSenLength()const;
    virtual int CopySend(char *buf, int sz, unsigned form = 0);
    virtual void SetSended(int sended = -1);//-1,·¢ËÍÍê
    void RespondLogin(int seq, int res);
protected:
    static void AckControl2Uav(const das::proto::PostControl2Uav &msg, int res, ObjectUav *obj=NULL);
private:
    void prcsRcvPostOperationInfo(das::proto::PostOperationInformation *msg);
    void prcsRcvPost2Gs(das::proto::PostStatus2GroundStation *msg);
    void prcsRcvReqMissions(das::proto::RequestRouteMissions *msg);

    void processBind(das::proto::RequestBindUav *msg);
    void processControl2Uav(das::proto::PostControl2Uav *msg);
    void processPostOr(das::proto::PostOperationRoute *msg);

    void _send(const google::protobuf::Message &msg);
    bool _isBind(const std::string &gs)const;
    bool _hasMission(const das::proto::RequestRouteMissions &req)const;
private:
    friend class UavManager;
    bool                            m_bBind;
    bool                            m_bConnected;
    double                          m_lat;
    double                          m_lon;
    int64_t                         m_tmLastInfo;
    int64_t                         m_tmLastBind;
    ProtoMsg                        *m_p;
    das::proto::OperationRoute      *m_mission;
    std::string                     m_lastBinder;
};

#endif//__OBJECT_UAV_H__

