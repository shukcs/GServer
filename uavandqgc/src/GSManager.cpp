#include "GSManager.h"
#include "net/socketBase.h"
#include "das.pb.h"
#include "ProtoMsg.h"
#include "Messages.h"
#include "DBMessages.h"
#include "common/Utility.h"
#include "tinyxml.h"
#include "ObjectGS.h"
#include "ObjectUav.h"
#include "ObjectManagers.h"
#include "FWItem.h"
#include "DBExecItem.h"

using namespace das::proto;
using namespace google::protobuf;
using namespace std;
#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif

////////////////////////////////////////////////////////////////////////////////
//GSManager
////////////////////////////////////////////////////////////////////////////////
GSManager::GSManager() : AbsPBManager()
{
    InitPrcsLogin((PrcsLoginHandle)&GSManager::PrcsProtoBuff);
}

GSManager::~GSManager()
{
}

int GSManager::AddDatabaseUser(const string &user, const string &pswd, ObjectGS *gs, int seq, int auth)
{
    if ((auth&ObjectGS::Type_ALL) != auth)
        return -1;

    DBMessage *msg = NULL;
    if (gs)
        msg = new DBMessage(*gs, IMessage::UserInsertRslt);
    else
        msg = new DBMessage(this);

    if (msg)
    {
        msg->SetSql("insertGSInfo");
        if (seq > 0)
            msg->SetSeqNomb(seq);

        msg->SetWrite("user", user);
        msg->SetWrite("pswd", pswd);
        msg->SetWrite("auth", auth);
        msg->SetWrite("regTm", (int64_t)Utility::secTimeCount());
        SendMsg(msg);
        return 1;
    }
    return 0;
}

const StringList &GSManager::Uavs() const
{
    return m_uavs;
}

string GSManager::CatString(const string &s1, const string &s2)
{
    return s1 < s2 ? s1 + ":" + s2 : s2 + ":" + s1;
}

int GSManager::GetObjectType() const
{
    return IObject::GroundStation;
}

IObject *GSManager::PrcsProtoBuff(ISocket *s, const google::protobuf::Message *proto)
{
    if (!s || !proto)
        return NULL;

    const string &name = proto->GetDescriptor()->full_name();
    if (name == d_p_ClassName(RequestGSIdentityAuthentication))
        return prcsPBLogin(s, (const RequestGSIdentityAuthentication *)proto);
    else if (name == d_p_ClassName(RequestNewGS))
        return prcsPBNewGs(s, (const RequestNewGS *)proto);

    return NULL;
}

bool GSManager::PrcsPublicMsg(const IMessage &ms)
{
    switch (ms.GetMessgeType())
    {
    case IMessage::User2User:
        processGSMessage(*(Gs2GsMessage*)&ms);
        break;
    case ObjectSignal::S_Login:
        processDeviceLogin(ms.GetSenderType(), ms.GetSenderID(), true);
        break;
    case ObjectSignal::S_Logout:
        processDeviceLogin(ms.GetSenderType(), ms.GetSenderID(), false);
        break;
    default:
        break;
    }
    return true;
}

void GSManager::processDeviceLogin(int tp, const std::string &dev, bool bLogin)
{
    if (tp != ObjectUav::UAVType() || dev.empty())
        return;

    if (bLogin)
        m_uavs.push_back(dev);
    else
        m_uavs.remove(dev);
    UpdateDeviceList udl;
    static int seq = 1;
    udl.set_seqno(seq++);
    udl.set_operation(bLogin ? 1 : 0);
    udl.add_id(dev);

    for (auto itr : m_objs)
    {
        if (itr->IsLinked())
            itr->CopyAndSend(udl);
    }
}

void GSManager::processGSMessage(const Gs2GsMessage &gsM)
{
    if (Gs2GsMessage *msg = new Gs2GsMessage(this, gsM.GetSenderID()))
    {
        auto gsmsg = (const GroundStationsMessage *)gsM.GetProtobuf();
        auto ack = new AckGroundStationsMessage;
        ack->set_seqno(gsmsg->seqno());
        ack->set_res(0);
        ack->set_gs(gsM.GetReceiverID());
        msg->AttachProto(ack);
        SendMsg(msg);
    }
}

IObject *GSManager::prcsPBLogin(ISocket *s, const RequestGSIdentityAuthentication *rgi)
{
    if (!rgi)
        return NULL;

    string usr = Utility::Lower(rgi->userid());
    string pswd = rgi->password();
    auto ret = (ObjectGS*)GetObjectByID(usr);
    if (ret)
    {
        s->SetHandleLink(ret);
        bool bLogin = !ret->IsConnect() && ret->GetPswd() == pswd;
		ret->SetLogined(bLogin, s);

        AckGSIdentityAuthentication ack;
        ack.set_seqno(rgi->seqno());
        ack.set_result(bLogin ? 1 : 0);
        ack.set_auth(ret->Authorize());

        ret->SendData2Link(&ack, s);
    }
    else if(ret ==NULL)
    {
		ret = new ObjectGS(usr, rgi->seqno());
		ret->SetPswd(pswd);
    }

    return ret;
}

IObject *GSManager::prcsPBNewGs(ISocket *s, const das::proto::RequestNewGS *msg)
{
    if(!s || !msg || msg->userid().empty())
        return NULL;

    string userId = Utility::Lower(msg->userid());
    ObjectGS *o = (ObjectGS *)GetObjectByID(userId);
    if (o && (o->IsLinked() || !o->IsAllowRelease()))
    {
        AckNewGS ack;
        ack.set_seqno(msg->seqno());
        ack.set_result(0);
        o->SendData2Link(&ack, s);
        return NULL;
    }
    if (!o)
	{
		o = new ObjectGS(userId);
		o->SetAuth(IObject::Type_ReqNewUser);
	}

    if (o)
    {
        o->SetCheck(Utility::RandString());
        o->_checkGS(userId, msg->seqno());
    }
    return o;
}

void GSManager::LoadConfig(const TiXmlElement *root)
{
    const TiXmlElement *cfg = root ? root->FirstChildElement("GSManager") : NULL;
    if (cfg)
    {
        const char *tmp = cfg->Attribute("thread");
        int n = tmp ? (int)Utility::str2int(tmp) : 2;
        tmp = cfg->Attribute("buff");
        int buSz = tmp ? (int)Utility::str2int(tmp) : 6144;
        InitThread(n, buSz);

        const TiXmlElement *e = cfg->FirstChildElement("Object");
        while (e)
        {
            const char *id = e->Attribute("id");
            const char *pswd = e->Attribute("pswd");
            if (id && pswd)
            {
                auto obj = new ObjectGS(id);
                if (!obj)
                    return;
                obj->SetPswd(pswd);
                const char *auth = e->Attribute("auth");
                obj->m_auth = auth ? int(Utility::str2int(auth)) : 3;
                AddObject(obj);
                m_objs.push_back(obj);
            }

            e = e->NextSiblingElement("Object");
        }
    }
}

bool GSManager::IsHasReuest(const char *buf, int len) const
{
    return Utility::FindString(buf, len, d_p_ClassName(RequestGSIdentityAuthentication)) >= 8
        || Utility::FindString(buf, len, d_p_ClassName(RequestNewGS)) >= 8;
}

DECLARE_MANAGER_ITEM(GSManager)