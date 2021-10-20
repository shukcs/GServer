#include "GVManager.h"
#include "socketBase.h"
#include "das.pb.h"
#include "ProtoMsg.h"
#include "Messages.h"
#include "Utility.h"
#include "tinyxml.h"
#include "ObjectGV.h"
#include "ObjectManagers.h"

using namespace das::proto;
using namespace google::protobuf;
using namespace std;
#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif

////////////////////////////////////////////////////////////////////////////////
//GVManager
////////////////////////////////////////////////////////////////////////////////
GVManager::GVManager() : AbsPBManager()
{
}

GVManager::~GVManager()
{
}

const StringList &GVManager::OnLineTrackers() const
{
    return m_trackers;
}

string GVManager::CatString(const string &s1, const string &s2)
{
    return s1 < s2 ? s1 + ":" + s2 : s2 + ":" + s1;
}

int GVManager::GetObjectType() const
{
    return ObjectGV::GVType();
}

IObject *GVManager::PrcsProtoBuff(ISocket *s)
{
    if (!m_p)
        return NULL;

    const string &name = m_p->GetMsgName();
    Message *pb = m_p->GetProtoMessage();
    if (name == d_p_ClassName(RequestIVIdentityAuthentication))
        return prcsPBLogin(s, (const RequestIVIdentityAuthentication *)pb);

    return NULL;
}

bool GVManager::PrcsPublicMsg(const IMessage &ms)
{
    switch (ms.GetMessgeType())
    {
    case IMessage::PushUavSndInfo:
        return false;
    case ObjectSignal::S_Login:
        m_trackers.push_back(ms.GetSenderID());
        return false;
    case ObjectSignal::S_Logout:
        m_trackers.remove(ms.GetSenderID());
        return false;
    case IMessage::ControlUser:
        return false;
    default:
        break;
    }
    return true;
}

IObject *GVManager::prcsPBLogin(ISocket *s, const RequestIVIdentityAuthentication *rgi)
{
    if (!rgi)
        return NULL;

    string usr = Utility::Lower(rgi->userid());
    string pswd = rgi->password();
    ObjectGV *o = (ObjectGV*)GetObjectByID(usr);
    if (o)
    {
        bool bLogin = !o->IsConnect() && o->m_pswd == pswd;
        if (bLogin)
            o->OnLogined(true, s);
        else
            Log(0, IObjectManager::GetObjectFlagID(o), 0, "[%s:%d]%s", s->GetHost().c_str(), s->GetPort(), "login fail");

        AckIVIdentityAuthentication ack;
        ack.set_seqno(rgi->seqno());
        ack.set_result(bLogin ? 1 : 0);
        ObjectAbsPB::SendProtoBuffTo(s, ack);
    }

    return o;
}

void GVManager::LoadConfig(const TiXmlElement *root)
{
    const TiXmlElement *cfg = root ? root->FirstChildElement("GVManager") : NULL;
    if (cfg)
    {
        int buSz = 1024;
        int n = 1;
        const char *tmp = cfg->Attribute("thread");
        n = tmp ? (int)Utility::str2int(tmp) : 1;
        tmp = cfg->Attribute("buff");
        buSz = tmp ? (int)Utility::str2int(tmp) : 6144;
        InitThread(n, buSz);

        const TiXmlElement *dbNode = cfg->FirstChildElement("Object");
        while (dbNode)
        {
            if (ObjectGV *gv = ObjectGV::ParseObjecy(*dbNode))
                AddObject(gv);

            dbNode = dbNode->NextSiblingElement("Object");
        }
    }
}

bool GVManager::IsHasReuest(const char *buf, int len) const
{
    return Utility::FindString(buf, len, d_p_ClassName(RequestIVIdentityAuthentication)) >= 8;
}

DECLARE_MANAGER_ITEM(GVManager)