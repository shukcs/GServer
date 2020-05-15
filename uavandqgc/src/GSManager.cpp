#include "GSManager.h"
#include "socketBase.h"
#include "das.pb.h"
#include "ProtoMsg.h"
#include "Messages.h"
#include "DBMessages.h"
#include "Utility.h"
#include "tinyxml.h"
#include "ObjectGS.h"
#include "ObjectManagers.h"
#include "FWItem.h"

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
, m_bInit(false)
{
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
        msg = new DBMessage(gs, IMessage::GSInsertRslt);
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
        SendMsg(msg);
        return 1;
    }
    return 0;
}

string GSManager::CatString(const string &s1, const string &s2)
{
    return s1 < s2 ? s1 + ":" + s2 : s2 + ":" + s1;
}

int GSManager::GetObjectType() const
{
    return IObject::GroundStation;
}

IObject *GSManager::PrcsProtoBuff(ISocket *s)
{
    if (!m_p)
        return NULL;

    const string &name = m_p->GetMsgName();
    Message *pb = m_p->GetProtoMessage();
    if (name == d_p_ClassName(RequestGSIdentityAuthentication))
        return prcsPBLogin(s, (const RequestGSIdentityAuthentication *)pb);
    else if (name == d_p_ClassName(RequestNewGS))
        return prcsPBNewGs(s, (const RequestNewGS *)pb);

    return NULL;
}

bool GSManager::PrcsPublicMsg(const IMessage &ms)
{
    if (ms.GetMessgeType() == IMessage::Gs2GsMsg)
    {
        auto gsmsg = (const GroundStationsMessage *)ms.GetContent();
        if (Gs2GsMessage *msg = new Gs2GsMessage(this, ms.GetSenderID()))
        {
            auto ack = new AckGroundStationsMessage;
            ack->set_seqno(gsmsg->seqno());
            ack->set_res(0);
            ack->set_gs(ms.GetReceiverID());
            msg->AttachProto(ack);
            SendMsg(msg);
        }
    }
    return true;
}

IObject *GSManager::prcsPBLogin(ISocket *s, const RequestGSIdentityAuthentication *rgi)
{
    if (!rgi)
        return NULL;

    string usr = Utility::Lower(rgi->userid());
    string pswd = rgi->password();
    ObjectGS *o = (ObjectGS*)GetObjectByID(usr);
    if (o && o->IsInitaled())
    {
        bool bLogin = !o->IsConnect() && o->m_pswd == pswd;  
        o->OnLogined(bLogin, s);

        AckGSIdentityAuthentication ack;
        ack.set_seqno(rgi->seqno());
        ack.set_result(bLogin ? 1 : 0);
        ack.set_auth(0);
        ObjectAbsPB::SendProtoBuffTo(s, ack);

        if (!bLogin)
            o = NULL;
    }
    else if(o==NULL)
    {
        o = new ObjectGS(usr);
        o->SetSeq(rgi->seqno());
        o->SetPswd(pswd);
    }

    return o;
}

IObject *GSManager::prcsPBNewGs(ISocket *s, const das::proto::RequestNewGS *msg)
{
    if(!s || !msg || msg->userid().empty())
        return NULL;

    string userId = Utility::Lower(msg->userid());
    ObjectGS *o = (ObjectGS *)GetObjectByID(userId);
    if (o && o->GetSocket())
    {
        AckNewGS ack;
        ack.set_seqno(msg->seqno());
        ack.set_result(0);
        ObjectAbsPB::SendProtoBuffTo(s, ack);
        return NULL;
    }
    if (!o)
        o = new ObjectGS(userId);

    if (o)
    {
        o->SetCheck(GSOrUavMessage::GenCheckString());
        o->_checkGS(userId, msg->seqno());
    }
    return o;
}

void GSManager::LoadConfig()
{
    TiXmlDocument doc;
    doc.LoadFile("GSManager.xml");

    const TiXmlElement *rootElement = doc.RootElement();
    const TiXmlNode *node = rootElement ? rootElement->FirstChild("GSManager") : NULL;
    const TiXmlElement *cfg = node ? node->ToElement() : NULL;
    if (cfg)
    {
        const char *tmp = cfg->Attribute("thread");
        int n = tmp ? (int)Utility::str2int(tmp) : 1;
        tmp = cfg->Attribute("buff");
        int buSz = tmp ? (int)Utility::str2int(tmp) : 6144;
        InitThread(n, buSz);
    }
}

bool GSManager::InitManager()
{
    if (!m_bInit)
    {
        m_bInit = true;
        AddDatabaseUser("root", "NIhao666", NULL, 0, ObjectGS::Type_ALL);
    }

    return m_bInit;
}

bool GSManager::IsHasReuest(const char *buf, int len) const
{
    return Utility::FindString(buf, len, d_p_ClassName(RequestGSIdentityAuthentication)) >= 8
        || Utility::FindString(buf, len, d_p_ClassName(RequestNewGS)) >= 8;
}

DECLARE_MANAGER_ITEM(GSManager)