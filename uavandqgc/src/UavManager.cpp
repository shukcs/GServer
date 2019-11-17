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
#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif
////////////////////////////////////////////////////////////////////////////////
//UavManager
////////////////////////////////////////////////////////////////////////////////
UavManager::UavManager() : IObjectManager(0)
, m_sqlEng(new VGMySql), m_p(new ProtoMsg)
, m_lastId(0), m_bInit(false)
{
}

UavManager::~UavManager()
{
    delete m_sqlEng;
    delete m_p;
}

int UavManager::PrcsBind(ObjectUav &uav, int ack, bool bBind, ObjectGS *sender)
{
    int res = -1;
    string gs = sender ? sender->GetObjectID() : string();
    if (gs.length() == 0)
        return res;
    string &gsOld = uav.m_lastBinder;

    bool bForceUnbind = sender && sender->GetAuth(ObjectGS::Type_UavManager);
    if (!uav.m_bExist)
        res = -2;
    else if (bBind && !bForceUnbind)
        res = (!uav.m_bBind || uav.m_lastBinder.empty() || gs==uav.m_lastBinder) ? 1 : -3;
    else if (!bBind)
        res = (bForceUnbind || gs == gsOld || gsOld.empty()) ? 1 : -3;

    if (res==1)
    {
        if(bBind!=uav.m_bBind)
        {
            uav.m_bBind = bBind;
            _saveBind(uav.GetObjectID(), bBind, gs);
        }
        if(gsOld != gs)
            gsOld = gs;
    }

    sendBindAck(uav, ack, res, bBind, gs);
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

void UavManager::SaveUavPos(const ObjectUav &uav)
{
    if (ExecutItem *item = VGDBManager::GetSqlByName("updatePos"))
    {
        item->ClearData();
        if (FiledVal *fd = item->GetConditionItem("id"))
            fd->SetParam(uav.GetObjectID());

        if (FiledVal *fd = item->GetWriteItem("lat"))
            fd->InitOf(uav.m_lat);
        if (FiledVal *fd = item->GetWriteItem("lon"))
            fd->InitOf(uav.m_lon);
        if (FiledVal *fd = item->GetWriteItem("timePos"))
            fd->InitOf(uav.m_tmLastPos);

        m_sqlEng->Execut(item);
    }
}

bool UavManager::CanFlight(double, double, double)
{
    return true;
}

uint32_t UavManager::toIntID(const std::string &uavid)
{
    StringList strLs = Utility::SplitString(uavid, ":");
    if (strLs.size() != 2
        || !Utility::Compare(strLs.front(), "VIGAU", false)
        || strLs.back().length() != 8)
        return 0;

    return (uint32_t)Utility::str2int(strLs.back(), 16);
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
    _getLastId();

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

void UavManager::_getLastId()
{
    ExecutItem *sql = VGDBManager::GetSqlByName("maxUavId");
    if (!sql)
        return;

    if (!m_sqlEng->Execut(sql))
        return;
    if (FiledVal *fd = sql->GetReadItem("max(id)"))
        m_lastId = toIntID((char*)fd->GetBuff());

    while (m_sqlEng->GetResult());
}

void UavManager::sendBindAck(const ObjectUav &uav, int ack, int res, bool bind, const std::string &gs)
{
    if (Uav2GSMessage *ms = new Uav2GSMessage(this, gs))
    {
        AckRequestBindUav *proto = new AckRequestBindUav();
        if (!proto)
            return;

        proto->set_seqno(ack);
        proto->set_opid(bind?1:0);
        proto->set_result(res);
        if (UavStatus *s = new UavStatus)
        {
            uav.transUavStatus(*s);
            proto->set_allocated_status(s);
        }
        ms->AttachProto(proto);
        SendMsg(ms);
    }
}

IObject *UavManager::_checkLogin(ISocket *s, const RequestUavIdentityAuthentication &uia)
{
    string uavid = Utility::Upper(uia.uavid());
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
            ret = new ObjectUav(uavid);
            ret->InitBySqlResult(*item);
            while (m_sqlEng->GetResult());
            res = 1;
        }
    }
    Log(0, uia.uavid(), 0, "[%s]%s", s->GetHost().c_str(), res==1 ? "login success" : "login fail");
    
    AckUavIdentityAuthentication ack;
    ack.set_seqno(uia.seqno());
    ack.set_result(res);
    ProtoMsg::SendProtoTo(ack, s);

    return ret;
}

void UavManager::_checkBindUav(const RequestBindUav &rbu, ObjectGS *gs)
{
    ObjectUav obj(rbu.uavid());
    if (ExecutItem *item = VGDBManager::GetSqlByName("queryUavInfo"))
    {
        item->ClearData();
        if (FiledVal *fd = item->GetConditionItem("id"))
            fd->SetParam(rbu.uavid());

        if (m_sqlEng->Execut(item))
            obj.InitBySqlResult(*item);

        while (m_sqlEng->GetResult());
    }
    PrcsBind(obj, rbu.seqno(),rbu.opid()==1, gs);
}

void UavManager::_checkUavInfo(const RequestUavStatus &uia, ObjectGS *gs)
{
    AckRequestUavStatus as;
    as.set_seqno(uia.seqno());

    for (int i = 0; i < uia.uavid_size(); ++i)
    {
        const string &uav = Utility::Upper(uia.uavid(i));
        ObjectUav *o = (ObjectUav *)GetObjectByID(uav);
        bool bMgr = gs && gs->GetAuth(ObjectGS::Type_UavManager);
        if (o)
        {
            UavStatus *us = as.add_status();
            o->transUavStatus(*us, bMgr);
            us = NULL;
        }
        else if (!_queryUavInfo(as, uav) && bMgr)
        {
            if (_addUavId(uav) > 0)
            {
                UavStatus *us = as.add_status();
                ObjectUav oU(uav);
                oU.transUavStatus(*us, bMgr);
                Log(0, gs?gs->GetObjectID():"", 0, "UavID (%d) insert!", uav);
            }
        }
    }

    if (gs)
    {
        Uav2GSMessage *ms = new Uav2GSMessage(this, gs->GetObjectID());
        ms->SetPBContent(as);
        SendMsg(ms);
    }
}

void UavManager::processAllocationUav(int seqno, const string &id)
{
    if (id.empty() && !m_sqlEng)
        return;

    if (GS2UavMessage *ms = new GS2UavMessage(this, id))
    {
        int res = 0;
        char idTmp[24] = {0};
        while (m_lastId > 0)
        {
            sprintf(idTmp, "VIGAU:%08X", m_lastId+1);
            int id = _addUavId(idTmp)>0?1:-1;
            res = id > 0 ? 1 : 0;
            if(res != 0)
                break;
        }

        AckIdentityAllocation *ack = new AckIdentityAllocation;
        ack->set_seqno(seqno);
        ack->set_result(res);
        ack->set_id(idTmp);
        ms->AttachProto(ack);
        SendMsg(ms);
        if (res == 1)
            Log(0, id, 0, "Allocate ID(%d) for UAV!", idTmp);
    }
}

int UavManager::_addUavId(const std::string &uav)
{
    uint32_t id = toIntID(uav);
    if (id < 1)
        return -1;

    ExecutItem *sql = VGDBManager::GetSqlByName("insertUavInfo");
    FiledVal *fd = sql->GetWriteItem("id");
    if (sql && fd)
    {
        sql->ClearData();
        fd->SetParam(uav);
        if (FiledVal *fdTmp = sql->GetWriteItem("authCheck"))
            fdTmp->SetParam(GSOrUavMessage::GenCheckString(8));
        if (m_sqlEng->Execut(sql))
        {
            if (m_lastId < id)
                m_lastId = id;
            return id;
        }
        return 0;
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

void UavManager::_saveBind(const std::string &uav, bool bBind, const std::string gs)
{
    if (ExecutItem *item = VGDBManager::GetSqlByName("updateBinded"))
    {
        item->ClearData();
        if (FiledVal *fd = item->GetConditionItem("id"))
            fd->SetParam(uav);

        if (FiledVal *fd = item->GetWriteItem("binded"))
            fd->InitOf<char>(bBind);
        if (FiledVal *fd = item->GetWriteItem("binder"))
            fd->SetParam(gs);
        if (FiledVal *fd = item->GetWriteItem("timeBind"))
            fd->InitOf(Utility::msTimeTick());
        m_sqlEng->Execut(item);
        Log(0, gs, 0, "%s %s", bBind ? "bind" : "unbind", uav.c_str());
    }
}

DECLARE_MANAGER_ITEM(UavManager)