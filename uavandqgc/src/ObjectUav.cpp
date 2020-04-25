#include "ObjectUav.h"
#include "ObjectManagers.h"
#include "das.pb.h"
#include "ProtoMsg.h"
#include "Messages.h"
#include "DBMessages.h"
#include "VGMysql.h"
#include "Utility.h"
#include "socketBase.h"
#include "IMutex.h"
#include "UavManager.h"
#include "ObjectGS.h"

#define WRITE_BUFFLEN   1024
#define MissionFinish   9
#define MissionMod      "Mission"
#define ReturnMod       "Return"
typedef union {
    float tmp[7];
    PACKEDSTRUCT(struct {
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
////////////////////////////////////////////////////////////////////////////////
//ObjectUav
////////////////////////////////////////////////////////////////////////////////
ObjectUav::ObjectUav(const std::string &id, const std::string &sim): ObjectAbsPB(id)
, m_strSim(sim), m_bBind(false), m_lastORNotify(0), m_lat(200), m_lon(0)
, m_tmLastBind(0), m_tmLastPos(0),m_tmValidLast(-1)
, m_mission(NULL), m_nCurMsItem(-1), m_bSys(false)
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

void ObjectUav::RespondLogin(int seq, int res)
{
    if(m_p && m_sock)
    {
        if (auto ack = new AckUavIdentityAuthentication)
        {
            ack->set_seqno(seq);
            ack->set_result(res);
            send(ack);
        }
        GetManager()->Log(0, m_id, 0, "[%s:%d]%s", m_sock->GetHost().c_str(), m_sock->GetPort(), res==1 ? "logined" : "login fail");
    }
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

bool ObjectUav::transToMissionItems(const std::string &v, UavRoute &rt)
{
    int sz = v.size();
    if (sz > 0 && rt.ParseFromArray(v.c_str(), sz))  
        return true;

    return false;
}

bool ObjectUav::transFormMissionItems(Variant &v, const OperationRoute &ms)
{
    UavRoute rt;
    for (int i = 0; i < ms.missions_size(); ++i)
    {
        rt.add_missions(ms.missions(i));
    }
    string buff;
    buff.resize(rt.ByteSize());
    rt.SerializeToArray(&buff.front(), buff.size());
    v.SetBuff(buff.c_str(), buff.size());
    return true;
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
        processBind((RequestBindUav*)msg->GetContent(), msg->GetSender());
    else if (msg->GetMessgeType() == IMessage::ControlUav)
        processControl2Uav((PostControl2Uav*)msg->GetContent());
    else if (msg->GetMessgeType() == IMessage::PostOR)
        processPostOr((PostOperationRoute*)msg->GetContent(), msg->GetSenderID());
    else if (msg->GetMessgeType() == IMessage::UavQueryRslt)
        processBaseInfo(*(DBMessage *)msg);
}

int ObjectUav::ProcessReceive(void *buf, int len)
{
    int pos = 0;
    int l = len;
    while (m_p && m_p->Parse((char*)buf + pos, l))
    {
        const string &name = m_p->GetMsgName();
        if (name == d_p_ClassName(RequestUavIdentityAuthentication))
            RespondLogin(((RequestUavIdentityAuthentication*)m_p->GetProtoMessage())->seqno(), 1);
        else if (name == d_p_ClassName(PostOperationInformation))
            prcsRcvPostOperationInfo((PostOperationInformation *)m_p->DeatachProto());
        else if (name == d_p_ClassName(PostStatus2GroundStation))
            prcsRcvPost2Gs((PostStatus2GroundStation *)m_p->GetProtoMessage());
        else if (name == d_p_ClassName(RequestRouteMissions))
            prcsRcvReqMissions((RequestRouteMissions *)m_p->GetProtoMessage());
        else if (name == d_p_ClassName(RequestPositionAuthentication))
            prcsPosAuth((RequestPositionAuthentication *)m_p->GetProtoMessage());

        pos += l;
        l = len - pos;
    }
    pos += l;
    return pos;
}

void ObjectUav::CheckTimer(uint64_t ms)
{
    ObjectAbsPB::CheckTimer(ms);
    if (m_sock)
    { 
        if (m_mission && !m_bSys && (uint32_t)ms-m_lastORNotify > 500)
            _notifyUavUOR(*m_mission);
        int64_t n = ms - m_tmLastPos;
        if (n > 3600000)
            Release();
        else if (n>6000)//超时关闭
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
        if (DBMessage *msg = new DBMessage(this, IMessage::UavQueryRslt))
        {
            msg->SetSql("queryUavInfo");
            msg->SetCondition("id", m_id);
            SendMsg(msg);
        }
    }
}

void ObjectUav::prcsRcvPostOperationInfo(PostOperationInformation *msg)
{
    if (!msg)
        return;

    m_tmLastPos = Utility::msTimeTick();
    int nCount = msg->oi_size();
    for (int i = 0; i < nCount; i++)
    {
        OperationInformation *oi = msg->mutable_oi(i);
        oi->set_uavid(GetObjectID());
        if (oi->has_gps() || oi->has_status())
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
        send(ack);
    }
}

void ObjectUav::prcsRcvPost2Gs(PostStatus2GroundStation *msg)
{
    if (!msg || !m_bBind || m_lastBinder.length() < 1)
        return;

    if (Uav2GSMessage *ms = new Uav2GSMessage(this, m_lastBinder))
    {
        ms->SetPBContent(*msg);
        SendMsg(ms);
    }
}

void ObjectUav::prcsRcvReqMissions(RequestRouteMissions *msg)
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
    send(ack);
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

void ObjectUav::prcsPosAuth(RequestPositionAuthentication *msg)
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
        send(ack);
    }
}

void ObjectUav::processBind(RequestBindUav *msg, IObject *obj)
{
    ObjectGS *sender = (ObjectGS*)obj;
    bool bBind = msg->opid() == 1;
    int res = -1;
    string gs = sender ? sender->GetObjectID() : string();
    if (gs.length() == 0)
        return;

    bool bForce = sender && sender->GetAuth(ObjectGS::Type_UavManager);
    if (Initialed > m_stInit)
        res = -2;
    else if (bForce)
        res = 1;
    else 
        res = (gs==m_lastBinder || m_bBind==false) ? 1 : -3;

    if (res == 1 && (bBind != m_bBind && gs!=m_lastBinder))
    {
        m_bBind = bBind&&!bForce;
        m_lastBinder = gs;
        saveBind(bBind, gs, bForce);
    }
    if (sender && bForce)
    {
        if (bBind)
            sender->Subcribe(GetObjectID(), IMessage::PushUavSndInfo);
        else
            sender->Unsubcribe(GetObjectID(), IMessage::PushUavSndInfo);
    }

    sendBindAck(msg->seqno(), res, bBind, gs);
}

void ObjectUav::processControl2Uav(PostControl2Uav *msg)
{
    if (!msg)
        return;

    int res = 0;
    if (m_lastBinder == msg->userid() && m_bBind)
    {
        auto ms = new PostControl2Uav(*msg);
        send(ms);
        res = 1;
    }
    
    SendMsg(AckControl2Uav(*msg, res, this));
}

void ObjectUav::processPostOr(PostOperationRoute *msg, const std::string &gs)
{
    if(!msg)
        return;

    ReleasePointer(m_mission);
    const OperationRoute mission = msg->or_();
    int ret = 0;
    if (_isBind(gs))
    {
        m_mission = new OperationRoute();
        if (!m_mission)
            return;

        m_mission->CopyFrom(mission);
        if (m_mission->createtime() == 0)
            m_mission->set_createtime(Utility::msTimeTick());

        m_bSys = false;
        m_nCurMsItem = 0;
        ret = 1;
        GetManager()->Log(0, mission.gsid(), 0, "Upload mission for %s!", GetObjectID().c_str());
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
    if (suc)
        InitialUAV(rslt, *this);
    m_stInit = suc ? Initialed : InitialFail;
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
        send(upload);
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
    int itemCount = m_mission ? m_mission->missions_size() : 0;
    if (m_mission && m_bSys && (mod==MissionMod || mod==ReturnMod) && m_nCurMsItem>-1)
    {
        GpsAdtionValue gpsAdt = { 0 };
        int count = (sizeof(GpsAdtionValue) + sizeof(float) - 1) / sizeof(float);
        for (int i = 0; i < gps.velocity_size() && i < count; ++i)
        {
            gpsAdt.tmp[i] = gps.velocity(i);
        }
        if (gpsAdt.curMs+1 == itemCount)
            _missionFinish();
        else
            m_nCurMsItem = gpsAdt.curMs;
    }
}

void ObjectUav::_missionFinish()
{
    m_nCurMsItem = -1;
    if (!m_mission)
        return;

    if (DBMessage *msg = new DBMessage(this, IMessage::Unknown, DBMessage::DB_GS))
    {
        msg->SetSql("insertMissions");
        msg->SetWrite("userID", m_mission->gsid());
        msg->SetWrite("uavID", m_mission->uavid());
        if (m_mission->has_rpid())
            msg->SetWrite("planID", m_mission->rpid());
        if (m_mission->has_landid())
            msg->SetWrite("landId", m_mission->landid());

        auto curTm = Utility::msTimeTick();
        msg->SetWrite("finishTime", curTm);
        if (m_mission->has_beg())
            msg->SetWrite("begin", m_mission->beg());
        if (m_mission->has_end())
            msg->SetWrite("end", m_mission->end());
        Variant v;
        transFormMissionItems(v, *m_mission);
        msg->SetWrite("route", v);
        SendMsg(msg);
    }
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
