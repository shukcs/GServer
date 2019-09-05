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
ObjectUav::ObjectUav(const std::string &id): IObject(NULL, id)
, m_bBind(false), m_bConnected(false), m_lat(0), m_lon(0)
, m_tmLastInfo(0), m_tmLastBind(0)
, m_p(new ProtoMsg)
{
}

ObjectUav::~ObjectUav()
{
    delete m_p;
}

bool ObjectUav::IsConnect() const
{
    return m_bConnected;
}

void ObjectUav::InitBySqlResult(const ExecutItem &sql)
{
    if (FiledValueItem *fd = sql.GetReadItem("binded"))
        m_bBind = fd->GetValue<char>()==1;
    if (FiledValueItem *fd = sql.GetReadItem("binder"))
        m_lastBinder = string((char*)fd->GetBuff(), fd->GetValidLen());
    if (FiledValueItem *fd = sql.GetReadItem("lat"))
        m_lat = fd->GetValue<double>();
    if (FiledValueItem *fd = sql.GetReadItem("lon"))
        m_lon = fd->GetValue<double>();
    if (FiledValueItem *fd = sql.GetReadItem("timeBind"))
        m_tmLastBind = fd->GetValue<int64_t>();
    if (FiledValueItem *fd = sql.GetReadItem("timePos"))
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
    m_bConnected = bConnected;
    if (!bConnected)
    {
        ((UavManager *)GetManager())->UpdatePos(GetObjectID(), m_lat, m_lon);

        ISocketManager *m = m_sock ? m_sock->GetManager() : NULL;
        if (m)
        {
            m->Log(0, GetObjectID(), 0, "disconnect");
            m_sock->Close();
            m_sock = NULL;
        }
    }
    
    m_tmLastInfo = Utility::msTimeTick();
}

void ObjectUav::RespondLogin(int seq, int res)
{
    if(m_p && m_sock)
    {
        AckUavIdentityAuthentication ack;
        ack.set_seqno(seq);
        ack.set_result(res);
        _send(ack);
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

        pos += l;
        l = len - pos;
    }
    pos += l;
    return pos;
}

int ObjectUav::GetSenLength() const
{
    if (m_p)
        return m_p->RemaimedLength();

    return 0;
}

int ObjectUav::CopySend(char *buf, int sz, unsigned form)
{
    if (m_p)
        return m_p->CopySend(buf, sz, form);

    return 0;
}

void ObjectUav::SetSended(int sended /*= -1*/)
{
    if (m_p)
        m_p->SetSended(sended);
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
    _send(ack);
}

void ObjectUav::prcsRcvPost2Gs(PostStatus2GroundStation *msg)
{
    if (!msg || !m_bBind || m_lastBinder.length() < 1)
        return;

    if (GSMessage *ms = new GSMessage(this, m_lastBinder))
    {
        ms->SetGSContent(*msg);
        SendMsg(ms);
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
        _send(*msg);
    }
    
    AckControl2Uav(*msg, res, this);
}

void ObjectUav::_send(const google::protobuf::Message &msg)
{
    if (m_p && m_sock)
        m_p->SendProto(msg, m_sock);
}