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
    case ObjectEvent::E_Login:
    case ObjectEvent::E_Logout:
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
    if (o && o->IsInitaled())
    {
        bool bLogin = !o->IsConnect() && o->m_pswd == pswd;
        if (bLogin)
            o->OnLogined(bLogin, s);
        else
            Log(0, o->GetObjectID(), 0, "[%s:%d]%s", s->GetHost().c_str(), s->GetPort(), "login fail");

        AckIVIdentityAuthentication ack;
        ack.set_seqno(rgi->seqno());
        ack.set_result(bLogin ? 1 : 0);
        ObjectAbsPB::SendProtoBuffTo(s, ack);
    }

    return o;
}

void GVManager::LoadConfig()
{
    TiXmlDocument doc;
    doc.LoadFile("GVManager.xml");

    const TiXmlElement *rootElement = doc.RootElement();
    const TiXmlNode *node = rootElement ? rootElement->FirstChild("Manager") : NULL;
    const TiXmlElement *cfg = node ? node->ToElement() : NULL;
    int buSz = 1024;
    int n = 1;
    if (cfg)
    {
        const char *tmp = cfg->Attribute("thread");
        n = tmp ? (int)Utility::str2int(tmp) : 1;
        tmp = cfg->Attribute("buff");
        buSz = tmp ? (int)Utility::str2int(tmp) : 6144;
        InitThread(n, buSz);

        const TiXmlNode *dbNode = node ? node->FirstChild("Object") : NULL;
        while (dbNode)
        {
            if (ObjectGV *gv = ObjectGV::ParseObjecy(*dbNode->ToElement()))
                AddObject(gv);

            dbNode = dbNode->NextSibling("Object");
        }
    }
}

bool GVManager::IsHasReuest(const char *buf, int len) const
{
    return Utility::FindString(buf, len, d_p_ClassName(RequestIVIdentityAuthentication)) >= 8;
}

DECLARE_MANAGER_ITEM(GVManager)