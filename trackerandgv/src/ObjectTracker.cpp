#include "ObjectTracker.h"
#include "ObjectManagers.h"
#include "das.pb.h"
#include "ProtoMsg.h"
#include "Messages.h"
#include "common/Utility.h"
#include "net/socketBase.h"
#include "TrackerManager.h"
#include "ObjectGV.h"
#include "GXClient.h"

#define WRITE_BUFFLEN 2048
using namespace std;
using namespace das::proto;
using namespace ::google::protobuf;
#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif

static uint64_t sValidTm = Utility::timeStamp(2015);
///////////////////////////////////////////////////////////////////////////////////////////////////////////
//ObjectTracker
///////////////////////////////////////////////////////////////////////////////////////////////////////////
ObjectTracker::ObjectTracker(const string &id, const string &sim) : ObjectAbsPB(id), m_strSim(sim),
m_lat(200), m_lon(0), m_posRecord(NULL), m_tmPos(-1), m_statGX(0)
{
    SetBuffSize(1024 * 2);
    string name = id;
    Utility::ReplacePart(name, ':', '=');
    if (!id.empty())
        m_strFile = Utility::ModuleDirectory()+ "/" + name;
}

ObjectTracker::~ObjectTracker()
{
}

int ObjectTracker::GetObjectType() const
{
    return TrackerType();
}

void ObjectTracker::_respondLogin(const RequestTrackerIdentityAuthentication &ra)
{
    if(IsLinked())
    {
        SetLogined(true);
        auto id = Utility::Upper(ra.trackerid());
        if (auto ack = new AckTrackerIdentityAuthentication)
        {
            ack->set_seqno(ra.seqno());
            ack->set_result(id==GetObjectID()?1:0);
            WaitSend(ack);
        }
    }
}

void ObjectTracker::_respond3rdLogin(const Request3rdIdentityAuthentication &ra)
{
    if (IsConnect())
    {
        SetLogined(true);
        auto uav = Utility::Upper(ra.identification());
        if (auto ack = new Ack3rdIdentityAuthentication)
        {
            ack->set_seqno(ra.seqno());
            ack->set_result(uav==GetObjectID() ? 1 : 0);
            WaitSend(ack);
        }
    }
}

void ObjectTracker::SetLogined(bool suc, ISocket *s)
{
    if (IsConnect() != suc && suc)
    {
        if (auto ms = new ObjectSignal(this, ObjectGV::GVType(), ObjectSignal::S_Login))
            SendMsg(ms);
        if (auto ms = new ObjectSignal(this, GXClient::GXClientType(), ObjectSignal::S_Login))
            SendMsg(ms);
    }
    ObjectAbsPB::SetLogined(suc, s);
}

bool ObjectTracker::IsAllowRelease() const
{
    return m_id != "VIGAT:00000000";
}

ILink *ObjectTracker::GetLink()
{
    if (m_id != "VIGAT:00000000")
        return ObjectAbsPB::GetLink();

    return NULL;
}

void ObjectTracker::RefreshRcv(int64_t ms)
{
    m_tmPos = ms;
}

void ObjectTracker::SetSimId(const std::string &sim)
{
    m_strSim = sim;
}

int ObjectTracker::TrackerType()
{
    return IObject::Tracker;
}

void ObjectTracker::ProcessMessage(const IMessage *msg)
{
    if (!msg)
        return;

    if (msg->GetMessgeType() == IMessage::ControlDevice)
    {
        auto pb = ((TrackerMessage *)msg)->GetProtobuf();
        CopyAndSend(*pb);
        if (pb->GetTypeName() == d_p_ClassName(QueryParameters))
            _ackPartParameters(msg->GetSenderID(), *(QueryParameters*)pb);
    }
    else if (msg->GetMessgeType() == IMessage::GXClinetStat)
    {
        m_statGX = ((GX2TrackerMessage *)msg)->GetStat();
    }
}

bool ObjectTracker::ProcessRcvPack(const void *pack)
{
    auto ptoto = (const Message*)pack;
    const string &name = ptoto->GetDescriptor()->full_name();
    if (name == d_p_ClassName(RequestTrackerIdentityAuthentication))
        _respondLogin(*(RequestTrackerIdentityAuthentication*)ptoto);
    if (name == d_p_ClassName(Request3rdIdentityAuthentication))
        _respond3rdLogin(*(Request3rdIdentityAuthentication*)ptoto);
    else if (name == d_p_ClassName(RequestPositionAuthentication))
        _prcsPosAuth((RequestPositionAuthentication *)ptoto);
    else if (name == d_p_ClassName(PostOperationInformation))
        _prcsOperationInformation((PostOperationInformation *)ptoto);
    else if (name == d_p_ClassName(AckQueryParameters))
        _prcsAckQueryParameters((AckQueryParameters *)ptoto);
    else if (name == d_p_ClassName(AckConfigurParameters))
        _prcsAckConfigurParameters((AckConfigurParameters *)ptoto);
    else if (name == d_p_ClassName(RequestProgramUpgrade))
        _prcsProgramUpgrade((RequestProgramUpgrade *)ptoto);
    else if (name == d_p_ClassName(PostHeartBeat))
        _prcsHeartBeat(*(PostHeartBeat *)ptoto);

    return IsInitaled();
}

int ObjectTracker::_checkPos(double lat, double lon, double alt)
{
    if (TrackerManager *m = (TrackerManager *)GetManager())
        return m->CanFlight(lat, lon, alt) ? 1 : 0;

    return 0;
}

void ObjectTracker::_checkFile()
{
    if (!m_posRecord && !m_strFile.empty())
    {
        m_posRecord = fopen(m_strFile.c_str(), "rb+");
        if (m_posRecord)//已经存在移动到末尾
        {
            fseek(m_posRecord, 0, SEEK_END);
        }
        else //创建
        {
            m_posRecord = fopen(m_strFile.c_str(), "wb+");
            fprintf(m_posRecord, "%s\n", m_id.c_str());
        }
    }
}

void ObjectTracker::_ackPartParameters(const std::string &gv, const das::proto::QueryParameters &qp)
{
    auto acp = new AckQueryParameters;
    if (!acp)
        return;

    acp->set_seqno(qp.seqno());
    acp->set_id(m_id);
    acp->set_result(1);
    if (auto ms = new Tracker2GVMessage(*this, gv))
    {
        if (auto pd = acp->add_pd())
        {
            pd->set_name("GX_Clinet_Stat");
            pd->set_readonly(true);
            pd->set_type(1);
            pd->set_value(Utility::l2string(m_statGX));
        }
        ms->AttachProto(acp);
        SendMsg(ms);
    }
    else
    {
        delete acp;
    }
}

void ObjectTracker::CheckTimer(uint64_t ms)
{
    if (IsConnect())
    {
        if (auto mss = new ObjectSignal(this, ObjectGV::GVType(), ObjectSignal::S_Logout))
            SendMsg(mss);
        if (auto mss = new ObjectSignal(this, GXClient::GXClientType(), ObjectSignal::S_Logout))
            SendMsg(mss);
    }
    ObjectAbsPB::CheckTimer(ms);
    ms -= m_tmPos;
    if (ms > 600000)
        ReleaseObject();
    else if (ms>10000)//超时关闭
        CloseLink();
}

void ObjectTracker::OnConnected(bool bConnected)
{
    if (bConnected)
        SetSocketBuffSize(WRITE_BUFFLEN);
    
    if (!bConnected && m_posRecord)
    {
        fclose(m_posRecord);
        m_posRecord = NULL;
    }
}

void ObjectTracker::InitObject()
{
    if (!IsInitaled())
    {
        IObject::InitObject();
        if (auto ack = new AckTrackerIdentityAuthentication)
        {
            ack->set_seqno(1);
            ack->set_result(1);
            WaitSend(ack);
        }
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
        GetManager()->Log(0, IObjectManager::GetObjectFlagID(this), 0, "Arm!");
        WaitSend(ack);
    }
}

void ObjectTracker::_prcsOperationInformation(PostOperationInformation *msg)
{
    if (!msg)
        return;

    auto ms = Utility::msTimeTick();
    for (int i = 0; (int64_t)ms - m_tmPos > 2000 && i < msg->oi_size(); ++i)
    {
        auto oi = msg->mutable_oi(i);
        _checkFile();
        if (!oi)
            continue;
        if (oi->timestamp() < sValidTm)
            oi->set_timestamp(ms);
        if (m_posRecord)
        {
            fprintf(m_posRecord, "%s\t", oi->uavid().c_str());
            fprintf(m_posRecord, "%s\t", Utility::bigint2string(ms).c_str());
            fprintf(m_posRecord, "%s\t", Utility::l2string(oi->gps().latitude()).c_str());
            fprintf(m_posRecord, "%s\t", Utility::l2string(oi->gps().longitude()).c_str());
            fprintf(m_posRecord, "%s\n", Utility::l2string(oi->gps().altitude()).c_str());
        }     
        m_tmPos = ms;
    }
    if (GXClient::St_Authed == m_statGX)
    {
        auto poi = new PostOperationInformation(*msg);
        if (auto *msGx = new Tracker2GXMessage(*this))
        {
            msGx->AttachProto(poi);
            SendMsg(msGx);
        }
        else
        {
            delete poi;
        }
    }

    if (Tracker2GVMessage *msGV = new Tracker2GVMessage(*this, string()))
    {
        msGV->AttachProto(msg);
        SendMsg(msGV);
    }
    else
    {
        delete msg;
    }

    if (auto ack = new AckOperationInformation)
    {
        ack->set_seqno(msg->seqno());
        ack->set_result(1);
        WaitSend(ack);
    }
}

void ObjectTracker::_prcsAckQueryParameters(das::proto::AckQueryParameters *msg)
{
    if (auto ack = new Tracker2GVMessage(*this, string()))
    {
        if (auto pd = msg->add_pd())
        {
            pd->set_name("GX_Clinet_Stat");
            pd->set_readonly(true);
            pd->set_type(1);
            pd->set_value(Utility::l2string(m_statGX));
        }
        ack->AttachProto(msg);
        SendMsg(ack);
    }
    else
    {
        delete msg;
    }
}

void ObjectTracker::_prcsAckConfigurParameters(das::proto::AckConfigurParameters *msg)
{
    if (auto ack = new Tracker2GVMessage(*this, string()))
    {
        ack->AttachProto(msg);
        SendMsg(ack);
    }
}

void ObjectTracker::_prcsProgramUpgrade(das::proto::RequestProgramUpgrade *msg)
{
    if (!msg)
        return;

    if (auto ack = new AckProgramUpgrade)
    {
        ack->set_seqno(msg->seqno());
        ack->set_result(1);
        ack->set_software(msg->software());
        ack->set_length(0);
        ack->set_forced(false);
        WaitSend(ack);
    }
}

void ObjectTracker::_prcsHeartBeat(const PostHeartBeat &msg)
{
    if (auto ahb = new AckHeartBeat)
    {
        ahb->set_seqno(msg.seqno());
        WaitSend(ahb);
    }
}
