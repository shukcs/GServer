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
#include "IMessage.h"
#include "tinyxml.h"

#define PROTOFLAG "das.proto."

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
    {
        m_mtx->Lock();
        m_p->SetSended(sended);
        m_mtx->Unlock();
    }
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
    {
        m_mtx->Lock();
        m_p->SendProto(msg, m_sock);
        m_mtx->Unlock();
    }
}
////////////////////////////////////////////////////////////////////////////////
//UavManager
////////////////////////////////////////////////////////////////////////////////
UavManager::UavManager() : IObjectManager(0)
, m_sqlEng(new VGMySql), m_p(new ProtoMsg)
, m_lastId(1)
{
    _parseConfig();
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

uint32_t UavManager::toIntID(const std::string &uavid)
{
    if (0 == uavid.compare("VIGAU:"))
        return (uint32_t)Utility::str2int(uavid.substr(6 + 1));
    
    return 0;
}

int UavManager::GetObjectType() const
{
    return IObject::Plant;
}

IObject *UavManager::PrcsReceiveByMrg(ISocket *s, const char *buf, int &len)
{
    int pos = 0;
    int l = len;
    IObject *o = NULL;
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
    Message *proto = (Message *)msg.GetContent();
    switch (msg.GetMessgeType())
    {
    case BindUav:
        _checkBindUav(*(RequestBindUav *)proto);
        return true;
    case QueryUav:
        _checkUavInfo(*(RequestUavStatus *)proto, msg.GetSenderID());
        return true;
    case ControlUav:
        ObjectUav::AckControl2Uav(*(PostControl2Uav*)proto, 0);
        return true;
    case UavAllocation:
        processAllocationUav(((RequestIdentityAllocation *)proto)->seqno(), msg.GetSenderID());
        return true;
    default:
        break;
    }

    return false;
}

void UavManager::_parseConfig()
{
    TiXmlDocument doc;
    doc.LoadFile("UavInfo.xml");
    _parseMySql(doc);
    const TiXmlElement *rootElement = doc.RootElement();
    const TiXmlNode *node = rootElement ? rootElement->FirstChild("UavManager") : NULL;
    const TiXmlElement *cfg = node ? node->ToElement():NULL;
    if (cfg)
    {
        const char *tmp = cfg->Attribute("thread");
        int n = tmp ? (int)Utility::str2int(tmp) : 1;
        InitThread(n);
    }
}

void UavManager::_parseMySql(const TiXmlDocument &doc)
{
    StringList tables;
    string db = VGDBManager::Instance().Load(doc, tables);
    if (!m_sqlEng)
        return;

    m_sqlEng->ConnectMySql(VGDBManager::Instance().GetMysqlHost(db),
        VGDBManager::Instance().GetMysqlPort(db),
        VGDBManager::Instance().GetMysqlUser(db),
        VGDBManager::Instance().GetMysqlPswd(db));
    m_sqlEng->EnterDatabase(db);
    for (const string &tb : tables)
    {
        m_sqlEng->CreateTable(VGDBManager::Instance().GetTableByName(tb));
    }
    for (const string &trigger : VGDBManager::Instance().GetTriggers())
    {
        m_sqlEng->CreateTrigger(VGDBManager::Instance().GetTriggerByName(trigger));
    }
}

void UavManager::sendBindRes(const RequestBindUav &msg, int res, bool bind)
{
    if (GSMessage *ms = new GSMessage(this, msg.binder()))
    {
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
        SendMsg(ms);
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

    if (GSMessage *ms = new GSMessage(this, gs))
    {
        ms->SetGSContent(as);
        SendMsg(ms);
    }
}

void UavManager::processAllocationUav(int seqno, const string &id)
{
    if (id.empty() && !m_sqlEng)
        return;
    if (UAVMessage *ms = new UAVMessage(this, id))
    {
        int res = 0;

        ExecutItem *sql = VGDBManager::Instance().GetSqlByName("insertUavInfo");
        FiledValueItem *fd = sql->GetWriteItem("id");
        char idTmp[24] = {0};
        while (sql && fd && m_lastId>0)
        {
            sql->ClearData();
            sprintf(idTmp, "VIGAU:%08X", m_lastId);
            fd->SetParam(idTmp);
            ++m_lastId;
            if(m_sqlEng->Execut(sql))
                break;
        }

        AckIdentityAllocation *ack = new AckIdentityAllocation;
        ack->set_seqno(seqno);
        ack->set_result(res);
        ack->set_id(idTmp);
        ms->AttachProto(ack);
        SendMsg(ms);
    }
}

DECLARE_MANAGER_ITEM(UavManager)