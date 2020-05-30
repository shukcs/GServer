#include "ObjectTracker.h"
#include "ObjectManagers.h"
#include "das.pb.h"
#include "ProtoMsg.h"
#include "Messages.h"
#include "Utility.h"
#include "socketBase.h"
#include "TrackerManager.h"
#include "ObjectGV.h"
#include "GXClient.h"

#define WRITE_BUFFLEN 1024
using namespace std;
using namespace das::proto;
using namespace ::google::protobuf;
#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif
///////////////////////////////////////////////////////////////////////////////////////////////////////////
//ObjectTracker
///////////////////////////////////////////////////////////////////////////////////////////////////////////
ObjectTracker::ObjectTracker(const string &id, const string &sim) : ObjectAbsPB(id), m_strSim(sim),
m_lat(200), m_lon(0), m_tmLastPos(0), m_tmValidLast(-1)
{
    SetBuffSize(1024 * 2);
}

ObjectTracker::~ObjectTracker()
{
}

int ObjectTracker::GetObjectType() const
{
    return TrackerType();
}

void ObjectTracker::_respondLogin(int seq, int res)
{
    if(m_p && m_sock)
    {
        if (auto ack = new AckTrackerIdentityAuthentication)
        {
            ack->set_seqno(seq);
            ack->set_result(res);
            WaitSend(ack);
        }
        OnLogined(true);
    }
}

void ObjectTracker::OnLogined(bool suc, ISocket *s)
{
    if (m_bLogined != suc && suc)
    {
        if (auto ms = new ObjectEvent(this, string(), ObjectGV::GVType(), ObjectEvent::E_Login))
            SendMsg(ms);
        if (auto ms = new ObjectEvent(this, string(), ObjectGXClinet::GXClinetType(), ObjectEvent::E_Login))
            SendMsg(ms);
    }
    ObjectAbsPB::OnLogined(suc, s);
}

bool ObjectTracker::IsValid() const
{
    return m_tmValidLast<0 || m_tmValidLast>Utility::msTimeTick();
}

void ObjectTracker::SetValideTime(int64_t tmV)
{
    m_tmValidLast = tmV;
}

void ObjectTracker::SetSimId(const std::string &sim)
{
    m_strSim = sim;
}

int ObjectTracker::TrackerType()
{
    return IObject::User + 1;
}

void ObjectTracker::ProcessMessage(IMessage *msg)
{
}

void ObjectTracker::PrcsProtoBuff()
{
    if (!m_p)
        return;

    const string &name = m_p->GetMsgName();
    if (name == d_p_ClassName(RequestTrackerIdentityAuthentication))
        _respondLogin(((RequestTrackerIdentityAuthentication*)m_p->GetProtoMessage())->seqno(), 1);
    else if (name == d_p_ClassName(RequestPositionAuthentication))
        _prcsPosAuth((RequestPositionAuthentication *)m_p->GetProtoMessage());
    else if (name == d_p_ClassName(PostOperationInformation))
        _prcsOperationInformation((PostOperationInformation *)m_p->DeatachProto());
}

int ObjectTracker::_checkPos(double lat, double lon, double alt)
{
    if (!IsValid())
        return 0;

    if (TrackerManager *m = (TrackerManager *)GetManager())
        return m->CanFlight(lat, lon, alt) ? 1 : 0;

    return 0;
}

void ObjectTracker::CheckTimer(uint64_t ms)
{
    if (!m_sock && m_bLogined)
    {
        if (auto ms = new ObjectEvent(this, string(), ObjectGV::GVType(), ObjectEvent::E_Logout))
            SendMsg(ms);
        if (auto ms = new ObjectEvent(this, string(), ObjectGXClinet::GXClinetType(), ObjectEvent::E_Logout))
            SendMsg(ms);
    }
    ObjectAbsPB::CheckTimer(ms);
    if (m_sock)
    { 
        int64_t n = ms - m_tmLastPos;
        if (n > 3600000)
            Release();
        else if (n>6000)//��ʱ�ر�
            m_sock->Close();
    }
}

void ObjectTracker::OnConnected(bool bConnected)
{
    ObjectAbsPB::OnConnected(bConnected);
    if (m_sock && bConnected)
        m_sock->ResizeBuff(WRITE_BUFFLEN);

    if (bConnected)
        m_tmLastPos = Utility::msTimeTick();
}

void ObjectTracker::InitObject()
{
    if (m_stInit == IObject::Uninitial)
    {
        _respondLogin(1, 1);
        m_stInit = IObject::Initialed;
    }
}

void ObjectTracker::_prcsPosAuth(RequestPositionAuthentication *msg)
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
        WaitSend(ack);
    }
}

void ObjectTracker::_prcsOperationInformation(PostOperationInformation *msg)
{
    if (!msg)
        return;

    m_tmLastPos = Utility::msTimeTick();
    if (Tracker2GVMessage *ms = new Tracker2GVMessage(this, string()))
    {
        ms->AttachProto(msg);
        SendMsg(ms);
    }
    if (auto *ms = new Tracker2GXMessage(this))
    {
        ms->SetPBContent(*msg);
        SendMsg(ms);
    }

    if (auto ack = new AckOperationInformation)
    {
        ack->set_seqno(msg->seqno());
        ack->set_result(1);
        WaitSend(ack);
    }
}