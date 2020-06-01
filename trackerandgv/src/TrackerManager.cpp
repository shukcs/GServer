#include "TrackerManager.h"
#include "ObjectManagers.h"
#include "das.pb.h"
#include "ProtoMsg.h"
#include "Utility.h"
#include "Varient.h"
#include "socketBase.h"
#include "tinyxml.h"
#include "ObjectTracker.h"
#include "Messages.h"
#include "ObjectGV.h"

#define PROTOFLAG "das.proto."

using namespace std;
using namespace das::proto;
using namespace ::google::protobuf;
#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif
////////////////////////////////////////////////////////////////////////////////
//TrackerManager
////////////////////////////////////////////////////////////////////////////////
TrackerManager::TrackerManager() : AbsPBManager()
{
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
        || !Utility::Compare(strLs.front(), "VIGAU", false)
        || strLs.back().length() != 8)
        return 0;

    return (uint32_t)Utility::str2int(strLs.back(), 16);
}

int TrackerManager::GetObjectType() const
{
    return IObject::User+1;
}

IObject *TrackerManager::PrcsProtoBuff(ISocket *s)
{
    if (!s || !m_p)
        return NULL;

    if (m_p->GetMsgName() == d_p_ClassName(RequestTrackerIdentityAuthentication))
    {
        RequestTrackerIdentityAuthentication *rua = (RequestTrackerIdentityAuthentication *)m_p->GetProtoMessage();
        return _checkLogin(s, *rua);
    }

    if (m_p->GetMsgName() == d_p_ClassName(RequestProgramUpgrade))
    {
        auto req = (RequestProgramUpgrade *)m_p->GetProtoMessage();
        AckProgramUpgrade ack;
        ack.set_seqno(req->seqno());
        ack.set_result(0);
        ack.set_software(req->software());
        ack.set_length(0);
        ack.set_forced(false);
        ObjectAbsPB::SendProtoBuffTo(s, ack);
    }

    return NULL;
}

bool TrackerManager::PrcsPublicMsg(const IMessage &msg)
{
    if (msg.GetMessgeType() == IMessage::SyncDeviceis)
    {
        const GV2TrackerMessage &ms = *(GV2TrackerMessage*)&msg;
        if (auto ack = new Tracker2GVMessage(this, msg.GetSenderID()))
        {
            auto dls = new AckSyncDeviceList;
            dls->set_seqno(((SyncDeviceList*)ms.GetProtobuf())->seqno());
            for (const pair<string, IObject *> &itr : m_objects)
            {
                dls->add_id(itr.first);
            }
            dls->set_result(1);
            ack->AttachProto(dls);
            SendMsg(ack);
        }
    }
    return true;
}

void TrackerManager::LoadConfig()
{
    TiXmlDocument doc;
    doc.LoadFile("TrackerManager.xml");
    const TiXmlElement *rootElement = doc.RootElement();
    const TiXmlNode *node = rootElement ? rootElement->FirstChild("Manager") : NULL;
    const TiXmlElement *cfg = node ? node->ToElement() : NULL;
    if (cfg)
    {
        const char *tmp = cfg->Attribute("thread");
        int n = tmp ? (int)Utility::str2int(tmp) : 1;
        InitThread(n, 512);
    }
}

bool TrackerManager::IsHasReuest(const char *buf, int len) const
{
    return Utility::FindString(buf, len, d_p_ClassName(RequestTrackerIdentityAuthentication)) >= 8
        || Utility::FindString(buf, len, d_p_ClassName(RequestProgramUpgrade)) >= 8;
}

IObject *TrackerManager::_checkLogin(ISocket *s, const RequestTrackerIdentityAuthentication &uia)
{
    string uavid = Utility::Upper(uia.trackerid());
    ObjectTracker *ret = (ObjectTracker *)GetObjectByID(uavid);
    string sim = uia.has_extradata() ? uia.extradata() : "";
    if (ret)
    {
        if (ret->GetSocket() == s)
            return NULL;

        bool bLogin = !ret->IsConnect() && ret->IsInitaled();
        if (bLogin)
        {
            ret->SetSimId(sim);
            ret->OnLogined(bLogin, s);
        }
        else
        {
            Log(0, ret->GetObjectID(), 0, "[%s:%d]%s", s->GetHost().c_str(), s->GetPort(), "login fail");
            ret = NULL;
        }

        AckTrackerIdentityAuthentication ack;
        ack.set_seqno(uia.seqno());
        ack.set_result(bLogin ? 1 : 0);
        ObjectAbsPB::SendProtoBuffTo(s, ack);
    }
    else
    {
        ret = new ObjectTracker(uavid, sim);
    }
    return ret;
}

DECLARE_MANAGER_ITEM(TrackerManager)