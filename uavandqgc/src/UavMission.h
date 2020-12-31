#ifndef  __UAV_MISSION_H__
#define __UAV_MISSION_H__

#include "ObjectUav.h"

#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif

class MissionData;
class MissionABData;
class UavMission
{
public:
    UavMission(ObjectUav *obj);
    ~UavMission();
public:
    google::protobuf::Message *PrcsRcvReqMissions(const das::proto::RequestRouteMissions &);
    google::protobuf::Message *GetNotifyUavUOR(uint32_t ms);
    bool ParsePostOr(const das::proto::OperationRoute &sor);
    void MavLinkfilter(das::proto::PostStatus2GroundStation &msg);
    void PrcsPostAssists(das::proto::PostOperationAssist *msg);
    void PrcsPostABPoint(das::proto::PostABPoint *msg);
    void PrcsPostReturn(das::proto::PostOperationReturn *msg);
    void PrcsABOperation(das::proto::PostABOperation *msg);
    int CountAll()const;
    int CountItems()const;
    void Clear();
    const std::string &GetParentID()const;
public:
    static google::protobuf::Message *AckRequestPost(UavMission &ms, google::protobuf::Message *msg);
private:
    void _missionFinish(int curItem);
    void _abMissionFinish(int curItem);
    void _saveMission(bool bSuspend, float acrage, int finshed, bool bMission=true);
    void saveOtherAssists(int latE, int lonE, int latR, int lonR, int stat);
    void saveOtherABPoints(int latA, int lonA, int latB, int lonB, bool bHas);
private:
    ObjectUav                       *m_parent;
    MissionData                     *m_mission;
    MissionABData                   *m_missionAB;
    das::proto::PostOperationAssist *m_assists;
    das::proto::PostABPoint         *m_abPoints;
    das::proto::PostOperationReturn *m_return;
};

#ifdef SOCKETS_NAMESPACE
}
#endif
#endif//__UAV_MISSION_H__

