#include "UavManager.h"
#include "ObjectManagers.h"
#include "das.pb.h"
#include "ProtoMsg.h"
#include "Utility.h"
#include "Varient.h"
#include "socketBase.h"
#include "tinyxml.h"
#include "ObjectUav.h"
#include "Messages.h"
#include "DBMessages.h"
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
UavManager::UavManager() : AbsPBManager()
, m_lastId(0), m_bInit(false)
{
}

UavManager::~UavManager()
{
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

IObject *UavManager::PrcsProtoBuff(ISocket *s)
{
    if (!s || !m_p)
        return NULL;

    if (m_p->GetMsgName() == d_p_ClassName(RequestUavIdentityAuthentication))
    {
        RequestUavIdentityAuthentication *rua = (RequestUavIdentityAuthentication *)m_p->GetProtoMessage();
        return _checkLogin(s, *rua);
    }

    return NULL;
}

bool UavManager::PrcsPublicMsg(const IMessage &msg)
{
    Message *proto = (Message *)msg.GetContent();
    switch (msg.GetMessgeType())
    {
    case IMessage::BindUav:
        _checkBindUav(*(RequestBindUav *)proto, (ObjectGS*)msg.GetSender());
        return true;
    case IMessage::QueryUav:
        _checkUavInfo(*(RequestUavStatus *)proto, (ObjectGS*)msg.GetSender());
        return true;
    case IMessage::ControlUav:
        SendMsg(ObjectUav::AckControl2Uav(*(PostControl2Uav*)proto, -1));
        return true;
    case IMessage::UavAllocation:
        processAllocationUav(((RequestIdentityAllocation *)proto)->seqno(), msg.GetSenderID());
        return true;
    case IMessage::NotifyFWUpdate:
        processNotifyProgram(*(NotifyProgram *)proto);
        return true;
    case IMessage::UavsMaxIDRslt:
        processMaxID(*(DBMessage *)&msg);
        return true;
    default:
        break;
    }

    return false;
}

void UavManager::LoadConfig()
{
    TiXmlDocument doc;
    doc.LoadFile("UavManager.xml");
    const TiXmlElement *rootElement = doc.RootElement();
    const TiXmlNode *node = rootElement ? rootElement->FirstChild("UavManager") : NULL;
    const TiXmlElement *cfg = node ? node->ToElement() : NULL;
    if (cfg)
    {
        const char *tmp = cfg->Attribute("thread");
        int n = tmp ? (int)Utility::str2int(tmp) : 1;
        InitThread(n, 512);
    }
}

bool UavManager::InitManager()
{
    if (!m_bInit)
    {
        m_bInit = true;
        _getLastId();
    }
    return m_bInit;
}

bool UavManager::IsHasReuest(const char *buf, int len) const
{
    return Utility::FindString(buf, len, d_p_ClassName(RequestUavIdentityAuthentication)) >= 8;
}

void UavManager::_getLastId()
{
    if (DBMessage *msg = new DBMessage(this, IMessage::UavsMaxIDRslt))
    {
        msg->SetSql("maxUavId");
        SendMsg(msg);
    }
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
            uav.TransUavStatus(*s);
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
    string sim = uia.has_extradata() ? uia.extradata() : "";
    if (ret)
    {
        bool bLogin = !ret->IsConnect() && ret->IsInitaled();
        if (bLogin)
        {
            ret->SetSocket(s);
            ret->SetSimId(sim);
        }

        ret->OnLogined(bLogin);
        AckUavIdentityAuthentication ack;
        ack.set_seqno(uia.seqno());
        ack.set_result(bLogin ? 1 : 0);
        s->ClearBuff();
        ObjectAbsPB::SendProtoBuffTo(s, ack);
    }
    else
    {
        s->ClearBuff();
        ObjectUav *ret = new ObjectUav(uavid, sim);
        return ret;
    }

    return NULL;
}

void UavManager::_checkBindUav(const RequestBindUav &rbu, ObjectGS *gs)
{
    bool bBind = gs->GetAuth(ObjectGS::Type_UavManager);
    if (gs || bBind)
        saveBind(rbu.uavid(), rbu.opid()==1&&!bBind, gs);
}

void UavManager::_checkUavInfo(const RequestUavStatus &uia, ObjectGS *gs)
{
    AckRequestUavStatus as;
    as.set_seqno(uia.seqno());

    StringList strLs;
    bool bMgr = gs && gs->GetAuth(ObjectGS::Type_UavManager);
    for (int i = 0; i < uia.uavid_size(); ++i)
    {
        const string &tmp = uia.uavid(i);
        string uav = Utility::Upper(tmp);
        ObjectUav *o = (ObjectUav *)GetObjectByID(uav);
        if (o)
        {
            UavStatus *us = as.add_status();
            o->TransUavStatus(*us, bMgr);
            us = NULL;
        }
        else
        {
            strLs.push_back(uav);
        }
    }
    if (strLs.size() > 0)
        queryUavInfo(gs, uia.seqno(), strLs, bMgr);

    if (gs)
    {
        Uav2GSMessage *ms = new Uav2GSMessage(this, gs->GetObjectID());
        ms->SetPBContent(as);
        SendMsg(ms);
    }
}

void UavManager::processAllocationUav(int seqno, const string &id)
{
    if (id.empty())
        return;

    if (GS2UavMessage *ms = new GS2UavMessage(this, id))
    {
        int res = 0;
        char idTmp[24] = {0};

        sprintf(idTmp, "VIGAU:%08X", m_lastId++);
        addUavId(seqno, idTmp);

        if (AckIdentityAllocation *ack = new AckIdentityAllocation)
        {
            ack->set_seqno(seqno);
            ack->set_result(res);
            ack->set_id(idTmp);
            ms->AttachProto(ack);
            SendMsg(ms);
        }
        if (res == 1)
            Log(0, id, 0, "Allocate ID(%d) for UAV!", idTmp);
    }
}

void UavManager::processNotifyProgram(const NotifyProgram &)
{
}

void UavManager::processMaxID(const DBMessage &msg)
{
    const Variant &v = msg.GetRead("max(id)");
    if (v.GetType() == Variant::Type_string)
        m_lastId = toIntID(v.ToString());
}

void UavManager::addUavId(int seq, const std::string &uav)
{
    uint32_t id = toIntID(uav);
    if (id < 1)
        return;

    if (DBMessage *msg = new DBMessage(this))
    {
        msg->SetSeqNomb(seq);
        msg->SetSql("insertUavInfo");
        msg->SetWrite("id", uav);
        msg->SetWrite("authCheck", GSOrUavMessage::GenCheckString(8));
        SendMsg(msg);
    }
}

void UavManager::queryUavInfo(ObjectGS *gs, int seq, const std::list<std::string> &uavs, bool bAdd)
{
    if (!gs)
        return;

    if (DBMessage *msg = new DBMessage(gs, IMessage::UavsQueryRslt, DBMessage::DB_Uav))
    {
        msg->SetSeqNomb(seq);
        int idx = 0;
        if (bAdd && uavs.size()==1)
        {
            uint32_t id = toIntID(uavs.front());
            if (id >= 1)
            {
                msg->SetSql("insertUavInfo", true);
                msg->SetWrite("id", uavs.front(), 1);
                msg->SetWrite("authCheck", GSOrUavMessage::GenCheckString(8), 1);
                idx = 2;
                if (id > m_lastId)
                    m_lastId = id;
            }
        }
        if (idx < 1)
            msg->SetSql("queryUavInfo", true);
        else
            msg->AddSql("queryUavInfo");

        msg->SetCondition("id", uavs, idx);
        SendMsg(msg);
    }
}

void UavManager::saveBind(const std::string &uav, bool bBind, ObjectGS *gs)
{
    if (!gs)
        return;

    if (DBMessage *msg = new DBMessage(gs, IMessage::UavBindRslt, DBMessage::DB_Uav))
    {
        bool force = gs->GetAuth(ObjectGS::Type_UavManager);
        msg->SetSql("updateBinded");
        msg->AddSql("queryUavInfo");
        msg->SetWrite("binder", gs->GetObjectID(), 1);
        msg->SetWrite("timeBind", Utility::msTimeTick(), 1);
        msg->SetWrite("binded", force ? false : bBind, 1);
        msg->SetCondition("id", uav, 1);
        if (!force)
        {
            msg->SetCondition("UavInfo.binded", false, 1);
            msg->SetCondition("UavInfo.binder", gs->GetObjectID(), 1);
        }

        msg->SetCondition("id", uav, 2);
        SendMsg(msg);
        Log(0, gs->GetObjectID(), 0, "%s %s", bBind ? "bind" : "unbind", uav.c_str());
    }
}

DECLARE_MANAGER_ITEM(UavManager)