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
m_lat(200), m_lon(0), m_posRecord(NULL), m_tmLast(-1)
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

void ObjectTracker::_respondLogin(int seq, int res)
{
    if(m_p && m_sock)
    {
        OnLogined(true);
        if (auto ack = new AckTrackerIdentityAuthentication)
        {
            ack->set_seqno(seq);
            ack->set_result(res);
            WaitSend(ack);
        }
    }
}

void ObjectTracker::OnLogined(bool suc, ISocket *s)
{
    if (m_bLogined != suc && suc)
    {
        if (auto ms = new ObjectSignal(this, ObjectGV::GVType(), ObjectSignal::S_Login))
            SendMsg(ms);
        if (auto ms = new ObjectSignal(this, ObjectGXClinet::GXClinetType(), ObjectSignal::S_Login))
            SendMsg(ms);
    }
    ObjectAbsPB::OnLogined(suc, s);
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
    if (!msg)
        return;

    if (msg->GetMessgeType() == IMessage::ControlDevice)
    {
        auto pb = ((TrackerMessage *)msg)->GetProtobuf();
        CopyAndSend(*pb);
    }  
}

void ObjectTracker::PrcsProtoBuff(uint64_t ms)
{
    if (!m_p)
        return;

    const string &name = m_p->GetMsgName();
    if (name == d_p_ClassName(RequestTrackerIdentityAuthentication))
        _respondLogin(((RequestTrackerIdentityAuthentication*)m_p->GetProtoMessage())->seqno(), 1);
    else if (name == d_p_ClassName(RequestPositionAuthentication))
        _prcsPosAuth((RequestPositionAuthentication *)m_p->GetProtoMessage());
    else if (name == d_p_ClassName(PostOperationInformation))
        _prcsOperationInformation((PostOperationInformation *)m_p->DeatachProto(), ms);
    else if (name == d_p_ClassName(AckQueryParameters))
        _prcsAckQueryParameters((AckQueryParameters *)m_p->DeatachProto());
    else if (name == d_p_ClassName(AckConfigurParameters))
        _prcsAckConfigurParameters((AckConfigurParameters *)m_p->DeatachProto());
    else if (name == d_p_ClassName(RequestProgramUpgrade))
        _prcsProgramUpgrade((RequestProgramUpgrade *)m_p->GetProtoMessage());
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

void ObjectTracker::CheckTimer(uint64_t ms)
{
    if (!m_sock && m_bLogined)
    {
        if (auto ms = new ObjectSignal(this, ObjectGV::GVType(), ObjectSignal::S_Logout))
            SendMsg(ms);
        if (auto ms = new ObjectSignal(this, ObjectGXClinet::GXClinetType(), ObjectSignal::S_Logout))
            SendMsg(ms);
    }
    ObjectAbsPB::CheckTimer(ms);
    if (m_sock)
    { 
        int64_t n = (int64_t)ms - m_tmLastInfo;
        if (n > 600000)
            Release();
        else if (n>10000)//超时关闭
            m_sock->Close();
    }
}

void ObjectTracker::OnConnected(bool bConnected)
{
    ObjectAbsPB::OnConnected(bConnected);
    if (m_sock && bConnected)
        m_sock->ResizeBuff(WRITE_BUFFLEN);
    
    if (!bConnected && m_posRecord)
    {
        fclose(m_posRecord);
        m_posRecord = NULL;
    }
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

void ObjectTracker::_prcsOperationInformation(PostOperationInformation *msg, uint64_t ms)
{
    if (!msg)
        return;

    _checkFile();
    if ((int64_t)ms - m_tmLast > 2000 && msg->oi_size()>0)
    {
        if (auto poi = new PostOperationInformation())
        {
            poi->set_seqno(msg->seqno());
            const OperationInformation &oi = msg->oi(0);    
            auto oiAdd = poi->add_oi();
            oiAdd->set_timestamp(ms);
            oiAdd->set_uavid(m_id);
            oiAdd->set_allocated_gps(new GpsInformation(oi.gps()));
            if (oi.has_status())
                oiAdd->set_allocated_status(new OperationStatus(oi.status()));

            if (m_posRecord)
            {
                fprintf(m_posRecord, "%s\t", Utility::bigint2string(ms).c_str());
                fprintf(m_posRecord, "%s\t", Utility::l2string(oiAdd->gps().latitude()).c_str());
                fprintf(m_posRecord, "%s\t", Utility::l2string(oiAdd->gps().longitude()).c_str());
                fprintf(m_posRecord, "%s\n", Utility::l2string(oiAdd->gps().altitude()).c_str());
            }

            if (auto *msGx = new Tracker2GXMessage(this))
            {
                msGx->AttachProto(poi);
                SendMsg(msGx);
            }
            else
            {
                delete poi;
            }
            m_tmLast = ms;
        }
    }

    if (Tracker2GVMessage *msGV = new Tracker2GVMessage(this, string()))
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
    if (auto ack = new Tracker2GVMessage(this, string()))
    {
        ack->AttachProto(msg);
        SendMsg(ack);
    }
}

void ObjectTracker::_prcsAckConfigurParameters(das::proto::AckConfigurParameters *msg)
{
    if (auto ack = new Tracker2GVMessage(this, string()))
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
