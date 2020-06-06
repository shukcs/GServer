#include "ObjectUav.h"
#include "ObjectManagers.h"
#include "das.pb.h"
#include "ProtoMsg.h"
#include "Messages.h"
#include "DBMessages.h"
#include "VGMysql.h"
#include "Utility.h"
#include "socketBase.h"
#include "UavManager.h"
#include "ObjectGS.h"
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
#define Landing		    "Landing"
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
using namespace ::google::protobuf;
#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif
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
//ObjectUav
///////////////////////////////////////////////////////////////////////////////////////////////////////////
ObjectUav::ObjectUav(const string &id, const string &sim) : ObjectAbsPB(id), m_strSim(sim), m_bBind(false)
, m_lastORNotify(0), m_lat(200), m_lon(0), m_tmLastBind(0), m_tmLastPos(0), m_tmValidLast(-1)
, m_mission(NULL), m_nCurRidge(-1), m_bSys(false), m_fliedBeg(0)
{
    SetBuffSize(1024 * 2);
}

ObjectUav::~ObjectUav()
{
    delete m_mission;
}

void ObjectUav::TransUavStatus(UavStatus &us, bool bAuth)const
{
    us.set_result(1);
    us.set_uavid(GetObjectID());
    if(m_lastBinder.length()>0)
        us.set_binder(m_lastBinder);

    us.set_binded(m_bBind);
    us.set_time(m_bBind?m_tmLastBind:m_tmLastBind);
    us.set_online(IsConnect());
    us.set_deadline(m_tmValidLast);
    if (bAuth)
        us.set_authstring(m_authCheck);

    us.set_simid(m_strSim);

    if (GpsInformation *gps = new GpsInformation)
    {
        gps->set_latitude(int(m_lat*1e7));
        gps->set_longitude(int(m_lon*1e7));
        gps->set_altitude(0);
        us.set_allocated_pos(gps);
    }
}

int ObjectUav::GetObjectType() const
{
    return UAVType();
}

void ObjectUav::_respondLogin(int seq, int res)
{
    if(m_p && m_sock)
    {
        if (auto ack = new AckUavIdentityAuthentication)
        {
            ack->set_seqno(seq);
            ack->set_result(res);
            WaitSend(ack);
        }
        OnLogined(true);
    }
}

void ObjectUav::OnLogined(bool suc, ISocket *s)
{
    if (m_bLogined != suc && suc)
    {
        if (auto ms = new ObjectSignal(this, ObjectGS::GSType(), ObjectSignal::S_Login))
            SendMsg(ms);
    }
    ObjectAbsPB::OnLogined(suc, s);
}

bool ObjectUav::IsValid() const
{
    return m_tmValidLast<0 || m_tmValidLast>Utility::msTimeTick();
}

void ObjectUav::SetValideTime(int64_t tmV)
{
    m_tmValidLast = tmV;
}

void ObjectUav::SetSimId(const std::string &sim)
{
    m_strSim = sim;
}

int ObjectUav::UAVType()
{
    return IObject::Plant;
}

void ObjectUav::InitialUAV(const DBMessage &rslt, ObjectUav &uav)
{
    const Variant &binded = rslt.GetRead("binded");
    if (!binded.IsNull())
        uav.m_bBind = binded.ToInt8() != 0;
    const Variant &binder = rslt.GetRead("binder");
    if (binder.GetType() == Variant::Type_string)
        uav.m_lastBinder = binder.ToString();
    const Variant &lat = rslt.GetRead("lat");
    if (lat.GetType() == Variant::Type_double)
        uav.m_lat = lat.ToDouble();
    const Variant &lon = rslt.GetRead("lon");
    if (lon.GetType() == Variant::Type_double)
        uav.m_lon = lon.ToDouble();
    const Variant &timeBind = rslt.GetRead("timeBind");
    if (timeBind.IsNull())
        uav.m_tmLastBind = timeBind.ToInt64();
    const Variant &timePos = rslt.GetRead("timePos");
    if (timePos.IsNull())
        uav.m_tmLastBind = timePos.ToInt64();
    const Variant &valid = rslt.GetRead("valid");
    if (valid.IsNull())
        uav.m_tmValidLast = valid.ToInt64();
    const Variant &authCheck = rslt.GetRead("authCheck");
    if (authCheck.GetType() == Variant::Type_string)
        uav.m_authCheck = authCheck.ToString();
}

IMessage *ObjectUav::AckControl2Uav(const PostControl2Uav &msg, int res, ObjectUav *obj)
{
    Uav2GSMessage *ms = NULL;
    if (obj)
        ms = new Uav2GSMessage(obj, msg.userid());
    else
        ms = new Uav2GSMessage(GetManagerByType(IObject::Plant), msg.userid());

    if (ms)
    {
        AckPostControl2Uav *ack = new AckPostControl2Uav;
        if (!ack)
        {
            delete ms;
            return NULL;
        }
        ack->set_seqno(msg.seqno());
        ack->set_result(res);
        ack->set_uavid(msg.uavid());
        ack->set_userid(msg.userid());
        ms->AttachProto(ack);
    }
    return ms;
}

void ObjectUav::ProcessMessage(IMessage *msg)
{
    if (msg->GetMessgeType() == IMessage::BindUav)
        processBind((RequestBindUav*)msg->GetContent(), *(GS2UavMessage*)msg);
    else if (msg->GetMessgeType() == IMessage::ControlDevice)
        processControl2Uav((PostControl2Uav*)msg->GetContent());
    else if (msg->GetMessgeType() == IMessage::PostOR)
        processPostOr((PostOperationRoute*)msg->GetContent(), msg->GetSenderID());
    else if (msg->GetMessgeType() == IMessage::DeviceQueryRslt)
        processBaseInfo(*(DBMessage *)msg);
}

void ObjectUav::PrcsProtoBuff(uint64_t)
{
    if (!m_p)
        return;

    const string &name = m_p->GetMsgName();
    if (name == d_p_ClassName(RequestUavIdentityAuthentication))
        _respondLogin(((RequestUavIdentityAuthentication*)m_p->GetProtoMessage())->seqno(), 1);
    else if (name == d_p_ClassName(PostOperationInformation))
        _prcsRcvPostOperationInfo((PostOperationInformation *)m_p->DeatachProto());
    else if (name == d_p_ClassName(PostStatus2GroundStation))
        _prcsRcvPost2Gs((PostStatus2GroundStation *)m_p->GetProtoMessage());
    else if (name == d_p_ClassName(RequestRouteMissions))
        _prcsRcvReqMissions((RequestRouteMissions *)m_p->GetProtoMessage());
    else if (name == d_p_ClassName(RequestPositionAuthentication))
        _prcsPosAuth((RequestPositionAuthentication *)m_p->GetProtoMessage());
}

void ObjectUav::CheckTimer(uint64_t ms)
{
    if (!m_sock && m_bLogined)
    {
        if (auto ms = new ObjectSignal(this, ObjectGS::GSType(), ObjectSignal::S_Logout))
            SendMsg(ms);
    }

    ObjectAbsPB::CheckTimer(ms);
    if (ms-m_tmLastPos > 600000)
    {
        Release();
    }
    else if (m_sock)
    { 
        if (m_mission && !m_bSys && (uint32_t)ms-m_lastORNotify > 500)
            _notifyUavUOR(*m_mission);
        else if (ms-m_tmLastPos > 6000)//超时关闭
            m_sock->Close();
    }
}

void ObjectUav::OnConnected(bool bConnected)
{
    ObjectAbsPB::OnConnected(bConnected);
    if (m_sock && bConnected)
        m_sock->ResizeBuff(WRITE_BUFFLEN);

    if (bConnected)
        m_tmLastPos = Utility::msTimeTick();
    else
        savePos();
}

void ObjectUav::InitObject()
{
    if (m_stInit == IObject::Uninitial)
    {
        m_stInit = IObject::Initialing;
        if (DBMessage *msg = new DBMessage(this, IMessage::DeviceQueryRslt))
        {
            msg->SetSql("queryUavInfo");
            msg->SetCondition("id", m_id);
            SendMsg(msg);
        }
    }
}

void ObjectUav::_prcsRcvPostOperationInfo(PostOperationInformation *msg)
{
    if (!msg)
        return;

    m_tmLastPos = Utility::msTimeTick();
    int nCount = msg->oi_size();
    for (int i = 0; i < nCount; i++)
    {
        OperationInformation *oi = msg->mutable_oi(i);
        oi->set_uavid(GetObjectID());
        if (oi->has_gps() && oi->has_status() && m_mission && m_nCurRidge >= 0)
            _prcsGps(oi->gps(), oi->status().operationmode());
    }

    if (Uav2GSMessage *ms = new Uav2GSMessage(this, m_bBind?m_lastBinder:string()))
    {
        ms->AttachProto(msg);
        SendMsg(ms);
    }

    if (auto ack = new AckOperationInformation)
    {
        ack->set_seqno(msg->seqno());
        ack->set_result(1);
        WaitSend(ack);
    }
}

void ObjectUav::_prcsRcvPost2Gs(PostStatus2GroundStation *msg)
{
    mavLinkfilter(*msg);
    if (!msg || !m_bBind || m_lastBinder.length() < 1)
        return;

    if (Uav2GSMessage *ms = new Uav2GSMessage(this, m_lastBinder))
    {
        ms->SetPBContent(*msg);
        SendMsg(ms);
    }
}

void ObjectUav::_prcsRcvReqMissions(RequestRouteMissions *msg)
{
    if (!msg)
        return;

    auto ack = new AckRequestRouteMissions;
    if (!ack)
        return;

    bool hasItem = _hasMission(*msg);
    int offset = msg->offset();
    ack->set_seqno(msg->seqno());
    ack->set_result(hasItem ? 1 : 0);
    ack->set_boundary(msg->boundary());
    ack->set_offset(hasItem ? offset : -1);
    if (m_mission)
    {
        int count = msg->boundary() ? m_mission->boundarys_size() : m_mission->missions_size();
        for (int i = 0; i<msg->count()&&count>=offset+i; ++i)
        {
            const string &msItem = msg->boundary() ? m_mission->boundarys(i+offset):m_mission->missions(i+offset);
            ack->add_missions(msItem.c_str(), msItem.size());
        }
    }
    m_bSys = true;
    WaitSend(ack);
    if (!m_mission || !_isBind(m_lastBinder))
        return;

    if (SyscOperationRoutes *sys = new SyscOperationRoutes())
    {
        int off = offset < 0 ? -1 : (offset + (msg->boundary() ? m_mission->missions_size() : 0));
        sys->set_seqno(msg->seqno());
        sys->set_result(hasItem);
        sys->set_uavid(GetObjectID());
        sys->set_index(off);
        sys->set_count(m_mission->missions_size() + m_mission->boundarys_size());

        Uav2GSMessage *gsm = new Uav2GSMessage(this, m_lastBinder);
        gsm->AttachProto(sys);
        SendMsg(gsm);
    }
}

void ObjectUav::_prcsPosAuth(RequestPositionAuthentication *msg)
{
    if (!msg || !msg->has_pos() || Utility::Upper(msg->devid())!=GetObjectID())
        return;

    const GpsInformation &pos = msg->pos();
    if (auto ack = new AckPositionAuthentication)
    {
        ack->set_seqno(msg->seqno());
        int n = _checkPos(pos.latitude() / 10e7, pos.longitude() / 10e7, pos.altitude() / 10e3);
        ack->set_result(n);
        GetManager()->Log(0, GetObjectID(), 0, "Arm!");
        ack->set_devid(GetObjectID());
        WaitSend(ack);
        if (n==1 && m_mission)
            m_nCurRidge = m_mission->beg()-1;
    }
}

void ObjectUav::processBind(RequestBindUav *msg, const GS2UavMessage &g2u)
{
    bool bBind = msg->opid() == 1;
    int res = -1;
    string gs = g2u.GetSenderID();
    if (gs.length() == 0)
        return;

    bool bForce = (g2u.GetAuth() & ObjectGS::Type_UavManager)!=0;
    if (Initialed > m_stInit)
        res = -2;
    else if (bForce)
        res = 1;
    else 
        res = (gs==m_lastBinder || m_bBind==false) ? 1 : -3;

    if (res == 1 && (!bBind || !bForce))
    {
        m_lastBinder = gs;
        if (m_bBind!= bBind)
        {
            m_bBind = bBind;
            saveBind(bBind, gs, bForce);
        }
    }

    sendBindAck(msg->seqno(), res, m_bBind, m_lastBinder);
}

void ObjectUav::processControl2Uav(PostControl2Uav *msg)
{
    if (!msg)
        return;

    int res = 0;
    if (m_lastBinder == msg->userid() && m_bBind)
    {
        CopyAndSend(*msg);
        res = 1;
    }
    
    SendMsg(AckControl2Uav(*msg, res, this));
}

void ObjectUav::processPostOr(PostOperationRoute *msg, const std::string &gs)
{
    if(!msg || !msg->has_or_())
        return;

    const OperationRoute &mission = msg->or_();
    int ret = 0;
    if (_isBind(gs) && _parsePostOr(mission))
    {
        GetManager()->Log(0, mission.gsid(), 0, "Upload mission for %s!", GetObjectID().c_str());
        ret = 1;
        _notifyUavUOR(*m_mission);
    }
    if (Uav2GSMessage *ms = new Uav2GSMessage(this, mission.gsid()))
    {
        AckPostOperationRoute ack;
        ack.set_seqno(msg->seqno());
        ack.set_result(ret);
        ms->SetPBContent(ack);
        SendMsg(ms);
    }
}

void ObjectUav::processBaseInfo(const DBMessage &rslt)
{
    bool suc = rslt.GetRead(EXECRSLT).ToBool();
    m_stInit = suc ? Initialed : InitialFail;
    if (suc)
        InitialUAV(rslt, *this);
    else
        Release();

    OnLogined(suc);
    if (auto ack = new AckUavIdentityAuthentication)
    {
        ack->set_seqno(1);
        ack->set_result(suc ? 1 : 0);
        WaitSend(ack);
    }
}

bool ObjectUav::_isBind(const std::string &gs) const
{
    return m_bBind && m_lastBinder==gs && !m_lastBinder.empty();
}

bool ObjectUav::_hasMission(const das::proto::RequestRouteMissions &req) const
{
    if (!m_mission || req.offset()<0 || req.count()<=0)
        return false;

    return (req.offset() + req.count()) <= (req.boundary() ? m_mission->boundarys_size() : m_mission->missions_size());
}

void ObjectUav::_notifyUavUOR(const OperationRoute &ort)
{
    static uint32_t sSeqno = 1;
    if (auto upload = new UploadOperationRoutes)
    {
        upload->set_seqno(sSeqno++);
        upload->set_uavid(ort.uavid());
        upload->set_userid(ort.gsid());
        upload->set_timestamp(ort.createtime());
        upload->set_countmission(ort.missions_size());
        upload->set_countboundary(ort.boundarys_size());
        WaitSend(upload);
    }
    m_lastORNotify = (uint32_t)Utility::msTimeTick();
}

int ObjectUav::_checkPos(double lat, double lon, double alt)
{
    if (!IsValid())
        return 0;

    if (UavManager *m = (UavManager *)GetManager())
        return m->CanFlight(lat, lon, alt) ? 1 : 0;

    return 0;
}

void ObjectUav::_prcsGps(const GpsInformation &gps, const string &mod)
{
    m_lat = gps.latitude() / 1e7;
    m_lon = gps.longitude() / 1e7;
    if (m_bSys && mod==MissionMod)
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

        if (m_mission->end() == cur)
            _missionFinish(INVALIDLat, INVALIDLat);
    }
    else if (mod == Landing && m_mission->end() == m_nCurRidge)
    {
        _missionFinish(INVALIDLat, INVALIDLat);
    }
}

bool ObjectUav::_parsePostOr(const OperationRoute &sor)
{
    int rdSz = sor.ridgebeg_size();
    int beg = sor.beg();
    if (rdSz-1 != sor.end()-beg)
        return false;

    ReleasePointer(m_mission);
    m_fliedBeg = 0;
    m_mission = new OperationRoute();
    if (!m_mission)
        return false;

    m_bSys = false;
    m_mission->CopyFrom(sor);
    if (m_mission->createtime() == 0)
        m_mission->set_createtime(Utility::msTimeTick());
    m_ridges.clear();
    for (int i = 0; i < sor.ridgebeg_size(); ++i)
    {
        int item = sor.ridgebeg(i);
        RidgeDat &dat = m_ridges[item];
        dat.length = genRidgeLength(item);
        dat.idx = beg + i;
    }

    return true;
}

int32_t ObjectUav::getCurRidgeByItem(int32_t curItem)
{
    if (m_ridges.size() < 0 || curItem >= m_mission->missions_size())
        return -1;

    m_nCurItem = curItem;
    for (const pair<int32_t, RidgeDat> &itr : m_ridges)
    {
        if (curItem <= itr.first)
            return itr.second.idx-1;
    }
    return m_mission->end();
}

void ObjectUav::_missionFinish(int lat, int lon)
{
    if (!m_mission || m_nCurRidge<m_mission->beg()-1)
        return;

    double remainCur = 0;
    int ridgeFlying = GetOprRidge();
    if (DBMessage *msg = new DBMessage(this, IMessage::Unknown, DBMessage::DB_GS))
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
        if (m_mission->has_beg())
            msg->SetWrite("begin", m_mission->beg());

        if (ridgeFlying > m_nCurRidge)
        {
            int latT=INVALIDLat, lonT = INVALIDLat;
            if (getGeo(m_mission->missions(m_nCurItem), latT, latT))
            {
                remainCur = geoDistance(lat, lon, latT, lonT);
                double opLn = GetOprLength();
                if (remainCur > opLn)
                    remainCur = opLn;
            }
            msg->SetWrite("continiuLat", lat);
            msg->SetWrite("continiuLon", lon);
        }

        msg->SetWrite("end", m_nCurRidge);
        msg->SetWrite("acreage", calculateOpArea(remainCur));
        SendMsg(msg);
    }
    m_fliedBeg = (float)remainCur;
    if (m_nCurRidge < m_mission->end())
        m_mission->set_beg(ridgeFlying);
    m_nCurRidge = -1;
}

void ObjectUav::savePos()
{
    if (DBMessage *msg = new DBMessage(this))
    {
        msg->SetSql("updatePos");
        msg->SetCondition("id", m_id);
        msg->SetWrite("lat", m_lat);
        msg->SetWrite("lon", m_lon);
        msg->SetWrite("timePos", m_tmLastPos);
        if(!m_strSim.empty())
            msg->SetWrite("simID", m_strSim);

        SendMsg(msg);
    }
}

void ObjectUav::saveBind(bool bBind, const string &gs, bool bForce)
{
    if (bForce && bBind)
        return;

    if (DBMessage *msg = new DBMessage(this))
    {
        msg->SetSql("updateBinded");
        msg->SetWrite("binder", gs);
        msg->SetWrite("timeBind", Utility::msTimeTick());
        msg->SetWrite("binded", bForce ? false : bBind);
        msg->SetCondition("id", m_id);
        if (!bForce)
        {
            msg->SetCondition("UavInfo.binded", false);
            msg->SetCondition("UavInfo.binder", bForce ? gs : m_lastBinder);
        }

        SendMsg(msg);
        GetManager()->Log(0, gs, 0, "%s %s", bBind ? "bind" : "unbind", m_id.c_str());
    }
}

void ObjectUav::sendBindAck(int ack, int res, bool bind, const std::string &gs)
{
    if (Uav2GSMessage *ms = new Uav2GSMessage(this, gs))
    {
        AckRequestBindUav *proto = new AckRequestBindUav();
        if (!proto)
            return;

        proto->set_seqno(ack);
        proto->set_opid(bind ? 1 : 0);
        proto->set_result(res);
        if (UavStatus *s = new UavStatus)
        {
            TransUavStatus(*s);
            proto->set_allocated_status(s);
        }
        ms->AttachProto(proto);
        SendMsg(ms);
    }
}

void ObjectUav::mavLinkfilter(const PostStatus2GroundStation &msg)
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
            if (supports.assist_state & 4)
                _missionFinish(supports.int_latitude, supports.int_longitude);
        }
    }
}

double ObjectUav::genRidgeLength(int idx)
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

float ObjectUav::calculateOpArea(double remainCur)
{
    if (!m_mission || m_nCurItem<1 || m_nCurItem>m_mission->missions_size())
        return 0;

    if (m_nCurRidge >= m_mission->end())
        return m_mission->acreage();

    double allOp = 0;
    double oped = -m_fliedBeg;
    int opR = GetOprRidge();
    for (const pair<int32_t, RidgeDat> &itr : m_ridges)
    {
        allOp += itr.second.length;
        if (itr.second.idx<=opR && itr.second.idx>=m_mission->beg())
        {
            oped += itr.second.length;
            if (itr.second.idx == opR)
                oped -= remainCur;
        }
    }
    if (allOp < 0.0001)
        return m_mission->acreage();

    return float(m_mission->acreage()*oped / allOp);
}

int ObjectUav::GetOprRidge() const
{
    auto itr = m_ridges.find(m_nCurItem);
    if (itr == m_ridges.end())
        return m_nCurRidge;

    return itr->second.idx;
}

double ObjectUav::GetOprLength() const
{
    auto itr = m_ridges.find(m_nCurItem);
    if (itr == m_ridges.end())
        return 0;

    return itr->second.length;
}
