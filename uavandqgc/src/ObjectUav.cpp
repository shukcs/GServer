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
#include "UavMission.h"
#include "GXClient.h"

#define WRITE_BUFFLEN  1024

using namespace std;
using namespace das::proto;
using namespace ::google::protobuf;
#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif
///////////////////////////////////////////////////////////////////////////////////////////////////////////
//ObjectUav
///////////////////////////////////////////////////////////////////////////////////////////////////////////
ObjectUav::ObjectUav(const string &id, const string &sim) : ObjectAbsPB(id), m_mission(NULL), m_strSim(sim)
, m_bBind(false), m_lat(200), m_lon(0), m_tmLastBind(0), m_tmLastPos(0), m_tmValidLast(-1), m_bSendGx(false)
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
    if (m_bLogined != suc)
    {
        if (suc)
        {
            SendMsg(new ObjectSignal(this, ObjectGS::GSType(), ObjectSignal::S_Login));
            SendMsg(new ObjectSignal(this, GXClient::GXClinetType(), ObjectSignal::S_Login));
        }
        else
        {
            savePos();
        }
    }
    ObjectAbsPB::OnLogined(suc, s);
}

void ObjectUav::FreshLogin(uint64_t ms)
{
    m_tmLastPos = ms;
    ILink::FreshLogin(ms);
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
    switch (msg->GetMessgeType())
    {
    case IMessage::BindUav:
        processBind((RequestBindUav*)msg->GetContent(), *(GS2UavMessage*)msg); break;
    case IMessage::ControlDevice:
        processControl2Uav((PostControl2Uav*)msg->GetContent()); break;
    case IMessage::PostOR:
        processPostOr((PostOperationRoute*)msg->GetContent(), msg->GetSenderID()); break;
    case IMessage::DeviceQueryRslt:
        processBaseInfo(*(DBMessage *)msg); break;
    case IMessage::GXClinetStat:
        processGxStat(*(GX2UavMessage *)msg); break;
    default:
        break;
    }
}

void ObjectUav::PrcsProtoBuff(uint64_t tm)
{
    if (!m_p)
        return;

    const string &name = m_p->GetMsgName();
    if (name == d_p_ClassName(RequestUavIdentityAuthentication))
        _respondLogin(((RequestUavIdentityAuthentication*)m_p->GetProtoMessage())->seqno(), 1);
    else if (name == d_p_ClassName(PostOperationInformation))
        _prcsRcvPostOperationInfo((PostOperationInformation *)m_p->DeatachProto(), tm);
    else if (name == d_p_ClassName(PostStatus2GroundStation))
        _prcsRcvPost2Gs((PostStatus2GroundStation *)m_p->GetProtoMessage());
    else if (name == d_p_ClassName(RequestRouteMissions))
        _prcsRcvReqMissions((RequestRouteMissions *)m_p->GetProtoMessage());
    else if (name == d_p_ClassName(RequestPositionAuthentication))
        _prcsPosAuth((RequestPositionAuthentication *)m_p->GetProtoMessage());
}

void ObjectUav::CheckTimer(uint64_t ms, char *buf, int len)
{
    if (!m_sock && m_bLogined)
    {
        SendMsg(new ObjectSignal(this, ObjectGS::GSType(), ObjectSignal::S_Logout));
        SendMsg(new ObjectSignal(this, GXClient::GXClinetType(), ObjectSignal::S_Logout));
    }

    ObjectAbsPB::CheckTimer(ms, buf, len);
    if (ms-m_tmLastPos > 600000)
    {
        Release();
    }
    else if (m_sock)
    {
        auto nor = m_mission ? m_mission->GetNotifyUavUOR(uint32_t(ms)) : NULL;
        if (nor)
            WaitSend(nor);
        else if (ms-m_tmLastPos > 6000)//超时关闭
            m_sock->Close();
    }
}

void ObjectUav::OnConnected(bool bConnected)
{
    if (m_sock && bConnected)
        m_sock->ResizeBuff(WRITE_BUFFLEN);
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

void ObjectUav::_prcsRcvPostOperationInfo(PostOperationInformation *msg, uint64_t tm)
{
    if (!msg)
        return;

    m_tmLastPos = tm;
    if (m_mission)
        m_mission->PrcsRcvPostOperationInfo(*msg);

    if (auto ms = new Uav2GSMessage(this, m_bBind?m_lastBinder:string()))
    {
        ms->AttachProto(msg);
        SendMsg(ms);
    }

    if (m_bSendGx)
        sendGx(*msg, tm);

    if (auto ack = new AckOperationInformation)
    {
        ack->set_seqno(msg->seqno());
        ack->set_result(1);
        WaitSend(ack);
    }
}

void ObjectUav::_prcsRcvPost2Gs(PostStatus2GroundStation *msg)
{
    if (m_mission)
        m_mission->MavLinkfilter(*msg);

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

    auto ack = (AckRequestRouteMissions*)(m_mission ? m_mission->PrcsRcvReqMissions(* msg) : NULL);
    if (ack)
    {
        WaitSend(ack);
        if (!m_mission || !_isBind(m_lastBinder))
            return;

        if (SyscOperationRoutes *sys = new SyscOperationRoutes())
        {
            int off = msg->offset()+ msg->count()/2;
            if (off < 0)
                off = -1;
            else if (msg->boundary())
                off += m_mission->CountItems();

            sys->set_seqno(msg->seqno());
            sys->set_result(ack->result());
            sys->set_uavid(GetObjectID());
            sys->set_index(off);
            sys->set_count(m_mission->CountAll());

            Uav2GSMessage *gsm = new Uav2GSMessage(this, m_lastBinder);
            gsm->AttachProto(sys);
            SendMsg(gsm);
        }
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
        if (m_mission)
            m_mission->ProcessArm(n==1);
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
    if (!m_mission)
        m_mission = new UavMission(this);
    if(!m_mission || !msg || !msg->has_or_())
        return;

    const OperationRoute &mission = msg->or_();
    int ret = 0;
    if (_isBind(gs) && m_mission->ParsePostOr(mission))
    {
        ret = 1;
        if (auto nor = m_mission->GetNotifyUavUOR((uint32_t)Utility::msTimeTick()))
            WaitSend(nor);
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

void ObjectUav::processGxStat(const GX2UavMessage &msg)
{
    m_bSendGx = msg.GetStat() >= GXClient::St_Authed;
}

bool ObjectUav::_isBind(const std::string &gs) const
{
    return m_bBind && m_lastBinder==gs && !m_lastBinder.empty();
}

int ObjectUav::_checkPos(double lat, double lon, double alt)
{
    if (!IsValid())
        return 0;

    if (UavManager *m = (UavManager *)GetManager())
        return m->CanFlight(lat, lon, alt) ? 1 : 0;

    return 0;
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

void ObjectUav::sendGx(const das::proto::PostOperationInformation &msg, uint64_t tm)
{
    auto ms = new Uav2GXMessage(this);
    auto poi = new PostOperationInformation();
    if (ms && poi && msg.oi_size() > 0)
    {
        poi->set_seqno(msg.seqno());
        const OperationInformation &pinf = msg.oi(0);
        auto info = poi->add_oi();
        info->set_uavid(GetObjectID());
        info->set_timestamp(tm);
        info->set_allocated_gps(new GpsInformation(pinf.gps()));
        if (pinf.has_attitude())
            info->set_allocated_attitude(new UavAttitude(pinf.attitude()));
        if (pinf.has_status())
            info->set_allocated_status(new OperationStatus(pinf.status()));

        ms->AttachProto(poi);
        SendMsg(ms);
    }
    else
    {
        delete ms;
        delete poi;
    }
}
