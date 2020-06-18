#ifndef  __UAV_MISSION_H__
#define __UAV_MISSION_H__

#include "ObjectUav.h"

#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif

class UavMission
{
private:
    typedef struct _RidgeDat{
        int     idx;
        double  length;
    } RidgeDat;
public:
    UavMission(ObjectUav *obj);
    ~UavMission();

public:
    void ProcessArm(bool bArm);
    google::protobuf::Message *PrcsRcvReqMissions(const das::proto::RequestRouteMissions &);
    google::protobuf::Message *GetNotifyUavUOR(uint32_t ms);
    bool ParsePostOr(const das::proto::OperationRoute &sor);
    void PrcsRcvPostOperationInfo(const das::proto::PostOperationInformation &msg);
    void MavLinkfilter(const das::proto::PostStatus2GroundStation &msg);
    int CountAll()const;
    int CountItems()const;
private:
    int32_t getCurRidgeByItem(int curItem);    //最新飞完垄
    bool _hasMission(const das::proto::RequestRouteMissions &req)const;
    void _prcsGps(const das::proto::GpsInformation &gps, const std::string &mod);
    void _missionFinish(int curItem);
    double genRidgeLength(int idx);
    float calculateOpArea(double opedNext)const;
    int _getOprRidge(int curItem)const;
    double _getOprLength(int curItem)const;
    void _saveMission(bool bSuspend, float acrage);
    bool isOtherSuspend(int lat, int lon)const;
private:
    ObjectUav                       *m_parent;
    das::proto::OperationRoute      *m_mission;
    int                             m_nCurRidge;
    bool                            m_bSys;
    bool                            m_bSuspend;
    double                          m_disBeg;
    double                          m_allLength;
    int                             m_latSuspend;
    int                             m_lonSuspend;
    uint32_t                        m_lastORNotify;
    std::map<int32_t, RidgeDat>     m_ridges;   //地垄key:itemseq
};

#ifdef SOCKETS_NAMESPACE
}
#endif
#endif//__UAV_MISSION_H__

