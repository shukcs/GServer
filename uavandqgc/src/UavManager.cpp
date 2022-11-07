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
#include "DBExecItem.h"

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
, m_lastId(0)
{
    typedef IObject *(UavManager::*PrcsLoginFunc)(ISocket *, const void *);
    InitPrcsLogin(
        std::bind((PrcsLoginFunc)&UavManager::PrcsProtoBuff
            , this, std::placeholders::_1, std::placeholders::_2));
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

IObject *UavManager::PrcsProtoBuff(ISocket *s, const Message *proto)
{
    if (!s)
        return NULL;

    if (proto->GetDescriptor()->full_name() == d_p_ClassName(RequestUavIdentityAuthentication))
    {
        auto rua = (const RequestUavIdentityAuthentication *)proto;
        return _checkLogin(s, *rua);
    }

    return NULL;
}

bool UavManager::PrcsPublicMsg(const IMessage &msg)
{
    if (m_lastId == 0)
    {
        _getLastId();
        m_lastId = 1;
    }

    Message *proto = (Message *)msg.GetContent();
    switch (msg.GetMessgeType())
    {
    case IMessage::QueryDevice:
        checkUavInfo(*(RequestUavStatus *)proto, *(GS2UavMessage*)&msg);
        return true;
    case IMessage::ControlDevice:
        SendMsg(ObjectUav::AckControl2Uav(*(PostControl2Uav*)proto, -1));
        return true;
    case IMessage::DeviceAllocation:
        processAllocationUav(((RequestIdentityAllocation *)proto)->seqno(), msg.GetSenderID());
        return true;
    case IMessage::NotifyFWUpdate:
        processNotifyProgram(*(NotifyProgram *)proto);
        return true;
    case IMessage::DeviceisMaxIDRslt:
        processMaxID(*(DBMessage *)&msg);
        return true;
    default:
        break;
    }

    return false;
}

void UavManager::LoadConfig(const TiXmlElement *root)
{
    const TiXmlElement *cfg = root ? root->FirstChildElement("UavManager") : NULL;
    if (cfg)
    {
        const char *tmp = cfg->Attribute("thread");
        int n = tmp ? (int)Utility::str2int(tmp) : 1;
        InitThread(n, 512);
    }
}

bool UavManager::IsHasReuest(const char *buf, int len) const
{
    return Utility::FindString(buf, len, d_p_ClassName(RequestUavIdentityAuthentication)) >= 8;
}

void UavManager::_getLastId()
{
    if (DBMessage *msg = new DBMessage(this, IMessage::DeviceisMaxIDRslt))
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
            ret->SetSimId(sim);
            ret->OnLogined(true, s);
        }
        else
        {
            Log(0, IObjectManager::GetObjectFlagID(ret), 0, "[%s:%d]%s", s->GetHost().c_str(), s->GetPort(), "login fail");
        }

        AckUavIdentityAuthentication ack;
        ack.set_seqno(uia.seqno());
        ack.set_result(bLogin ? 1 : 0);
        ObjectAbsPB::SendProtoBuffTo(s, ack);
    }
    else
    {
        ret = new ObjectUav(uavid, sim);
    }
    return ret;
}

void UavManager::checkUavInfo(const RequestUavStatus &uia, const GS2UavMessage &gs)
{
    AckRequestUavStatus as;
    as.set_seqno(uia.seqno());

    StringList strLs;
    bool bMgr = (0 != (gs.GetAuth()&IObject::Type_Manager));
    const string &id = gs.GetSenderID();
    for (int i = 0; i < uia.uavid_size(); ++i)
    {
        string uav = Utility::Upper(uia.uavid(i));
        auto o = (ObjectUav *)GetObjectByID(uav);
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
        queryUavInfo(id, uia.seqno(), strLs, bMgr && uia.uavid_size()==1);

    if (!id.empty())
    {
        Uav2GSMessage *ms = new Uav2GSMessage(this, id);
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
            Log(0, "UAV:" + id, 0, "Allocate ID(%d) for UAV!", idTmp);
    }
}

void UavManager::processNotifyProgram(const NotifyProgram &)
{
}

void UavManager::processMaxID(const DBMessage &msg)
{
    const Variant &v = msg.GetRead("max(id)");
    if (v.GetType() == Variant::Type_string)
        m_lastId = toIntID(v.ToString())+1;
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
        msg->SetWrite("authCheck", Utility::RandString(8));
        SendMsg(msg);
    }
}

void UavManager::queryUavInfo(const string &gs, int seq, const std::list<std::string> &uavs, bool bAdd)
{
    if (gs.empty())
        return;

    if (DBMessage *msg = new DBMessage(gs,IObject::GroundStation, IMessage::DeviceisQueryRslt, DBMessage::DB_Uav))
    {
        msg->SetSeqNomb(seq);
        int idx = 0;
        if (bAdd && uavs.size()==1)
        {
            if (uint32_t id = toIntID(uavs.front()))
            {
                msg->SetSql("insertUavInfo", true);
                msg->SetWrite("id", uavs.front());
                msg->SetWrite("authCheck", Utility::RandString(8));
                idx = 1;
                if (id > m_lastId)
                    m_lastId = id+1;
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

DECLARE_MANAGER_ITEM(UavManager)