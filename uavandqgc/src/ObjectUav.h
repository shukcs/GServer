#ifndef  __OBJECT_UAV_H__
#define __OBJECT_UAV_H__

#include "ObjectAbsPB.h"

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

#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif

class ObjectUav : public ObjectAbsPB
{
protected:
    ObjectUav(const std::string &id);
    ~ObjectUav();

    void InitBySqlResult(const ExecutItem &sql);
    void transUavStatus(das::proto::UavStatus &us);
    void RespondLogin(int seq, int res);
public:
    static int UAVType();
protected:
    void OnConnected(bool bConnected);
    virtual int GetObjectType()const;
    virtual void ProcessMassage(const IMessage &msg);
    virtual int ProcessReceive(void *buf, int len);
    VGMySql *GetMySql()const;

    void CheckTimer(uint64_t ms);
protected:
    static void AckControl2Uav(const das::proto::PostControl2Uav &msg, int res, ObjectUav *obj=NULL);
private:
    void prcsRcvPostOperationInfo(das::proto::PostOperationInformation *msg);
    void prcsRcvPost2Gs(das::proto::PostStatus2GroundStation *msg);
    void prcsRcvReqMissions(das::proto::RequestRouteMissions *msg);

    void processBind(das::proto::RequestBindUav *msg);
    void processControl2Uav(das::proto::PostControl2Uav *msg);
    void processPostOr(das::proto::PostOperationRoute *msg);

    bool _isBind(const std::string &gs)const;
    bool _hasMission(const das::proto::RequestRouteMissions &req)const;
    void _notifyUavUOR(const das::proto::OperationRoute &ort);
private:
    friend class UavManager;
    bool                            m_bBind;
    uint32_t                        m_lastORNotify;
    double                          m_lat;
    double                          m_lon;
    int64_t                         m_tmLastInfo;
    int64_t                         m_tmLastBind;
    das::proto::OperationRoute      *m_mission;
    bool                            m_bSys;
    std::string                     m_lastBinder;
};

#endif//__OBJECT_UAV_H__

