#ifndef  __OBJECT_UAV_H__
#define __OBJECT_UAV_H__

#include "ObjectAbsPB.h"

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
        class RequestPositionAuthentication;
        class AckPositionAuthentication;
        class GpsInformation;
    }
}

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

class DBMessage;
class ObjectGS;
class ObjectUav : public ObjectAbsPB
{
public:
    ObjectUav(const std::string &id, const std::string &sim="");
    ~ObjectUav();

    void TransUavStatus(das::proto::UavStatus &us, bool bAuth = false)const;
    void RespondLogin(int seq, int res);
    bool IsValid()const;
    void SetValideTime(int64_t tmV);
    void SetSimId(const std::string &sim);
public:
    static int UAVType();
    static void InitialUAV(const DBMessage &msg, ObjectUav &uav);
protected:
    virtual int GetObjectType()const;
    virtual void ProcessMessage(IMessage *msg);
    virtual int ProcessReceive(void *buf, int len);

    void CheckTimer(uint64_t ms);
    void OnConnected(bool bConnected);
    void InitObject();
protected:
    static IMessage *AckControl2Uav(const das::proto::PostControl2Uav &msg, int res, ObjectUav *obj = NULL);
private:
    void prcsRcvPostOperationInfo(das::proto::PostOperationInformation *msg);
    void prcsRcvPost2Gs(das::proto::PostStatus2GroundStation *msg);
    void prcsRcvReqMissions(das::proto::RequestRouteMissions *msg);
    void prcsPosAuth(das::proto::RequestPositionAuthentication *msg);

    void processBind(das::proto::RequestBindUav *msg, IObject *sender);
    void processControl2Uav(das::proto::PostControl2Uav *msg);
    void processPostOr(das::proto::PostOperationRoute *msg, const std::string &gs);
    void processBaseInfo(const DBMessage &msg);

    bool _isBind(const std::string &gs)const;
    bool _hasMission(const das::proto::RequestRouteMissions &req)const;
    void _notifyUavUOR(const das::proto::OperationRoute &ort);
    int _checkPos(double lat, double lon, double alt);
    void _prcsGps(const das::proto::GpsInformation &gps, const std::string &mod);
private:
    void _missionFinish();
    void savePos();
    void saveBind(bool bBind, const std::string &gs, bool bForce=false);
    void sendBindAck(int ack, int res, bool bind, const std::string &gs);
private:
    friend class UavManager;
    std::string                     m_strSim;
    bool                            m_bBind;
    uint32_t                        m_lastORNotify;
    double                          m_lat;
    double                          m_lon;
    int64_t                         m_tmLastBind;
    int64_t                         m_tmLastPos;
    int64_t                         m_tmValidLast;
    das::proto::OperationRoute      *m_mission;
    int                             m_nCurMsItem;
    bool                            m_bSys;
    std::string                     m_lastBinder;
    std::string                     m_authCheck;
};

#ifdef SOCKETS_NAMESPACE
}
#endif
#endif//__OBJECT_UAV_H__

