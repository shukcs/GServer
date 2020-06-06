#include "ObjectGV.h"
#include "socketBase.h"
#include "das.pb.h"
#include "ProtoMsg.h"
#include "Messages.h"
#include "Utility.h"
#include "IMessage.h"
#include "GVManager.h"
#include "ObjectTracker.h"
#include "tinyxml.h"

enum {
    WRITE_BUFFLEN = 1024 * 8,
    MaxSend = 3 * 1024,
    MAXLANDRECORDS = 200,
    MAXPLANRECORDS = 200,
};

using namespace das::proto;
using namespace google::protobuf;
using namespace std;
#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif
////////////////////////////////////////////////////////////////////////////////
//ObjectGV
////////////////////////////////////////////////////////////////////////////////
ObjectGV::ObjectGV(const std::string &id): ObjectAbsPB(id)
, m_auth(1)
{
    SetBuffSize(WRITE_BUFFLEN);
}

ObjectGV::~ObjectGV()
{
}

void ObjectGV::OnConnected(bool bConnected)
{
    ObjectAbsPB::OnConnected(bConnected);
    if (bConnected && m_sock)
        m_sock->ResizeBuff(WRITE_BUFFLEN);
}

void ObjectGV::SetPswd(const std::string &pswd)
{
    m_pswd = pswd;
}

const std::string &ObjectGV::GetPswd() const
{
    return m_pswd;
}

void ObjectGV::SetAuth(int a)
{
    m_auth = a;
}

int ObjectGV::Authorize() const
{
    return m_auth;
}

void ObjectGV::SetSeq(int seq)
{
    m_seq = seq;
}

int ObjectGV::GVType()
{
    return IObject::User;
}

ObjectGV *ObjectGV::ParseObjecy(const TiXmlElement &e)
{
    const char *id = e.Attribute("id");
    const char *pswd = e.Attribute("pswd");
    if (id && pswd)
    {
        auto obj = new ObjectGV(id);
        if (!obj)
            return NULL;
        obj->SetPswd(pswd);
        const char *auth = e.Attribute("auth");
        obj->m_auth = auth ? int(Utility::str2int(auth)) : 3;
        obj->InitObject();
        return obj;
    }
    return NULL;
}

int ObjectGV::GetObjectType() const
{
    return GVType();
}

void ObjectGV::_prcsLogin(RequestGVIdentityAuthentication *msg)
{
    if (msg && m_p)
    {
        bool bSuc = m_pswd == msg->password();
        OnLogined(bSuc);
        if (auto ack = new AckGVIdentityAuthentication)
        {
            ack->set_seqno(msg->seqno());
            ack->set_result(bSuc ? 1 : -1);
            WaitSend(ack);
        }
    }
}

void ObjectGV::_prcsHeartBeat(PostHeartBeat *msg)
{
    if (auto ahb = new AckHeartBeat)
    {
        ahb->set_seqno(msg->seqno());
        WaitSend(ahb);
    }
}

void ObjectGV::_prcsSyncDevice(SyncDeviceList *ms)
{
    if (auto msg = new GV2TrackerMessage(this, string()))
    {
        msg->AttachProto(ms);
        SendMsg(msg);
    }
}

void ObjectGV::_prcsQueryParameters(das::proto::QueryParameters *pb)
{
    if (auto msg = new GV2TrackerMessage(this, pb->id()))
    {
        msg->AttachProto(pb);
        SendMsg(msg);
    }
}

void ObjectGV::_prcsConfigureParameters(das::proto::ConfigureParameters *pb)
{
    if (auto msg = new GV2TrackerMessage(this, pb->id()))
    {
        msg->AttachProto(pb);
        SendMsg(msg);
    }
}

void ObjectGV::ProcessMessage(IMessage *msg)
{
    int tp = msg ? msg->GetMessgeType() : IMessage::Unknown;

    switch (tp)
    {
    case IMessage::ControlUser:
        CopyAndSend(*((Tracker2GVMessage*)msg)->GetProtobuf());
        break;
    case IMessage::PushUavSndInfo:
        process2GsMsg(((TrackerMessage *)msg)->GetProtobuf());
        break;
    case IMessage::SyncDeviceisRslt:
        process2GsMsg(((TrackerMessage *)msg)->GetProtobuf());
        break;
    case ObjectSignal::S_Login:
    case ObjectSignal::S_Logout:
        processEvent(*msg, tp);
        break;
    default:
        break;
    }
}

void ObjectGV::PrcsProtoBuff(uint64_t)
{
    if (!m_p)
        return;

    string strMsg = m_p->GetMsgName();
    if (strMsg == d_p_ClassName(RequestGVIdentityAuthentication))
        _prcsLogin((RequestGVIdentityAuthentication*)m_p->GetProtoMessage());
    else if (strMsg == d_p_ClassName(PostHeartBeat))
        _prcsHeartBeat((PostHeartBeat *)m_p->GetProtoMessage());
    else if (strMsg == d_p_ClassName(SyncDeviceList))
        _prcsSyncDevice((SyncDeviceList *)m_p->DeatachProto());
    else if (strMsg == d_p_ClassName(QueryParameters))
        _prcsQueryParameters((QueryParameters *)m_p->DeatachProto());
    else if (strMsg == d_p_ClassName(ConfigureParameters))
        _prcsConfigureParameters((ConfigureParameters *)m_p->DeatachProto());
}

void ObjectGV::process2GsMsg(const google::protobuf::Message *msg)
{
    if (!msg)
        return;

    if (Message *ms = msg->New())
    {
        ms->CopyFrom(*msg);
        WaitSend(ms);
    }
}

void ObjectGV::processEvent(const IMessage &msg, int tp)
{
    if (   !m_bLogined
        || msg.GetSenderType() != ObjectTracker::TrackerType()
        || (ObjectSignal::S_Login!=tp && ObjectSignal::S_Logout!=tp) )
        return;

    if (auto *snd = new UpdateDeviceList)
    {
        snd->set_seqno(m_seq++);
        snd->set_operation(ObjectSignal::S_Login == tp ? 1 : 0);
        snd->add_id(msg.GetSenderID());
        WaitSend(snd);
    }
}

void ObjectGV::InitObject()
{
    if (IObject::Uninitial == m_stInit)
        m_stInit = IObject::Initialed;
}

void ObjectGV::CheckTimer(uint64_t ms)
{
    ObjectAbsPB::CheckTimer(ms);
    ms -= m_tmLastInfo;
    if (ms > 600000)
        Release();
    else if (m_sock && ms > 30000)//³¬Ê±¹Ø±Õ
        m_sock->Close();
}

bool ObjectGV::IsAllowRelease() const
{
    return false;
}
