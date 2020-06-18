#include "UavMission.h"
#include "das.pb.h"
#include "Utility.h"
#include "DBMessages.h"
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

typedef union {
    float tmp[7];
    MAVPACKED(struct {
        float velocity[3];
        uint16_t precision;     //航线精度
        uint16_t gndHeight;     //地面高度
        uint16_t gpsVSpeed;     //垂直速度
        uint16_t curMs;         //当前任务点
        uint8_t fixType;        //定位模式及模块状态
        uint8_t baseMode;       //飞控模块状态
        uint8_t satellites;     //卫星数
        uint8_t sysStat;        //飞控状态
        uint8_t missionRes : 4; //任务状态
        uint8_t voltageErr : 4; //电压报警
        uint8_t sprayState : 4; //喷洒报警
        uint8_t magneErr : 2;   //校磁报警
        uint8_t gpsJam : 1;     //GPS干扰
        uint8_t stDown : 1;     //下载状态 0:没有或者完成，1:正下载
        uint8_t sysType;        //飞控类型
    });
} GpsAdtionValue;

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
//UavMission
///////////////////////////////////////////////////////////////////////////////////////////////////////////
UavMission::UavMission(ObjectUav *obj) : m_parent(obj), m_mission(NULL), m_nCurRidge(-1)
, m_bSys(false), m_bSuspend(false), m_disBeg(0), m_allLength(0)
, m_latSuspend(INVALIDLat), m_lastORNotify(0)
{
}

UavMission::~UavMission()
{
    delete m_mission;
}

void UavMission::PrcsRcvPostOperationInfo(const PostOperationInformation &msg)
{
    int nCount = msg.oi_size();
    if (m_nCurRidge>=0)
    {
        for (int i = 0; i < nCount; i++)
        {
            const OperationInformation &oi = msg.oi(i);
            if (oi.has_gps() && oi.has_status())
                _prcsGps(oi.gps(), oi.status().operationmode());
        }
    }
}

google::protobuf::Message *UavMission::PrcsRcvReqMissions(const RequestRouteMissions &msg)
{
    auto ack = new AckRequestRouteMissions;
    if (!ack)
        return NULL;

    bool hasItem = _hasMission(msg);
    int offset = msg.offset();
    ack->set_seqno(msg.seqno());
    ack->set_result(hasItem ? 1 : 0);
    ack->set_boundary(msg.boundary());
    ack->set_offset(hasItem ? offset : -1);
    if (m_mission)
    {
        int count = msg.boundary() ? m_mission->boundarys_size() : m_mission->missions_size();
        for (int i = 0; i<msg.count()&&count>=offset+i; ++i)
        {
            const string &msItem = msg.boundary() ? m_mission->boundarys(i+offset):m_mission->missions(i+offset);
            ack->add_missions(msItem.c_str(), msItem.size());
        }
    }
    m_bSys = true;
    return ack;
}

void UavMission::ProcessArm(bool bArm)
{
    if (bArm && m_nCurRidge && m_bSys && m_mission && m_mission->beg() <= m_mission->end())
        m_nCurRidge = 0;
}

bool UavMission::_hasMission(const das::proto::RequestRouteMissions &req) const
{
    if (!m_mission || req.offset()<0 || req.count()<=0)
        return false;

    return (req.offset() + req.count()) <= (req.boundary() ? m_mission->boundarys_size() : m_mission->missions_size());
}

google::protobuf::Message *UavMission::GetNotifyUavUOR(uint32_t ms)
{
    if (!m_mission || m_bSys || ms - m_lastORNotify < 500)
        return NULL;

    static uint32_t sSeqno = 1;
    m_lastORNotify = (uint32_t)Utility::msTimeTick();
    if (auto upload = new UploadOperationRoutes)
    {
        upload->set_seqno(sSeqno++);
        upload->set_uavid(m_mission->uavid());
        upload->set_userid(m_mission->gsid());
        upload->set_timestamp(m_mission->createtime());
        upload->set_countmission(m_mission->missions_size());
        upload->set_countboundary(m_mission->boundarys_size());
        return upload;
    }
    return NULL;
}

void UavMission::_prcsGps(const GpsInformation &gps, const string &mod)
{
    if (MagMission == mod)
    {
        m_nCurRidge = -1;
    }
    else if (mod == ReturmMod || mod == MissionMod || m_bSuspend)
    {
        GpsAdtionValue gpsAdt = { 0 };
        int count = (sizeof(GpsAdtionValue) + sizeof(float) - 1) / sizeof(float);
        for (int i = 0; i < gps.velocity_size() && i < count; ++i)
        {
            gpsAdt.tmp[i] = gps.velocity(i);
        }
        int cur = getCurRidgeByItem(gpsAdt.curMs);
        if (cur > 0)
            m_nCurRidge = cur;

        if (m_mission->end() == cur || m_bSuspend)
        {
            _missionFinish(gpsAdt.curMs);
            m_nCurRidge = -1;
            m_bSuspend = false;
        }
    }
}

bool UavMission::ParsePostOr(const OperationRoute &sor)
{
    int rdSz = sor.ridgebeg_size();
    int beg = sor.beg();
    if (rdSz-1 != sor.end()-beg)
        return false;

    ReleasePointer(m_mission);
    m_disBeg = 0;
    m_mission = new OperationRoute();
    if (!m_mission)
        return false;

    m_bSys = false;
    m_mission->CopyFrom(sor);
    if (m_mission->createtime() == 0)
        m_mission->set_createtime(Utility::msTimeTick());
    m_ridges.clear();
    m_allLength = 0;
    for (int i = 0; i < sor.ridgebeg_size(); ++i)
    {
        int item = sor.ridgebeg(i);
        RidgeDat &dat = m_ridges[item];
        dat.length = genRidgeLength(item);
        dat.idx = beg + i;
        m_allLength += dat.length;
    }
    m_latSuspend = INVALIDLat;

    return true;
}

int32_t UavMission::getCurRidgeByItem(int curItem)
{
    if (m_ridges.size() < 0 || curItem >= m_mission->missions_size())
        return -1;

    for (const pair<int32_t, RidgeDat> &itr : m_ridges)
    {
        if (curItem <= itr.first)
            return itr.second.idx-1;
    }
    return m_mission->end();
}

void UavMission::_missionFinish(int curItem)
{
    int ridgeFlying = _getOprRidge(curItem);
    if (!m_mission || ridgeFlying<m_mission->beg())
        return;

    bool bRemain = m_bSuspend && ridgeFlying>m_nCurRidge;
    double flied = bRemain ? _getOprLength(curItem) : 0;
    if (bRemain)
    {
        int latT = INVALIDLat, lonT = INVALIDLat;
        if (getGeo(m_mission->missions(curItem), latT, latT))
            flied -= geoDistance(m_latSuspend, m_lonSuspend, latT, lonT);

        if (ridgeFlying==m_mission->beg() && flied<m_disBeg)
            return;		          //没有飞到断点之前不要统计

        if (flied < 0)
            flied = 0;
    }

    _saveMission(ridgeFlying > m_nCurRidge, calculateOpArea(flied));
    m_disBeg = flied;
    m_mission->set_beg(m_nCurRidge + 1);
}

void UavMission::MavLinkfilter(const PostStatus2GroundStation &msg)
{
    for (int i = 0; i < msg.data_size(); ++i)
    {
        const ::std::string &dt = msg.data(i);
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
            if (  m_nCurRidge >= 0
               && (supports.assist_state & 4) == 4
               && isOtherSuspend(supports.int_latitude, supports.int_longitude) )
            {
                m_latSuspend = supports.int_latitude;
                m_lonSuspend = supports.int_longitude;
                m_bSuspend = true;
            }
        }
    }
}

int UavMission::CountAll() const
{
    if (m_mission)
        return  m_mission->boundarys_size() + m_mission->missions_size();
    return 0;
}

int UavMission::CountItems() const
{
    if (m_mission)
        return  m_mission->missions_size();

    return 0;
}

double UavMission::genRidgeLength(int idx)
{
    if (m_mission && idx > 0 && idx < m_mission->missions_size())
    {
        int lat1;
        int lon1;
        getGeo(m_mission->missions(idx - 1), lat1, lon1);
        int lat2;
        int lon2;
        getGeo(m_mission->missions(idx), lat2, lon2);

        return geoDistance(lat1, lon1, lat2, lon2);
    }
    return 0;
}

float UavMission::calculateOpArea(double flied)const
{
    if (!m_mission)
        return 0;

    double oped = flied; //下垄已经飞完的
    for (const pair<int32_t, RidgeDat> &itr : m_ridges)
    {
        int idx = itr.second.idx;
        if (m_mission->beg()<= idx && idx <=m_nCurRidge)
            oped += itr.second.length;
    }

    oped -= m_disBeg;
    if (m_allLength < 0.0001)
        return m_mission->acreage();

    return float(m_mission->acreage()*oped / m_allLength);
}

int UavMission::_getOprRidge(int curItem) const
{
    auto itr = m_ridges.find(curItem);
    if (itr == m_ridges.end())
        return m_nCurRidge;

    return itr->second.idx;
}

double UavMission::_getOprLength(int curItem) const
{
    auto itr = m_ridges.find(curItem);
    if (itr == m_ridges.end())
        return 0;

    return itr->second.length;
}

void UavMission::_saveMission(bool bSuspend, float acrage)
{
    if (!m_parent)
        return;

    if (DBMessage *msg = new DBMessage(m_parent, IMessage::Unknown, DBMessage::DB_GS))
    {
        msg->SetSql("insertMissions");
        msg->SetWrite("userID", m_mission->gsid());
        if (m_mission->has_landid())
            msg->SetWrite("landId", m_mission->landid());
        if (m_mission->has_rpid())
            msg->SetWrite("planID", m_mission->rpid());

        msg->SetWrite("uavID", m_mission->uavid());
        msg->SetWrite("createTime", m_mission->createtime());
        msg->SetWrite("finishTime", Utility::msTimeTick());

        if (bSuspend)
        {
            msg->SetWrite("continiuLat", m_latSuspend);
            msg->SetWrite("continiuLon", m_lonSuspend);
        }

        if (m_mission->has_beg())
            msg->SetWrite("begin", m_mission->beg());
        msg->SetWrite("end", m_nCurRidge);
        msg->SetWrite("acreage", acrage);
        m_parent->SendMsg(msg);
    }
}

bool UavMission::isOtherSuspend(int lat, int lon) const
{
    return m_latSuspend != lat || m_lonSuspend != lon;
}
