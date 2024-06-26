﻿#include "ObjectUav.h"
#include "ObjectManagers.h"
#include "das.pb.h"
#include "ProtoMsg.h"
#include "Messages.h"
#include "DBMessages.h"
#include "VGMysql.h"
#include "common/Utility.h"
#include "net/socketBase.h"
#include "UavManager.h"
#include "ObjectGS.h"
#include "UavMission.h"
#include "GXClient.h"

#define WRITE_BUFFLEN  1024
#define NODATARELEASETM  600000

using namespace std;
using namespace das::proto;
using namespace ::google::protobuf;
#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif
///////////////////////////////////////////////////////////////////////////////////////////////////////////
//ObjectUav
///////////////////////////////////////////////////////////////////////////////////////////////////////////
ObjectUav::ObjectUav(const string &id, const string &sim) : ObjectAbsPB(id), m_mission(new UavMission(this))
, m_strSim(sim), m_bBind(false), m_lat(200), m_lon(0), m_tmLastBind(0), m_tmLastPos(0), m_tmValidLast(-1)
, m_bSendGx(false)
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
    if (auto ack = new AckUavIdentityAuthentication)
    {
        ack->set_seqno(seq);
        ack->set_result(res);
        WaitSend(ack);
    }
    SetLogined(true);
}

void ObjectUav::SetLogined(bool suc, ISocket *s)
{
    if (IsConnect()!=suc && suc)
    {
		SendMsg(new ObjectSignal(this, ObjectGS::GSType(), ObjectSignal::S_Login));
		SendMsg(new ObjectSignal(this, GXClient::GXClientType(), ObjectSignal::S_Login));
    }
    ObjectAbsPB::SetLogined(suc, s);
}

void ObjectUav::RefreshRcv(int64_t ms)
{
    m_tmLastPos = ms;
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

void ObjectUav::SndBinder(const google::protobuf::Message &pb)
{
    if (m_bBind && !m_lastBinder.empty())
    {
        if (Uav2GSMessage *ms = new Uav2GSMessage(*this, m_lastBinder))
        {
            ms->SetPBContent(pb);
            SendMsg(ms);
        }
    }
}

const string &ObjectUav::GetBinder() const
{
    return m_bBind ? m_lastBinder : Utility::EmptyStr();
}

int ObjectUav::UAVType()
{
    return IObject::Uav;
}

void ObjectUav::InitialUAV(const DBMessage &rslt, ObjectUav &uav, uint32_t idx)
{
    const Variant &binded = rslt.GetRead("binded", idx);
    if (!binded.IsNull())
        uav.m_bBind = binded.ToInt8() != 0;

    uav.m_lastBinder = rslt.GetRead("binder", idx).ToString();
    const Variant &lat = rslt.GetRead("lat", idx);
    if (lat.GetType() == Variant::Type_double)
        uav.m_lat = lat.ToDouble();
    const Variant &lon = rslt.GetRead("lon", idx);
    if (lon.GetType() == Variant::Type_double)
        uav.m_lon = lon.ToDouble();
    const Variant &timeBind = rslt.GetRead("timeBind", idx);
    if (!timeBind.IsNull())
        uav.m_tmLastBind = timeBind.ToInt64();
    const Variant &timePos = rslt.GetRead("timePos", idx);
    if (!timePos.IsNull())
        uav.m_tmLastBind = timePos.ToInt64();
    const Variant &valid = rslt.GetRead("valid", idx);
    if (!valid.IsNull())
        uav.m_tmValidLast = valid.ToInt64();
    uav.m_authCheck = rslt.GetRead("authCheck", idx).ToString();
}

IMessage *ObjectUav::AckControl2Uav(const PostControl2Uav &msg, int res, ObjectUav *obj)
{
    Uav2GSMessage *ms = NULL;
    if (obj)
        ms = new Uav2GSMessage(*obj, msg.userid());
    else
        ms = new Uav2GSMessage(GetManagerByType(IObject::Uav), msg.userid());

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

void ObjectUav::ProcessMessage(const IMessage *msg)
{
    if (!msg)
        return;

    switch (msg->GetMessgeType())
    {
    case IMessage::BindUav:
        processBind(*(UavStatus*)msg->GetContent()); break;
    case IMessage::ControlDevice:
        processControl2Uav((PostControl2Uav*)msg->GetContent()); break;
    case IMessage::ControlDevice2:
        processRequestPost((Message*)msg->GetContent(), msg->GetSenderID()); break;
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

bool ObjectUav::ProcessRcvPack(const void *pack)
{
    auto proto = (Message*)pack;
    const string &name = proto->GetDescriptor()->full_name();
    if (name == d_p_ClassName(RequestUavIdentityAuthentication))
        _respondLogin(((RequestUavIdentityAuthentication*)proto)->seqno(), 1);
    else if (name == d_p_ClassName(PostOperationInformation))
        _prcsRcvPostOperationInfo((PostOperationInformation *)proto);
    else if (name == d_p_ClassName(PostStatus2GroundStation))
        _prcsRcvPost2Gs((PostStatus2GroundStation *)proto);
    else if (name == d_p_ClassName(PostOperationAssist))
        _prcsAssist((PostOperationAssist *)proto);
    else if (name == d_p_ClassName(PostABPoint))
        _prcsABPoint((PostABPoint *)proto);
    else if (name == d_p_ClassName(PostOperationReturn))
        _prcsReturn((PostOperationReturn *)proto);
    else if (name == d_p_ClassName(RequestRouteMissions))
        _prcsRcvReqMissions((RequestRouteMissions *)proto);
    else if (name == d_p_ClassName(RequestPositionAuthentication))
        _prcsPosAuth((RequestPositionAuthentication *)proto);
    else if (name == d_p_ClassName(PostBlocks))
        _prcsPostBlocks((PostBlocks *)proto);
    else if (name == d_p_ClassName(PostABOperation))
        _prcsABOperation((PostABOperation *)proto);

    return IsInitaled();
}

void ObjectUav::CheckTimer(uint64_t ms)
{
    auto msDiff = ms - m_tmLastPos;
    if (msDiff > NODATARELEASETM)
    {
        ReleaseObject();
    }
    else if (msDiff>6000 && IsLinked())//超时关闭
    {
        CloseLink();
    }
    else if (IsLinked() && m_mission)
    {
        if (auto nor = m_mission->GetNotifyUavUOR(uint32_t(ms)))
            WaitSend(nor);
    }
}

void ObjectUav::OnConnected(bool bConnected)
{
    if (bConnected)
	{
		SetSocketBuffSize(WRITE_BUFFLEN);
	}
	else
	{
		if (m_mission)
			m_mission->Clear();

		SendMsg(new ObjectSignal(this, ObjectGS::GSType(), ObjectSignal::S_Logout));
		SendMsg(new ObjectSignal(this, GXClient::GXClientType(), ObjectSignal::S_Logout));
		savePos();
	}
}

void ObjectUav::InitObject()
{
    if (!IsInitaled())
    {
		IObject::InitObject();
        if (DBMessage *msg = new DBMessage(*this, IMessage::DeviceQueryRslt))
        {
            msg->SetSql("queryUavInfo");
            msg->SetCondition("id", m_id);
            SendMsg(msg);
        }
		StartTimer(500);
	}
}

void ObjectUav::_prcsRcvPostOperationInfo(PostOperationInformation *msg)
{
    if (!msg)
        return;

    auto tm = Utility::msTimeTick();
    if (IsLinked())
        m_tmLastPos = tm;

    if (auto ms = new Uav2GSMessage(*this, m_bBind?m_lastBinder:string()))
    {
        ms->SetPBContent(*msg);
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
    if (!msg)
        return;

    if (m_mission)
        m_mission->MavLinkfilter(*msg);

    if (msg->data_size() > 0)
        SndBinder(*msg);
}

void ObjectUav::_prcsAssist(PostOperationAssist *msg)
{
    if (!msg)
        return;

    SndBinder(*msg);
    if (auto ack = new AckOperationReturn)
    {
        ack->set_seqno(msg->seqno());
        ack->set_result(1);
        WaitSend(ack);
    }

    if (m_mission)
        m_mission->PrcsPostAssists(msg);
    else
        delete msg;
}

void ObjectUav::_prcsABPoint(PostABPoint *msg)
{
    if (!msg)
        return;

    SndBinder(*msg);
    if (auto ack = new AckPostABPoint)
    {
        ack->set_seqno(msg->seqno());
        ack->set_result(1);
        WaitSend(ack);
    }

    if (m_mission)
        m_mission->PrcsPostABPoint(msg);
    else
        delete msg;
}

void ObjectUav::_prcsReturn(PostOperationReturn *msg)
{

    if (!msg)
        return;

    SndBinder(*msg);
    if (auto ack = new AckOperationReturn)
    {
        ack->set_seqno(msg->seqno());
        ack->set_result(1);
        WaitSend(ack);
    }

    if (m_mission)
        m_mission->PrcsPostReturn(msg);
    else
        delete msg;
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

            Uav2GSMessage *gsm = new Uav2GSMessage(*this, m_lastBinder);
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
        GetManager()->Log(0, IObjectManager::GetObjectFlagID(this), 0, "Arm!");
        ack->set_devid(GetObjectID());
        WaitSend(ack);
    }
}

void ObjectUav::_prcsPostBlocks(das::proto::PostBlocks *msg)
{
    if (!msg || Utility::Upper(msg->uavid()) != GetObjectID())
        return;

    if (auto ack = new AckBlocks)
    {
        ack->set_seqno(msg->seqno());
        ack->set_result(1);
        WaitSend(ack);
    }
    SndBinder(*msg);
}

void ObjectUav::_prcsABOperation(PostABOperation *msg)
{
    if (!msg || Utility::Upper(msg->uavid()) != GetObjectID())
        return;

    if (auto ack = new AckABOperation)
    {
        ack->set_seqno(msg->seqno());
        ack->set_result(1);
        WaitSend(ack);
    }
    if (m_mission)
        m_mission->PrcsABOperation(msg);
}

void ObjectUav::processBind(const UavStatus &msg)
{
    m_lastBinder = msg.binder();
    m_bBind = msg.binded();
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

void ObjectUav::processRequestPost(Message *msg, const string &gs)
{
    if (!msg || !m_mission || gs.empty())
        return;

    Message *pb = UavMission::AckRequestPost(*m_mission, msg);
    if (pb)
        SndBinder(*pb);
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
    if (Uav2GSMessage *ms = new Uav2GSMessage(*this, mission.gsid()))
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
	else ///不存在的UAV
		ReleaseObject();

    SetLogined(suc);
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
    if (DBMessage *msg = new DBMessage(*this))
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

void ObjectUav::sendGx(const das::proto::PostOperationInformation &msg, uint64_t tm)
{
    auto ms = new Uav2GXMessage(*this);
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
