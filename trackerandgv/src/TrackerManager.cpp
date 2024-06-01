#include "TrackerManager.h"
#include "ObjectManagers.h"
#include "das.pb.h"
#include "ProtoMsg.h"
#include "common/Utility.h"
#include "common/Varient.h"
#include "net/socketBase.h"
#include "tinyxml.h"
#include "ObjectTracker.h"
#include "Messages.h"
#include "ObjectGV.h"

#define PROTOFLAG "das.proto."

using namespace std;
using namespace das::proto;
////////////////////////////////////////////////////////////////////////////////
//TrackerManager
////////////////////////////////////////////////////////////////////////////////
TrackerManager::TrackerManager() : AbsPBManager()
{
    InitPrcsLogin((PrcsLoginHandle)&TrackerManager::PrcsProtoBuff);
}

TrackerManager::~TrackerManager()
{
}

bool TrackerManager::CanFlight(double, double, double)
{
    return true;
}

uint32_t TrackerManager::toIntID(const std::string &uavid)
{
    StringList strLs = Utility::SplitString(uavid, ":");
    if (strLs.size() != 2
        || !Utility::Compare(strLs.front(), "VIGAT", false)
        || strLs.back().length() != 8)
        return 0;

    return (uint32_t)Utility::str2int(strLs.back(), 16);
}

bool TrackerManager::IsValid3rdID(const std::string &id)
{
    return id == "YIFEI:SERVER";
}

int TrackerManager::GetObjectType() const
{
    return ObjectTracker::TrackerType();
}

IObject *TrackerManager::PrcsProtoBuff(ISocket *s, const google::protobuf::Message *proto)
{
    if (!s)
        return NULL;

    const string &name = proto->GetDescriptor()->full_name();
    if (name == d_p_ClassName(RequestTrackerIdentityAuthentication))
    {
        auto *rua = (const RequestTrackerIdentityAuthentication *)proto;
        return _checkLogin(s, *rua);
    }
    if (name == d_p_ClassName(Request3rdIdentityAuthentication))
    {
        auto rua = (const Request3rdIdentityAuthentication *)proto;
        return _check3rdLogin(s, *rua);
    }
    if (name == d_p_ClassName(RequestProgramUpgrade))
    {
       return _checkProgram(s, *(const RequestProgramUpgrade*)proto);
    }

    return NULL;
}

bool TrackerManager::PrcsPublicMsg(const IMessage &)
{
    return true;
}

void TrackerManager::LoadConfig(const TiXmlElement *rootElement)
{
    const TiXmlElement *cfg = rootElement ? rootElement->FirstChildElement("TrackerManager") : NULL;
    if (cfg)
    {
        const char *tmp = cfg->Attribute("thread");
        int n = tmp ? (int)Utility::str2int(tmp) : 1;
        tmp = cfg->Attribute("buff");
        int sz = tmp ? (int)Utility::str2int(tmp) : 512;
        InitThread(n, sz);
        AddObject(new ObjectTracker("VIGAT:00000000"));
    }
}

bool TrackerManager::IsHasReuest(const char *buf, int len) const
{
    return Utility::FindString(buf, len, d_p_ClassName(RequestTrackerIdentityAuthentication)) >= 8
        || Utility::FindString(buf, len, d_p_ClassName(Request3rdIdentityAuthentication)) >= 8
        || Utility::FindString(buf, len, d_p_ClassName(RequestProgramUpgrade)) >= 8;
}

IObject *TrackerManager::_checkLogin(ISocket *s, const RequestTrackerIdentityAuthentication &uia)
{
    string uavid = Utility::Upper(uia.trackerid());
    if (toIntID(uavid) < 1)
        return NULL;

    ObjectTracker *ret = (ObjectTracker *)GetObjectByID(uavid);
    string sim = uia.has_extradata() ? uia.extradata() : "";
    Log(0, "Tracker:" + uavid, 0, "[%s:%d]%s", s->GetHost().c_str(), s->GetPort(), "RequestTrackerIdentityAuthentication!");
    if (ret)
    {
        AckTrackerIdentityAuthentication ack;
        ack.set_seqno(uia.seqno());
        if (!ret->IsConnect())
        {
            ack.set_result(1);
            ret->SetLogined(true, s);
            ret->SetSimId(sim);
        }
        else
        {
            ack.set_result(0);
            Log(0, IObjectManager::GetObjectFlagID(ret), 0, "[%s:%d]%s", s->GetHost().c_str(), s->GetPort(), "login fail");
        }

        ret->SendData2Link(&ack, s);
    }
    else
    {
        ret = new ObjectTracker(uavid, sim);
    }
    return ret;
}

IObject *TrackerManager::_check3rdLogin(ISocket *s, const Request3rdIdentityAuthentication &uia)
{
    string uavid = Utility::Upper(uia.identification());
    string sim = uia.secretkey();
    if (!IsValid3rdID(uavid))
        return NULL;

    ObjectTracker *ret = (ObjectTracker *)GetObjectByID(uavid);
    if (ret)
    {
        Ack3rdIdentityAuthentication ack;
        ack.set_seqno(uia.seqno());
        if (!ret->IsConnect())
        {
            ack.set_result(1);
            ret->SetLogined(true, s);
            ret->SetSimId(sim);
        }
        else
        {
            ack.set_result(0);
            Log(0, IObjectManager::GetObjectFlagID(ret), 0, "[%s:%d]%s", s->GetHost().c_str(), s->GetPort(), "login fail");
        }

        ret->SendData2Link(&ack, s);
    }
    else
    {
        ret = new ObjectTracker(uavid, sim);
    }
    return ret;
}

IObject *TrackerManager::_checkProgram(ISocket *s, const RequestProgramUpgrade &rpu)
{
    string uavid = Utility::Upper(rpu.has_extradata() ? rpu.extradata() : string());
    Log(0, "Tracker:" + uavid, 0, "[%s:%d]%s", s->GetHost().c_str(), s->GetPort(), "RequestProgramUpgrade!");

    auto ret = (ObjectTracker *)GetObjectByID("VIGAT:00000000");

    if (ret)
    {
        AckProgramUpgrade ack;
        ack.set_seqno(rpu.seqno());
        ack.set_result(1);
        ack.set_software(rpu.software());
        ack.set_length(0);
        ack.set_forced(false);
        ret->SendData2Link(&ack, s);
    }

    return ret;
}


DECLARE_MANAGER_ITEM(TrackerManager)