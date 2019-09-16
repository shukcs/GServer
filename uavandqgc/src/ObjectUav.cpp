#include "ObjectUav.h"
#include "ObjectManagers.h"
#include "das.pb.h"
#include "ProtoMsg.h"
#include "Messages.h"
#include "VGMysql.h"
#include "VGDBManager.h"
#include "DBExecItem.h"
#include "Utility.h"
#include "socketBase.h"
#include "IMutex.h"
#include "IMessage.h"
#include "UavManager.h"

using namespace std;
using namespace das::proto;
using namespace ::google::protobuf;
////////////////////////////////////////////////////////////////////////////////
//ObjectUav
////////////////////////////////////////////////////////////////////////////////
ObjectUav::ObjectUav(const std::string &id): ObjectAbsPB(id)
, m_bBind(false), m_lat(0), m_lon(0), m_tmLastInfo(0)
, m_tmLastBind(0), m_mission(NULL)
{
}

ObjectUav::~ObjectUav()
{
}

void ObjectUav::InitBySqlResult(const ExecutItem &sql)
{
    if (FiledVal *fd = sql.GetReadItem("binded"))
        m_bBind = fd->GetValue<char>()==1;
    if (FiledVal *fd = sql.GetReadItem("binder"))
        m_lastBinder = string((char*)fd->GetBuff(), fd->GetValidLen());
    if (FiledVal *fd = sql.GetReadItem("lat"))
        m_lat = fd->GetValue<double>();
    if (FiledVal *fd = sql.GetReadItem("lon"))
        m_lon = fd->GetValue<double>();
    if (FiledVal *fd = sql.GetReadItem("timeBind"))
        m_tmLastBind = fd->GetValue<int64_t>();
    if (FiledVal *fd = sql.GetReadItem("timePos"))
        m_tmLastInfo = fd->GetValue<int64_t>();
}

void ObjectUav::transUavStatus(UavStatus &us)
{
    us.set_result(1);
    us.set_uavid(GetObjectID());
    if(m_lastBinder.length()>0)
        us.set_binder(m_lastBinder);
    us.set_binded(m_bBind);
    if (m_bBind)
        us.set_bindtime(m_tmLastBind);
    else
        us.set_unbindtime(m_tmLastBind);
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
    return IObject::Plant;
}

void ObjectUav::OnConnected(bool bConnected)
{
    ObjectAbsPB::OnConnected(bConnected);
    m_tmLastInfo = Utility::msTimeTick();
}

void ObjectUav::RespondLogin(int seq, int res)
{
    if(m_p && m_sock)
    {
        AckUavIdentityAuthentication ack;
        ack.set_seqno(seq);
        ack.set_result(res);
        send(ack);
    }
}

void ObjectUav::AckControl2Uav(const PostControl2Uav &msg, int res, ObjectUav *obj)
{
    GSMessage *ms = NULL;
    if (obj)
        ms = new GSMessage(obj, msg.userid());
    else
        ms = new GSMessage(GetManagerByType(IObject::Plant), msg.userid());

    if (ms)
    {
        AckPostControl2Uav *ack = new AckPostControl2Uav;
        if (!ack)
        {
            delete ms;
            return;
        }
        ack->set_seqno(msg.seqno());
        ack->set_result(res);
        ack->set_uavid(msg.uavid());
        ack->set_userid(msg.userid());
        ms->AttachProto(ack);
        SendMsg(ms);
    }
}

void ObjectUav::ProcessMassage(const IMessage &msg)
{
    if (msg.GetMessgeType() == BindUav)
        processBind((RequestBindUav*)msg.GetContent());
    else if (msg.GetMessgeType() == ControlUav)
        processControl2Uav((PostControl2Uav*)msg.GetContent());
    else if (msg.GetMessgeType() == PostOR)
        processPostOr((PostOperationRoute*)msg.GetContent());
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

        pos += l;
        l = len - pos;
    }
    pos += l;
    return pos;
}

VGMySql *ObjectUav::GetMySql() const
{
    return ((UavManager*)GetManager())->GetMySql();
}

void ObjectUav::prcsRcvPostOperationInfo(PostOperationInformation *msg)
{
    if (!msg)
        return;

    m_tmLastInfo = Utility::msTimeTick();
    int nCount = msg->oi_size();
    for (int i = 0; i < nCount; i++)
    {
        OperationInformation *oi = msg->mutable_oi(i);
        oi->set_uavid(GetObjectID());
        if (oi->has_gps())
        {
            const GpsInformation &gps = oi->gps();
            m_lat = gps.latitude() / 1e7;
            m_lon = gps.longitude() / 1e7;
        }
    }

    GSMessage *ms = new GSMessage(this, m_lastBinder);
    if (ms && m_bBind && m_lastBinder.length() > 0)
    {
        ms->AttachProto(msg);
        SendMsg(ms);
    }
   
    AckOperationInformation ack;
    ack.set_seqno(msg->seqno());
    ack.set_result(1);
    send(ack);
}

void ObjectUav::prcsRcvPost2Gs(PostStatus2GroundStation *msg)
{
    if (!msg || !m_bBind || m_lastBinder.length() < 1)
        return;

    if (GSMessage *ms = new GSMessage(this, m_lastBinder))
    {
        ms->SetPBContentPB(*msg);
        SendMsg(ms);
    }
}

void ObjectUav::prcsRcvReqMissions(RequestRouteMissions *msg)
{
    if (!msg)
        return;

    AckRequestRouteMissions ack;
    bool hasItem = _hasMission(*msg);
    int offset = msg->offset();
    ack.set_seqno(msg->seqno());
    ack.set_result(hasItem ? 1 : 0);
    ack.set_boundary(msg->boundary());
    ack.set_offset(hasItem ? offset : -1);
    if (m_mission)
    {
        int count = msg->boundary() ? m_mission->boundarys_size() : m_mission->missions_size();
        for (int i = offset; i < count; ++i)
        {
            const string &msItem = msg->boundary() ? m_mission->boundarys(i):m_mission->missions(i);
            ack.add_missions(msItem.c_str(), msItem.size());
        }
    }
    send(ack);
    if (!_isBind(m_lastBinder))
        return;

    if (SyscOperationRoutes *sys = new SyscOperationRoutes())
    {
        int off = offset < 0 ? -1 : (offset + (msg->boundary() ? m_mission->missions_size() : 0));
        sys->set_seqno(msg->seqno());
        sys->set_result(hasItem);
        sys->set_uavid(GetObjectID());
        sys->set_index(off);
        sys->set_count(m_mission->missions_size() + m_mission->boundarys_size());

        GSMessage *gsm = new GSMessage(this, m_lastBinder);
        gsm->AttachProto(sys);
        SendMsg(gsm);
    }
}

void ObjectUav::processBind(RequestBindUav *msg)
{
    if (UavManager *m = (UavManager *)GetManager())
    {
        if (1 == m->PrcsBind(msg, m_lastBinder))
        {
            m_bBind = 1 == msg->opid();
            m_lastBinder = msg->binder();
        }
    }
}

void ObjectUav::processControl2Uav(PostControl2Uav *msg)
{
    if (!msg)
        return;

    int res = 0;
    if (m_lastBinder == msg->userid() && m_bBind)
    {
        res = 1;
        send(*msg);
    }
    
    AckControl2Uav(*msg, res, this);
}

void ObjectUav::processPostOr(PostOperationRoute *msg)
{
    if(!msg)
        return;

    ReleasePointer(m_mission);
    const OperationRoute mission = msg->or_();
    int ret = 0;
    if (_isBind(mission.gsid()) && (m_mission = new OperationRoute()))
    {
        m_mission->CopyFrom(mission);
        if (m_mission->createtime() == 0)
            m_mission->set_createtime(Utility::msTimeTick());
        UploadOperationRoutes upload;
        upload.set_seqno(msg->seqno());
        upload.set_uavid(mission.uavid());
        upload.set_userid(mission.gsid());
        upload.set_timestamp(m_mission->createtime());
        upload.set_countmission(mission.missions_size());
        upload.set_countboundary(mission.boundarys_size());
        send(upload);
        ret = 1;
    }
    if (GSMessage *ms = new GSMessage(this, mission.gsid()))
    {
        AckPostOperationRoute ack;
        ack.set_seqno(msg->seqno());
        ack.set_result(ret);
        ms->SetPBContentPB(ack);
        SendMsg(ms);
    }
}

bool ObjectUav::_isBind(const std::string &gs) const
{
    return m_bBind && m_lastBinder == gs && !m_lastBinder.empty();
}

bool ObjectUav::_hasMission(const das::proto::RequestRouteMissions &req) const
{
    if (m_mission || req.offset()<0 || req.count()<=0)
        return false;

    return (req.offset() + req.count()) < (req.boundary() ? m_mission->boundarys_size() : m_mission->missions_size());
}
