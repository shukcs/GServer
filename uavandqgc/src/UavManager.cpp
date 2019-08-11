#include "UavManager.h"
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

#define PROTOFLAG "das.proto."

using namespace std;
using namespace das::proto;

class GSMessage : public IMessage
{
public:
    GSMessage(IObject *sender, const std::string &idRcv)
        :IMessage(sender, idRcv, IObject::GroundStation, Unknown), m_msg(NULL)
    {
    }
    ~GSMessage()
    {
        delete m_msg;
    }
    void *GetContent() const
    {
        return m_msg;
    }
    void SetGSContent(const google::protobuf::Message &msg)
    {
        delete m_msg;
        m_msg = NULL;
        m_tpMsg = Unknown;
        const string &name = msg.GetTypeName();
        if (name == d_p_ClassName(AckRequestBindUav))
        {
            m_msg = new AckRequestBindUav;
            m_tpMsg = BindUavRes;
        }
        else if (name == d_p_ClassName(AckPostControl2Uav))
        {
            m_msg = new AckPostControl2Uav;
            m_tpMsg = ControlUavRes;
        }
        else if (name == d_p_ClassName(AckRequestUploadOperationRoutes))
        {
            m_msg = new AckRequestUploadOperationRoutes;
            m_tpMsg = SychMissionRes;
        }
        else if (name == d_p_ClassName(OperationInformation))
        {
            m_msg = new OperationInformation;
            m_tpMsg = PushUavSndInfo;
        }
        else if (name == d_p_ClassName(PostStatus2GroundStation))
        {
            m_msg = new OperationInformation;
            m_tpMsg = ControlGs;
        }
        else if (name == d_p_ClassName(AckRequestUavStatus))
        {
            m_msg = new AckRequestUavStatus;
            m_tpMsg = QueryUavRes;
        }

        if (m_tpMsg != Unknown)
            m_msg->CopyFrom(msg);
    }
    void AttachProto(google::protobuf::Message *msg)
    {
        delete m_msg;
        m_msg = NULL;
        if (!msg)
            return;
        const string &name = msg->GetTypeName();
        if (name == d_p_ClassName(AckRequestBindUav))
            m_tpMsg = BindUavRes;
        else if (name == d_p_ClassName(AckPostControl2Uav))
            m_tpMsg = ControlUavRes;
        else if (name == d_p_ClassName(AckRequestUploadOperationRoutes))
            m_tpMsg = SychMissionRes;
        else if (name == d_p_ClassName(OperationInformation))
            m_tpMsg = PushUavSndInfo;
        else if (name == d_p_ClassName(PostStatus2GroundStation))
            m_tpMsg = ControlGs;
        else if (name == d_p_ClassName(AckRequestUavStatus))
            m_tpMsg = QueryUavRes;
        else
            m_tpMsg = Unknown;

        m_msg = msg;
    }
    int GetContentLength() const
    {
        return m_msg ? m_msg->ByteSize() : 0;
    }
private:
    google::protobuf::Message  *m_msg;
};
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
        ((UavManager *)GetManager())->UpdatePos(GetObjectID(), m_lat, m_lon);
    else
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

void ObjectUav::ProcessMassage(const IMessage &msg)
{
    if (msg.GetMessgeType() == BindUav)
        processBind((RequestBindUav*)msg.GetContent());
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
            prcsRcvPostOperationInfo((PostOperationInformation *)m_p->GetProtoMessage());
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
    {
        m_mtx->Lock();
        m_p->SetSended(sended);
        m_mtx->Unlock();
    }
}

void ObjectUav::prcsRcvPostOperationInfo(das::proto::PostOperationInformation *msg)
{
    if (!msg)
        return;

    m_tmLastInfo = Utility::msTimeTick();
    int nCount = msg->oi_size();
    for (int i = 0; i < nCount; i++)
    {
        OperationInformation *oi = msg->mutable_oi(i);
        if (oi->has_gps())
        {
            const GpsInformation &gps = oi->gps();
            m_lat = gps.latitude() / 1e7;
            m_lon = gps.longitude() / 1e7;
        }
        if (m_bBind && m_lastBinder.length() > 0)
        {
            GSMessage *ms = new GSMessage(this, m_lastBinder);
            if (!ms)
                continue;
            ms->SetGSContent(*oi);
            ObjectManagers::Instance().SendMsg(ms);
        }
    }
   
    AckOperationInformation ack;
    ack.set_seqno(msg->seqno());
    ack.set_result(1);
    _send(ack);
}

void ObjectUav::prcsRcvPost2Gs(das::proto::PostStatus2GroundStation *msg)
{
    if (!msg || !m_bBind || m_lastBinder.length() < 1)
        return;

    if (GSMessage *ms = new GSMessage(this, m_lastBinder))
    {
        ms->SetGSContent(*msg);
        ObjectManagers::Instance().SendMsg(ms);
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

void ObjectUav::_send(const google::protobuf::Message &msg)
{
    if (m_p && m_sock)
    {
        m_mtx->Lock();
        m_p->SendProto(msg, m_sock);
        m_mtx->Unlock();
    }
}
////////////////////////////////////////////////////////////////////////////////
//UavManager
////////////////////////////////////////////////////////////////////////////////
UavManager::UavManager():m_sqlEng(new VGMySql)
, m_p(new ProtoMsg)
{
}

UavManager::~UavManager()
{
    delete m_sqlEng;
    delete m_p;
}

int UavManager::PrcsBind(const RequestBindUav *msg, const std::string &gsOld)
{
    int res = -1;
    string binder = msg ? msg->binder() : string();
    if (binder.length() == 0)
        return res;

    int op = msg->opid();
    if (1 == op)
        res = (gsOld.length() == 0 || binder == gsOld) ? 1 : -3;
    else if (0 == op)
        res = (binder == gsOld) ? 1 : -3;

    const std::string &uav = msg->uavid();
    if(res == 1)
    {
        if (ExecutItem *item = VGDBManager::Instance().GetSqlByName("updateBinded"))
        {
            item->ClearData();
            if (FiledValueItem *fd = item->GetConditionItem("id"))
                fd->SetParam(uav);

            if (FiledValueItem *fd = item->GetWriteItem("binded"))
                fd->InitOf<char>(1 == op);
            if (FiledValueItem *fd = item->GetWriteItem("binder"))
                fd->SetParam(binder);
            m_sqlEng->Execut(item);
            printf("%s %s %s\n", binder.c_str(), 1 == op ? "bind" : "unbind", uav.c_str());
        }
    }
    sendBindRes(*msg, res, 1 == op);
    return res;
}

void UavManager::UpdatePos(const std::string &uav, double lat, double lon)
{
    if (ExecutItem *item = VGDBManager::Instance().GetSqlByName("updatePos"))
    {
        item->ClearData();
        if (FiledValueItem *fd = item->GetConditionItem("id"))
            fd->SetParam(uav);

        if (FiledValueItem *fd = item->GetWriteItem("lat"))
            fd->InitOf(lat);
        if (FiledValueItem *fd = item->GetWriteItem("lon"))
            fd->InitOf(lon);

        m_sqlEng->Execut(item);
    }
}

int UavManager::GetObjectType() const
{
    return IObject::Plant;
}

IObject *UavManager::ProcessReceive(ISocket *s, const char *buf, int &len)
{
    int pos = 0;
    int l = len;
    IObject *o = NULL;
    _ensureDBValid();
    while (m_p->Parse(buf+pos, l))
    {
        pos += l;
        l = len - pos;
        if (m_p->GetMsgName() == d_p_ClassName(RequestUavIdentityAuthentication))
        {
            RequestUavIdentityAuthentication *rua = (RequestUavIdentityAuthentication *)m_p->GetProtoMessage();
            o = _checkLogin(s, *rua);
            len = pos;

            break;
        }
    }

    return o;
}

bool UavManager::PrcsRemainMsg(const IMessage &msg)
{
    if (BindUav == msg.GetMessgeType())
    {
        RequestBindUav *rb = ((RequestBindUav *)msg.GetContent());
        _checkBindUav(*rb);
        return true;
    }
    if (QueryUav == msg.GetMessgeType())
    {
        RequestUavStatus *rus = ((RequestUavStatus *)msg.GetContent());
        _checkUavInfo(*rus, msg.GetSenderID());
        return true;
    }
    return false;
}

void UavManager::_ensureDBValid()
{
    if (m_sqlEng && !m_sqlEng->IsValid())
    {
        m_sqlEng->ConnectMySql(VGDBManager::Instance().GetMysqlHost(),
            VGDBManager::Instance().GetMysqlPort(),
            VGDBManager::Instance().GetMysqlUser(),
            VGDBManager::Instance().GetMysqlPswd(),
            VGDBManager::Instance().GetDatabase());
    }
}

void UavManager::sendBindRes(const das::proto::RequestBindUav &msg, int res, bool bind)
{
    if (GSMessage *ms = new GSMessage(NULL, msg.binder()))
    {
        ms->SetSenderType(IObject::Plant);
        AckRequestBindUav *proto = new AckRequestBindUav();
        if (!proto)
            return;

        UavStatus *s = new UavStatus;
        s->set_result(proto->result());
        s->set_uavid(msg.uavid().c_str());
        s->set_binder(msg.binder());
        s->set_binded(bind);
        if (res == 1)
        {
            if (bind)
                s->set_bindtime(Utility::secTimeCount());
            else
                s->set_unbindtime(Utility::secTimeCount());
        }
        proto->set_seqno(msg.seqno());
        proto->set_opid(msg.opid());
        proto->set_result(res);
        proto->set_allocated_status(s);
        ms->AttachProto(proto);
        ObjectManagers::Instance().SendMsg(ms);
    }
}

IObject *UavManager::_checkLogin(ISocket *s, const RequestUavIdentityAuthentication &uia)
{
    string uavid = uia.uavid();
    ObjectUav *ret = (ObjectUav *)GetObjectByID(uavid);
    int res = -1;
    if (ret)
    {
        if (!ret->IsConnect())
        {
            ret->SetSocket(s);
            res = 1;
        }
    }
    else if (ExecutItem *item = VGDBManager::Instance().GetSqlByName("queryUavInfo"))
    {
        item->ClearData();
        if (FiledValueItem *fd = item->GetConditionItem("id"))
            fd->SetParam(uavid);

        if (m_sqlEng->Execut(item))
        {
            ret = new ObjectUav(uia.uavid());
            AddObject(ret);
            ret->InitBySqlResult(*item);
            while (m_sqlEng->GetResult());
            res = 1;
        }
    }

    AckUavIdentityAuthentication ack;
    ack.set_seqno(uia.seqno());
    ack.set_result(res);
    ProtoMsg::SendProtoTo(ack, s);

    return ret;
}

void UavManager::_checkBindUav(const RequestBindUav &rbu)
{
    if (ExecutItem *item = VGDBManager::Instance().GetSqlByName("queryUavInfo"))
    {
        item->ClearData();
        if (FiledValueItem *fd = item->GetConditionItem("id"))
            fd->SetParam(rbu.uavid());

        if (!m_sqlEng->Execut(item))
            return;

        if (FiledValueItem *fd = item->GetReadItem("binder"))
        {
            string binder((char*)fd->GetBuff(), fd->GetValidLen());
            PrcsBind(&rbu, binder);
        }
        while (m_sqlEng->GetResult());
    }
}

void UavManager::_checkUavInfo(const RequestUavStatus &uia, const std::string &gs)
{
    AckRequestUavStatus as;
    as.set_seqno(uia.seqno());

    for (int i = 0; i < uia.uavid_size(); ++i)
    {
        const string &uav = uia.uavid(i);
        ObjectUav *o = (ObjectUav *)GetObjectByID(uav);
        if (o)
        {
            UavStatus *us = as.add_status();
            o->transUavStatus(*us);
            us = NULL;
        }
        else if (ExecutItem *item = VGDBManager::Instance().GetSqlByName("queryUavInfo"))
        {
            item->ClearData();
            if (FiledValueItem *fd = item->GetConditionItem("id"))
                fd->SetParam(uav);

            if (!m_sqlEng->Execut(item))
                continue;

            UavStatus *us = as.add_status();
            ObjectUav oU(uav);
            oU.InitBySqlResult(*item);
            oU.transUavStatus(*us);
            while (m_sqlEng->GetResult());
        }
    }

    if (GSMessage *ms = new GSMessage(NULL, gs))
    {
        ms->SetSenderType(IObject::Plant);
        ms->SetGSContent(as);
        ObjectManagers::Instance().SendMsg(ms);
    }
}

DECLARE_MANAGER_ITEM(UavManager)