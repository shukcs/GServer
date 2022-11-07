#include "UavMission.h"
#include "das.pb.h"
#include "Utility.h"
#include "DBMessages.h"
#include "ProtoMsg.h"
#include "common/mavlink.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define EARTH_RADIOS 6367558.49686

enum 
{
    WRITE_BUFFLEN = 1024,
    MissionFinish = 9,
    INVALIDLat = (int)2e9,
};
#define MissionMod      "Mission"
#define ReturmMod       "Return"
#define MagMission      "MagMsm"

using namespace std;
using namespace das::proto;

static double geoDistance(int lat1, int lon1, int lat2, int lon2)
{
    double dif = (lat1/1e7)*(M_PI/180);
    double x = double(lat1 - lat2)*1e-7*M_PI*EARTH_RADIOS / 180;
    double y = double(lon1 - lon2)*1e-7*M_PI*EARTH_RADIOS / 180;
    y = y*cos(dif);

    return sqrt(x*x + y*y);
}

static bool getGeo(const string &buff, int &lat, int &lon)
{
    if (buff.size() < MAVLINK_MSG_ID_MISSION_ITEM_INT_LEN)
        return false;
    mavlink_message_t msg = { 0 };
    msg.msgid = MAVLINK_MSG_ID_MISSION_ITEM_INT;
    msg.len = uint8_t(buff.size());
    memcpy(msg.payload64, buff.c_str(), msg.len);
    mavlink_mission_item_int_t item = { 0 };
    mavlink_msg_mission_item_int_decode(&msg, &item);
    if (item.command != MAV_CMD_NAV_WAYPOINT)
        return false;
    lat = item.x;
    lon = item.y;
    return true;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
//MissionData
///////////////////////////////////////////////////////////////////////////////////////////////////////////
class MissionData
{
private:
    typedef struct _RidgeDat {
        int     idx;
        double  length;
    } RidgeDat;
public:
    MissionData() : m_allLength(0.0), m_disBeg(0.0), m_tmCreate(0), m_acreage(0)
        ,m_ridgeBeg(-1), m_lastORNotify(0), m_bSys(false)
    {}
    ~MissionData(){}
    float CalculateOpArea(const Coordinate *suspend, int curItem)
    {
        bool bFinish = false;
        int ridgeCur = GetCurRidge(curItem, bFinish);
        if (ridgeCur < 0 || ridgeCur < m_ridgeBeg)
            return -1;

        bool bRemain = suspend && bFinish;
        double flied = bRemain ? _getOprLength(curItem) : 0;
        if (bRemain)
        {
            int latT = INVALIDLat, lonT = INVALIDLat;
            if (getGeo(getItem(false, curItem), latT, latT))
                flied -= geoDistance(suspend->latitude(), suspend->longitude(), latT, lonT);

            if (ridgeCur == m_disBeg && flied < m_disBeg)
                return -1;		          //没有飞到断点之前不要统计

            if (flied < 0)
                flied = 0;
        }

        double oped = flied - m_disBeg; //下垄已经飞完的减去前次飞过的
        for (const pair<int32_t, RidgeDat> &itr : m_ridges)
        {
            int idx = itr.second.idx;
            if (m_ridgeBeg <= idx && itr.first < curItem)
                oped += itr.second.length;
        }

        m_ridgeBeg = bFinish ? ridgeCur + 1 : ridgeCur;
        m_disBeg = flied;
        if (m_allLength < 0.0001)
            return m_acreage;

        return float(m_acreage*oped / m_allLength);
    }
    const string &GetGs()const
    {
        return m_user;
    }
    const string &GetLandId()const
    {
        return m_landId;
    }
    const string &GetPlanId()const
    {
        return m_rpid;
    }
    int64_t GetCreateTm()const
    {
        return m_tmCreate;
    }
    int GetBegRidge()const
    {
        return m_ridgeBeg;
    }
    bool ProcessRcvOpRoute(const OperationRoute &sor)
    {
        int rdSz = sor.ridgebeg_size();
        m_ridgeBeg = sor.beg();
        if (rdSz - 1 != sor.end() - m_ridgeBeg)
            return false;

        m_boundarys.clear();
        for (int i = 0; i<sor.boundarys().size();++i)
        {
            m_boundarys.push_back(sor.boundarys(i));
        }
        m_missionItems.clear();
        for (int i = 0; i < sor.missions().size(); ++i)
        {
            m_missionItems.push_back(sor.missions(i));
        }
        m_disBeg = 0;
        m_bSys = false;
        m_rpid = sor.rpid();
        m_landId = sor.landid();
        m_user = sor.gsid();
        m_tmCreate = sor.createtime() == 0 ? Utility::msTimeTick() : sor.createtime();
        m_ridges.clear();
        m_allLength = 0;
        m_acreage = sor.acreage();
        for (int i = 0; i < sor.ridgebeg_size(); ++i)
        {
            int item = sor.ridgebeg(i);
            RidgeDat &dat = m_ridges[item];
            dat.length = _genRidgeLength(item);
            dat.idx = m_ridgeBeg + i;
            m_allLength += dat.length;
        }
        return true;
    }
    AckRequestRouteMissions *RequstMissions(int offset, int count, bool bBdr)
    {
        auto ack = new AckRequestRouteMissions;
        if (!ack)
            return NULL;

        bool hasItem = _hasMission(bBdr, offset, count);
        ack->set_result(hasItem ? 1 : 0);
        ack->set_boundary(bBdr);
        ack->set_offset(hasItem ? offset : -1);
 
        StringList &ls = bBdr ? m_boundarys : m_missionItems;
        auto itr = ls.begin();
        for (int i = 0; i < offset && itr != ls.end(); ++i, ++itr);
        for (int i = 0; i < count && itr!=ls.end(); ++i)
        {
            ack->add_missions(itr->c_str(), itr->size());
            ++itr;
        }

        m_bSys = true;
        return ack;
    }
    int CountMissions()const
    {
        return m_missionItems.size();
    }
    int CountBoundarys()const
    {
        return m_boundarys.size();
    }
    int GetCurRidge(int curItem, bool &bFinish) const
    {
        int idx = 0;
        for (auto itr = m_ridges.begin(); itr != m_ridges.end(); ++itr)
        {
            idx = itr->second.idx;
            if (curItem <= itr->first)
            {
                bFinish = curItem < itr->first;//判断是否开始本垄
                if (bFinish)
                    idx--;

                return idx;
            }
        }

        bFinish = true;
        return idx;
    }
    double GetOprLength(int curItem) const
    {
        auto itr = m_ridges.find(curItem);
        if (itr == m_ridges.end())
            return 0;

        return itr->second.length;
    }
    UploadOperationRoutes *GetNotifyRoute(int64_t ms, const string &uav)
    {
        if (m_bSys || ms - m_lastORNotify < 500)
            return NULL;

        m_lastORNotify = (uint32_t)ms;
        static uint32_t sSeqno = 1;
        if (auto upload = new UploadOperationRoutes)
        {
            upload->set_seqno(sSeqno++);
            upload->set_uavid(uav);
            upload->set_msid(m_rpid);
            upload->set_timestamp(m_tmCreate);
            upload->set_countmission(CountMissions());
            upload->set_countboundary(CountBoundarys());
            return upload;
        }
        return NULL;
    }
private:
    double _getOprLength(int curItem) const
    {
        auto itr = m_ridges.find(curItem);
        if (itr == m_ridges.end())
            return 0;

        return itr->second.length;
    }
    bool _hasMission(bool bBdr, int offset, int count) const
    {
        if (offset < 0 || count <= 0)
            return false;

        return offset+count <= int(bBdr ? m_boundarys.size() : m_missionItems.size());
    }
    double _genRidgeLength(int idx)
    {
        if (idx > 0 && idx < (int)m_missionItems.size())
        {
            int lat1;
            int lon1;
            getGeo(getItem(false, idx), lat1, lon1);
            int lat2;
            int lon2;
            getGeo(getItem(false, idx), lat2, lon2);

            return geoDistance(lat1, lon1, lat2, lon2);
        }
        return 0;
    }
    const string &getItem(bool bdr, int idx)const
    {
        const StringList &ls = bdr ? m_boundarys : m_missionItems;
        if (idx >= (int)ls.size() || idx < 0)
            return Utility::EmptyStr();

        for (auto itr = ls.begin(); itr!=ls.end(); ++itr)
        {
            if (idx-- == 0)
                return *itr;
        }
        return Utility::EmptyStr();
    }
private:
    double                          m_allLength;
    double                          m_disBeg;
    int64_t                         m_tmCreate;
    float                           m_acreage;
    int32_t                         m_ridgeBeg;
    uint32_t                        m_lastORNotify;
    bool                            m_bSys;
    StringList                      m_missionItems;
    StringList                      m_boundarys;
    string                          m_rpid;
    string                          m_landId;
    string                          m_user;
    std::map<int32_t, RidgeDat>     m_ridges;       //地垄key:itemseq
};
///////////////////////////////////////////////////////////////////////////////////////////////////////////
//MissionData
///////////////////////////////////////////////////////////////////////////////////////////////////////////
class MissionABData
{
private:
    typedef struct _ABMissionItem {
        int32_t     lat;
        int32_t     lon;
        int32_t     alt;
    } ABMisnItem;
public:
    MissionABData() : m_sprayWidth(0.0), m_lastAbLat(0), m_lastAbLon(0), m_lastItem(0)
    {}
    ~MissionABData() {}
    float CalculateOpArea(const Coordinate *suspend, int item)
    {
        if (m_sprayWidth < 0.1 || item<0)
            return -1;

        double oped = 0;
        if (m_lastItem % 2 && m_abMisnItems.find(m_lastItem) != m_abMisnItems.end())
        {
            const ABMisnItem &item = m_abMisnItems.find(m_lastItem)->second;
            oped += geoDistance(item.lat, item.lon, m_lastAbLat, m_lastAbLon);
            m_lastItem++;
        }
        for (auto itr = m_abMisnItems.begin(); itr != m_abMisnItems.end(); itr++)
        {
            if (m_lastItem <= itr->first && itr->first <= item)
            {
                auto tmp = itr++;
                if (itr == m_abMisnItems.end())
                    break;
                oped += geoDistance(tmp->second.lat, tmp->second.lon, itr->second.lat, itr->second.lon);
            }
        }
        m_lastItem = item;
        if (item % 2 == 0 && suspend)
        {
            m_lastItem++;
            auto itr = m_abMisnItems.find(item);
            if (suspend)
            {
                m_lastAbLat = suspend->latitude();
                m_lastAbLon = suspend->longitude();
            }
            else if (itr != m_abMisnItems.end())
            {
                m_lastAbLat = suspend->latitude();
                m_lastAbLon = suspend->longitude();
            }
            if (itr != m_abMisnItems.end())
                oped += geoDistance(itr->second.lat, itr->second.lon, m_lastAbLat, m_lastAbLon);
        }
        return float(m_sprayWidth * oped * 15 / 10000); ///亩
    }
    void PrcsABOperation(const PostABOperation &msg)
    {
        if (msg.has_sw())
            m_sprayWidth = msg.sw();

        int next = m_abMisnItems.size();
        for (int i = 0; i + 3 < msg.points_size(); i += 4)
        {
            int idx = msg.points(i);
            if (0 == idx)
            {
                m_abMisnItems.clear();
                m_lastItem = 0;
            }
            if (next++ == m_lastItem)
            {
                ABMisnItem &it = m_abMisnItems[idx];
                it.lat = msg.points(i + 1);
                it.lon = msg.points(i + 2);
                it.alt = msg.points(i + 3);
            }
        }
    }
    int CountMissions()const
    {
        return m_abMisnItems.size();
    }
private:
    double                          m_sprayWidth;
    int32_t                         m_lastAbLat;
    int32_t                         m_lastAbLon;
    int                             m_lastItem;
    std::map<int32_t, ABMisnItem>   m_abMisnItems;
};
//////////////////////////////////////////////////////////////////////////////////////////
//UavMission
//////////////////////////////////////////////////////////////////////////////////////////
UavMission::UavMission(ObjectUav *obj) : m_parent(obj), m_mission(NULL), m_missionAB(NULL)
, m_assists(NULL), m_abPoints(NULL), m_return(NULL)
{
}

UavMission::~UavMission()
{
    delete m_mission;
    Clear();
}

void UavMission::PrcsPostAssists(PostOperationAssist *msg)
{
    if (!msg)
        return;
    if (!m_assists)
        m_assists = new PostOperationAssist;

    m_assists->CopyFrom(*msg);
}

void UavMission::PrcsPostABPoint(PostABPoint *msg)
{
    if (!msg)
        return;

    if (!m_abPoints)
        m_abPoints = new PostABPoint;
    m_abPoints->CopyFrom(*msg);
}

void UavMission::PrcsPostReturn(PostOperationReturn *msg)
{
    if (!msg)
        return;

    if (!m_return)
        m_return = new PostOperationReturn;
    m_return->CopyFrom(*msg);
    if (msg->mission() && m_mission && m_mission->GetPlanId()== msg->msid())
        _missionFinish(msg->msitem());
    else if (!msg->mission() && m_missionAB)
        _abMissionFinish(msg->msitem());
}

void UavMission::PrcsABOperation(das::proto::PostABOperation *msg)
{
    if (!msg)
        return;

    if (!m_missionAB)
        m_missionAB = new MissionABData;

    if (m_missionAB)
        m_missionAB->PrcsABOperation(*msg);
}

google::protobuf::Message *UavMission::PrcsRcvReqMissions(const RequestRouteMissions &msg)
{
    AckRequestRouteMissions *ret = NULL;
    if (m_mission)
    {
        ret = m_mission->RequstMissions(msg.offset(), msg.count(), msg.boundary());
        if (ret)
            ret->set_seqno(msg.seqno());
    }

    return ret;
}

google::protobuf::Message *UavMission::GetNotifyUavUOR(uint32_t ms)
{
    if (m_mission)
        return m_mission->GetNotifyRoute(ms, GetParentID());

    return NULL;
}

bool UavMission::ParsePostOr(const OperationRoute &sor)
{
    if (!m_mission)
        m_mission = new MissionData;
    if (m_mission)
        return m_mission->ProcessRcvOpRoute(sor);

    return false;
}

void UavMission::_missionFinish(int curItem)
{
    bool bFinish = false;
    int ridgeCur = m_mission ? m_mission->GetCurRidge(curItem, bFinish):-1;
    if (!m_mission || ridgeCur < m_mission->GetBegRidge())
        return;
    float acrage = m_mission->CalculateOpArea(m_return->has_suspend() ? &m_return->suspend() : NULL, curItem);
    if (acrage > 0)
        _saveMission(!bFinish, acrage, bFinish ? ridgeCur : ridgeCur - 1);
}

void UavMission::_abMissionFinish(int curItem)
{
    float acrage = m_missionAB->CalculateOpArea(m_return->has_suspend() ? &m_return->suspend() : NULL, curItem);
    if (acrage > 0)
        _saveMission(curItem%2!=1, acrage, curItem/2, false);
}

void UavMission::MavLinkfilter(PostStatus2GroundStation &pb)
{
    for (int i = 0; i < pb.data_size(); ++i)
    {
        const ::std::string &dt = pb.data(i);
        if (dt.size() < 3)
            continue;

        uint16_t msgId = *(uint16_t*)dt.c_str();
        if (MAVLINK_MSG_ID_ASSIST_POSITION == msgId)
        {
            mavlink_message_t msg = { 0 };
            msg.msgid = msgId;
            msg.msgid = *(uint16_t*)dt.c_str();
            msg.len = uint8_t(dt.size()) - sizeof(uint16_t);
            memcpy(msg.payload64, dt.c_str() + sizeof(uint16_t), msg.len);

            mavlink_assist_position_t supports;
            mavlink_msg_assist_position_decode(&msg, &supports);
            if ((supports.assist_state & 3))
                saveOtherAssists(supports.start_latitude, supports.start_longitude, supports.end_latitude, supports.end_longitude, supports.assist_state);
            else
                saveOtherABPoints(supports.start_latitude, supports.start_longitude, supports.end_latitude, supports.end_longitude, (supports.assist_state&0x10)!=0);
            pb.mutable_data()->DeleteSubrange(i, 1);
        }
    }
}

int UavMission::CountAll() const
{
    if (m_mission)
        return  m_mission->CountBoundarys() + m_mission->CountMissions();
    return 0;
}

int UavMission::CountItems() const
{
    if (m_mission)
        return  m_mission->CountMissions();

    return 0;
}

void UavMission::Clear()
{
    ReleasePointer(m_assists);
    ReleasePointer(m_abPoints);
    ReleasePointer(m_return);
}

const std::string &UavMission::GetParentID()const
{
    if (m_parent)
        return m_parent->GetObjectID();

    return Utility::EmptyStr();
}

google::protobuf::Message *UavMission::AckRequestPost(UavMission &ms, google::protobuf::Message *msg)
{
    if (!msg)
        return NULL;

    const string &name = msg->GetDescriptor()->full_name();
    if (name == d_p_ClassName(RequestOperationAssist))
    {
        auto seq = ((RequestOperationAssist*)msg)->seqno();
        if (ms.m_assists)
        {
            ms.m_assists->set_seqno(seq);
            return ms.m_assists;
        }

        static PostOperationAssist sAst;
        sAst.set_seqno(seq);
        sAst.set_uavid(ms.GetParentID());
        return &sAst;
    }
    if (name == d_p_ClassName(RequestABPoint))
    {
        auto seq = ((RequestABPoint*)msg)->seqno();
        if (ms.m_abPoints)
        {
            ms.m_abPoints->set_seqno(seq);
            return ms.m_abPoints;
        }

        static PostABPoint sAB;
        sAB.set_seqno(seq);
        sAB.set_uavid(ms.GetParentID());
        return &sAB;
    }
    if (name == d_p_ClassName(RequestOperationReturn))
    {
        auto seq = ((RequestOperationReturn*)msg)->seqno();
        if (ms.m_return)
        {
            ms.m_return->set_seqno(seq);
            return ms.m_return;
        }

        static PostOperationReturn sRet;
        sRet.set_seqno(seq);
        sRet.set_uavid(ms.GetParentID());
        return &sRet;
    }
    return NULL;
}

void UavMission::_saveMission(bool bSuspend, float acrage, int finshed, bool bMission)
{
    if (!m_parent || !m_mission)
        return;

    if (DBMessage *msg = new DBMessage(m_parent, IMessage::Unknown, DBMessage::DB_GS))
    {
        msg->SetSql("insertMissions");
        msg->SetWrite("userID", bMission ? m_mission->GetGs() : m_parent->GetBinder());
        if (bMission)
        {
            msg->SetWrite("landId", m_mission->GetLandId());
            msg->SetWrite("planID", m_mission->GetPlanId());
            msg->SetWrite("createTime", m_mission->GetCreateTm());
        }

        msg->SetWrite("uavID", m_parent->GetObjectID());
        msg->SetWrite("finishTime", Utility::msTimeTick());

        if (bSuspend && m_return && m_return->has_suspend())
        {
            msg->SetWrite("continiuLat", m_return->suspend().latitude());
            msg->SetWrite("continiuLon", m_return->suspend().longitude());
        }

        if (bMission)
            msg->SetWrite("begin", m_mission->GetBegRidge());
        else
            msg->SetWrite("begin", 0);///
        msg->SetWrite("end", finshed);
        msg->SetWrite("acreage", acrage);
        m_parent->SendMsg(msg);
    }
}

void UavMission::saveOtherAssists(int latE, int lonE, int latR, int lonR, int stat)
{
    if (!m_parent)
        return;

    if (!m_assists)
    {
        m_assists = new PostOperationAssist;
        if (!m_assists)
            return;

        m_assists->set_seqno(m_assists->seqno() + 1);
        m_assists->set_uavid(m_parent->GetObjectID());
    }
    auto coor = (stat & 1) ? new Coordinate() : NULL;
    if (coor)
    {
        coor->set_latitude(latE);
        coor->set_longitude(lonE);
    }
    m_assists->set_allocated_enter(coor);

    coor = (stat & 2) ? new Coordinate() : NULL;
    if (coor)
    {
        coor->set_latitude(latR);
        coor->set_longitude(lonR);
    }
    m_assists->set_allocated_return_(coor);
    m_parent->SndBinder(*m_assists);
}

void UavMission::saveOtherABPoints(int latA, int lonA, int latB, int lonB, bool bHas)
{
    if (!m_parent)
        return;

    if (!bHas && m_abPoints)
    {
        m_abPoints->set_allocated_pointa(NULL);
        m_abPoints->set_allocated_pointb(NULL);
    }
    else if (bHas)
    {
        if (!m_abPoints)
        {
            m_abPoints = new PostABPoint;
            if (!m_abPoints)
                return;

            m_abPoints->set_seqno(1);
            m_abPoints->set_uavid(m_parent->GetObjectID());
        }
        if (auto coor = new Coordinate())
        {
            coor->set_latitude(latA);
            coor->set_longitude(lonA);
            m_abPoints->set_allocated_pointa(coor);
        }

        if (auto coor = new Coordinate())
        {
            coor->set_latitude(latB);
            coor->set_longitude(lonB);
            m_abPoints->set_allocated_pointb(coor);
        }
    }
    if (m_abPoints)
        m_parent->SndBinder(*m_abPoints);
}
