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
    google::protobuf::Message *PrcsRcvReqMissions(const das::proto::RequestRouteMissions &);
    google::protobuf::Message *GetNotifyUavUOR(uint32_t ms);
    bool ParsePostOr(const das::proto::OperationRoute &sor);
    bool MavLinkfilter(das::proto::PostStatus2GroundStation &msg);
    void PrcsPostAssists(das::proto::PostOperationAssist *msg);
    void PrcsPostABPoint(das::proto::PostABPoint *msg);
    void PrcsPostReturn(das::proto::PostOperationReturn *msg);
    int CountAll()const;
    int CountItems()const;
    void Clear();
    const std::string &GetParentID()const;
public:
    static google::protobuf::Message *AckRequestPost(UavMission &ms, google::protobuf::Message *msg);
private:
    int32_t getCurRidge(int curItem, bool &bFinish)const;    //最新飞完垄
    bool _hasMission(const das::proto::RequestRouteMissions &req)const;
    void _missionFinish(int curItem);
    double genRidgeLength(int idx);
    float calculateOpArea(double opedNext, int curItem)const;
    double _getOprLength(int curItem)const;
    void _saveMission(bool bSuspend, float acrage, int finshed);
    void saveOtherAssists(int latE, int lonE, int latR, int lonR, int stat);
    void saveOtherABPoints(int latA, int lonA, int latB, int lonB, bool bHas);
private:
    ObjectUav                       *m_parent;
    das::proto::OperationRoute      *m_mission;
    das::proto::PostOperationAssist *m_assists;
    das::proto::PostABPoint         *m_abPoints;
    das::proto::PostOperationReturn *m_return;
    bool                            m_bSys;
    double                          m_disBeg;
    double                          m_allLength;
    uint32_t                        m_lastORNotify;
    std::map<int32_t, RidgeDat>     m_ridges;   //地垄key:itemseq
};

#ifdef SOCKETS_NAMESPACE
}
#endif
#endif//__UAV_MISSION_H__

