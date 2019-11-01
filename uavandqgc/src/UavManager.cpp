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
#include "ObjectUav.h"
#include "ObjectGS.h"

#define PROTOFLAG "das.proto."

using namespace std;
using namespace das::proto;
using namespace ::google::protobuf;
////////////////////////////////////////////////////////////////////////////////
//UavManager
////////////////////////////////////////////////////////////////////////////////
UavManager::UavManager() : IObjectManager(0)
, m_sqlEng(new VGMySql), m_p(new ProtoMsg)
, m_lastId(1), m_bInit(false)
{
}

UavManager::~UavManager()
{
    delete m_sqlEng;
    delete m_p;
}

int UavManager::PrcsBind(const RequestBindUav *msg, const string &gsOld, bool binded, ObjectGS *sender)
{
    int res = -1;
    string binder = msg ? msg->binder() : string();
    if (binder.length() == 0)
        return res;

    bool bForceUnbind = sender && sender->GetAuth(ObjectGS::Type_UavManager);
    int op = msg->opid();
    if (1 == op && !bForceUnbind)
        res = (!binded || gsOld.length() == 0 || binder == gsOld) ? 1 : -3;
    else if (0 == op)
        res = (bForceUnbind || binder == gsOld || gsOld.empty()) ? 1 : -3;

    const std::string &uav = msg->uavid();
    if(res == 1)
    {
        if (ExecutItem *item = VGDBManager::GetSqlByName("updateBinded"))
        {
            item->ClearData();
            if (FiledVal *fd = item->GetConditionItem("id"))
                fd->SetParam(uav);

            if (FiledVal *fd = item->GetWriteItem("binded"))
                fd->InitOf<char>(1 == op);
            if (FiledVal *fd = item->GetWriteItem("binder"))
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
    if (ExecutItem *item = VGDBManager::GetSqlByName("updatePos"))
    {
        item->ClearData();
        if (FiledVal *fd = item->GetConditionItem("id"))
            fd->SetParam(uav);

        if (FiledVal *fd = item->GetWriteItem("lat"))
            fd->InitOf(lat);
        if (FiledVal *fd = item->GetWriteItem("lon"))
            fd->InitOf(lon);

        m_sqlEng->Execut(item);
    }
}

VGMySql *UavManager::GetMySql()const
{
    return m_sqlEng;
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

IObject *UavManager::PrcsReceiveByMgr(ISocket *s, const char *buf, int &len)
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
        _checkBindUav(*(RequestBindUav *)proto, (ObjectGS*)msg.GetSender());
        return true;
    case QueryUav:
        _checkUavInfo(*(RequestUavStatus *)proto, (ObjectGS*)msg.GetSender());
        return true;
    case ControlUav:
        ObjectUav::AckControl2Uav(*(PostControl2Uav*)proto, -1);
        return true;
    case UavAllocation:
        processAllocationUav(((RequestIdentityAllocation *)proto)->seqno(), msg.GetSenderID());
        return true;
    default:
        break;
    }

    return false;
}

void UavManager::LoadConfig()
{
    if (m_bInit)
        return;

    TiXmlDocument doc;
    doc.LoadFile("UavInfo.xml");
    _parseMySql(doc);
    const TiXmlElement *rootElement = doc.RootElement();
    const TiXmlNode *node = rootElement ? rootElement->FirstChild("UavManager") : NULL;
    const TiXmlElement *cfg = node ? node->ToElement() : NULL;
    if (cfg)
    {
        const char *tmp = cfg->Attribute("thread");
        int n = tmp ? (int)Utility::str2int(tmp) : 1;
        InitThread(n);
    }
    m_bInit = true;
}

void UavManager::_parseMySql(const TiXmlDocument &doc)
{
    StringList tables;
    string db = VGDBManager::Load(doc, tables);
    if (!m_sqlEng)
        return;

    m_sqlEng->ConnectMySql(VGDBManager::GetMysqlHost(db),
        VGDBManager::GetMysqlPort(db),
        VGDBManager::GetMysqlUser(db),
        VGDBManager::GetMysqlPswd(db));
    m_sqlEng->EnterDatabase(db);
    for (const string &tb : tables)
    {
        m_sqlEng->CreateTable(VGDBManager::GetTableByName(tb));
    }
    for (const string &trigger : VGDBManager::GetTriggers())
    {
        m_sqlEng->CreateTrigger(VGDBManager::GetTriggerByName(trigger));
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
    else if (ExecutItem *item = VGDBManager::GetSqlByName("queryUavInfo"))
    {
        item->ClearData();
        if (FiledVal *fd = item->GetConditionItem("id"))
            fd->SetParam(uavid);

        if (m_sqlEng->Execut(item))
        {
            ret = new ObjectUav(uia.uavid());
            ret->InitBySqlResult(*item);
            while (m_sqlEng->GetResult());
            res = 1;
        }
    }

    if (ISocketManager *m = s->GetManager())
        m->Log(0, uia.uavid(), 0, "[%s]%s", s->GetHost().c_str(), res==1 ? "login success" : "login fail");

    AckUavIdentityAuthentication ack;
    ack.set_seqno(uia.seqno());
    ack.set_result(res);
    ProtoMsg::SendProtoTo(ack, s);

    return ret;
}

void UavManager::_checkBindUav(const RequestBindUav &rbu, ObjectGS *gs)
{
    if (ExecutItem *item = VGDBManager::GetSqlByName("queryUavInfo"))
    {
        item->ClearData();
        if (FiledVal *fd = item->GetConditionItem("id"))
            fd->SetParam(rbu.uavid());

        if (!m_sqlEng->Execut(item))
            return;
        bool bind = false;
        if (FiledVal *fd = item->GetReadItem("binded"))
            bind = fd->GetValue<char>() != 0;
        if (FiledVal *fd = item->GetReadItem("binder"))
            PrcsBind(&rbu, string((char*)fd->GetBuff(), fd->GetValidLen()), bind, gs);

        while (m_sqlEng->GetResult());
    }
}

void UavManager::_checkUavInfo(const RequestUavStatus &uia, ObjectGS *gs)
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
        else if (!_queryUavInfo(as, uav) && gs && gs->GetAuth(ObjectGS::Type_UavManager))
        {
            if (_addUavId(uav) > 0)
            {
                UavStatus *us = as.add_status();
                ObjectUav oU(uav);
                oU.transUavStatus(*us);
            }
        }
    }

    if (gs)
    {
        GSMessage *ms = new GSMessage(this, gs->GetObjectID());
        ms->SetPBContentPB(as);
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
        char idTmp[24] = {0};
        while (m_lastId>0)
        {
            sprintf(idTmp, "VIGAU:%08X", m_lastId);
            if(_addUavId(idTmp) != 0)
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

int UavManager::_addUavId(const std::string &uav)
{
    StringList strLs = Utility::SplitString(uav, ":");
    if ( strLs.size() != 2
      || strLs.front() != "VIGAU"
      || strLs.back().length()!=8
      || Utility::str2int(strLs.back(), 16)==0 )
        return -1;

    ExecutItem *sql = VGDBManager::GetSqlByName("insertUavInfo");
    FiledVal *fd = sql->GetWriteItem("id");
    if (sql && fd)
    {
        sql->ClearData();
        fd->SetParam(uav);
        return m_sqlEng->Execut(sql) ? 1 : 0;
    }
    return -1;
}

bool UavManager::_queryUavInfo(das::proto::AckRequestUavStatus &aus, const string &uav)
{
    if (ExecutItem *item = VGDBManager::GetSqlByName("queryUavInfo"))
    {
        item->ClearData();
        if (FiledVal *fd = item->GetConditionItem("id"))
            fd->SetParam(uav);

        if (!m_sqlEng->Execut(item))
            return false;

        UavStatus *us = aus.add_status();
        ObjectUav oU(uav);
        oU.InitBySqlResult(*item);
        oU.transUavStatus(*us);
        while (m_sqlEng->GetResult());
        return true;
    }
    return false;
}

DECLARE_MANAGER_ITEM(UavManager)