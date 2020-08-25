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
static string sEmptyStr;

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
UavMission::UavMission(ObjectUav *obj) : m_parent(obj), m_mission(NULL), m_assists(NULL), m_abPoints(NULL)
, m_return(NULL), m_bSys(false), m_disBeg(0), m_allLength(0), m_lastORNotify(0)
{
}

UavMission::~UavMission()
{
    delete m_mission;
    Clear();
}

void UavMission::PrcsPostAssists(PostOperationAssist *msg)
{
    ReleasePointer(m_assists);
    m_assists = msg;
}

void UavMission::PrcsPostABPoint(PostABPoint *msg)
{
    ReleasePointer(m_abPoints);
    m_abPoints = msg;
}

void UavMission::PrcsPostReturn(PostOperationReturn *msg)
{
    if (!msg)
        return;

    ReleasePointer(m_return);
    m_return = msg;
    if (m_mission && m_return && m_mission->rpid() == msg->msid() && msg->mission())
        _missionFinish(msg->msitem());
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
        upload->set_msid(m_mission->rpid());
        upload->set_timestamp(m_mission->createtime());
        upload->set_countmission(m_mission->missions_size());
        upload->set_countboundary(m_mission->boundarys_size());
        return upload;
    }
    return NULL;
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

    return true;
}

void UavMission::_missionFinish(int curItem)
{
    bool bFinish;
    int ridgeCur = getCurRidge(curItem, bFinish);
    if (!m_mission || ridgeCur<m_mission->beg())
        return;

    bool bRemain = m_return && m_return->has_suspend() && bFinish;
    double flied = bRemain ? _getOprLength(curItem) : 0;
    if (bRemain)
    {
        int latT = INVALIDLat, lonT = INVALIDLat;
        if (getGeo(m_mission->missions(curItem), latT, latT))
            flied -= geoDistance(m_return->suspend().latitude(), m_return->suspend().longitude(), latT, lonT);

        if (ridgeCur==m_mission->beg() && flied<m_disBeg)
            return;		          //没有飞到断点之前不要统计

        if (flied < 0)
            flied = 0;
    }

    _saveMission(!bFinish, calculateOpArea(flied, curItem), bFinish?ridgeCur:ridgeCur-1);
    m_disBeg = flied;
    if (bFinish)
        ridgeCur++;
    m_mission->set_beg(ridgeCur);
}

bool UavMission::MavLinkfilter(PostStatus2GroundStation &pb)
{
    bool ret = true;
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
            continue;
        }
        ret = false;
    }
    return ret;
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

    return sEmptyStr;
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

float UavMission::calculateOpArea(double flied, int curItem)const
{
    if (!m_mission)
        return 0;

    double oped = flied; //下垄已经飞完的
    for (const pair<int32_t, RidgeDat> &itr : m_ridges)
    {
        int idx = itr.second.idx;
        if (m_mission->beg()<= idx && itr.first<curItem)
            oped += itr.second.length;
    }

    oped -= m_disBeg;
    if (m_allLength < 0.0001)
        return m_mission->acreage();

    return float(m_mission->acreage()*oped / m_allLength);
}

int UavMission::getCurRidge(int curItem, bool &bFinish) const
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

double UavMission::_getOprLength(int curItem) const
{
    auto itr = m_ridges.find(curItem);
    if (itr == m_ridges.end())
        return 0;

    return itr->second.length;
}

void UavMission::_saveMission(bool bSuspend, float acrage, int finshed)
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

        if (bSuspend && m_return && m_return->has_suspend())
        {
            msg->SetWrite("continiuLat", m_return->suspend().latitude());
            msg->SetWrite("continiuLon", m_return->suspend().longitude());
        }

        if (m_mission->has_beg())
            msg->SetWrite("begin", m_mission->beg());
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

    coor = (stat & 1) ? new Coordinate() : NULL;
    if (coor)
    {
        coor->set_latitude(latR);
        coor->set_longitude(lonR);
    }
    m_assists->set_allocated_return_(coor);
    m_parent->BinderProcess(*m_assists);
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
        m_parent->BinderProcess(*m_abPoints);
}
