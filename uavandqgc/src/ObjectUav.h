#ifndef  __OBJECT_UAV_H__
#define __OBJECT_UAV_H__

#include "ObjectAbsPB.h"

namespace google {
    namespace protobuf {
        class Message;
    }
}
namespace das {
    namespace proto {
        class RequestBindUav;
        class PostOperationInformation;
        class OperationRoute;
        class PostOperationRoute;
        class PostStatus2GroundStation;
        class PostControl2Uav;
        class PostOperationAssist;
        class PostABPoint;
        class PostOperationReturn;
        class RequestUavIdentityAuthentication;
        class RequestUavStatus;
        class RequestRouteMissions;
        class UavStatus;
        class MissionSuspend;
        class RequestMissionSuspend;
        class RequestPositionAuthentication;
        class AckPositionAuthentication;
        class GpsInformation;
        class UavRoute;
        class PostBlocks;
        class PostABOperation;
    }
}

class UavMission;
#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

class DBMessage;
class ObjectGS;
class Variant;
class GS2UavMessage;
class GX2UavMessage;
class ObjectUav : public ObjectAbsPB
{
public:
    ObjectUav(const std::string &id, const std::string &sim="");
    ~ObjectUav();

    void TransUavStatus(das::proto::UavStatus &us, bool bAuth = false)const;
    bool IsValid()const;
    void SetValideTime(int64_t tmV);
    void SetSimId(const std::string &sim);
    void SndBinder(const google::protobuf::Message &pb);
    const std::string &GetBinder()const;
public:
    static int UAVType();
    static void InitialUAV(const DBMessage &msg, ObjectUav &uav, uint32_t idx=0);
protected:
    virtual int GetObjectType()const override;
    virtual void ProcessMessage(const IMessage *msg) override;
    void PrcsProtoBuff(uint64_t);

    void CheckTimer(uint64_t ms)override;
    void OnConnected(bool bConnected)override;
    void InitObject()override;
    void _respondLogin(int seq, int res);
    void OnLogined(bool suc, ISocket *s=NULL);
    void FreshLogin(uint64_t ms);
protected:
    static IMessage *AckControl2Uav(const das::proto::PostControl2Uav &msg, int res, ObjectUav *obj = NULL);
private:
    void _prcsRcvPostOperationInfo(das::proto::PostOperationInformation *msg, uint64_t tm);
    void _prcsRcvPost2Gs(das::proto::PostStatus2GroundStation *msg);
    void _prcsAssist(das::proto::PostOperationAssist *msg);
    void _prcsABPoint(das::proto::PostABPoint *msg);
    void _prcsReturn(das::proto::PostOperationReturn *msg);
    void _prcsRcvReqMissions(das::proto::RequestRouteMissions *msg);
    void _prcsPosAuth(das::proto::RequestPositionAuthentication *msg);
    void _prcsPostBlocks(das::proto::PostBlocks *msg);
    void _prcsABOperation(das::proto::PostABOperation *msg);

    void processBind(const das::proto::UavStatus &st);
    void processControl2Uav(das::proto::PostControl2Uav *msg);
    void processRequestPost(google::protobuf::Message *msg, const std::string &gs);
    void processPostOr(das::proto::PostOperationRoute *msg, const std::string &gs);
    void processBaseInfo(const DBMessage &msg);
    void processGxStat(const GX2UavMessage &msg);

    bool _isBind(const std::string &gs)const;
    int _checkPos(double lat, double lon, double alt);
private:
    void savePos();
    void sendGx(const das::proto::PostOperationInformation &msg, uint64_t tm);
private:
    friend class UavManager;
    UavMission                      *m_mission;
    std::string                     m_strSim;
    bool                            m_bBind;
    double                          m_lat;
    double                          m_lon;
    int64_t                         m_tmLastBind;
    uint64_t                        m_tmLastPos;
    int64_t                         m_tmValidLast;
    bool                            m_bSendGx;
    std::string                     m_lastBinder;
    std::string                     m_authCheck;
};

#ifdef SOCKETS_NAMESPACE
}
#endif
#endif//__OBJECT_UAV_H__

