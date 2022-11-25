#include "VgFZManager.h"
#include "socketBase.h"
#include "das.pb.h"
#include "ProtoMsg.h"
#include "Messages.h"
#include "DBMessages.h"
#include "Utility.h"
#include "tinyxml.h"
#include "ObjectVgFZ.h"
#include "ObjectManagers.h"
#include "DBExecItem.h"

using namespace das::proto;
using namespace google::protobuf;
using namespace std;
#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif

////////////////////////////////////////////////////////////////////////////////
//VgFZManager
////////////////////////////////////////////////////////////////////////////////
VgFZManager::VgFZManager() : IObjectManager()
{
    InitPackPrcs((SerierlizePackFuc)&ProtobufParse::serialize
        , (ParsePackFuc)&ProtobufParse::Parse
        , (RelaesePackFuc)&ProtobufParse::releaseProtobuf);

    InitPrcsLogin((PrcsLoginHandle)&VgFZManager::PrcsProtoBuff);
}

VgFZManager::~VgFZManager()
{
}

int VgFZManager::AddDatabaseUser(const string &user, const string &pswd, ObjectVgFZ *gs, int seq, int auth)
{
    if ((auth&ObjectVgFZ::Type_ALL) != auth)
        return -1;

    DBMessage *msg = NULL;
    if (gs)
        msg = new DBMessage(gs, IMessage::UserInsertRslt);
    else
        msg = new DBMessage(this);

    if (msg)
    {
        msg->SetSql("insertFZInfo");
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

const StringList &VgFZManager::Uavs() const
{
    return m_uavs;
}

string VgFZManager::CatString(const string &s1, const string &s2)
{
    return s1 < s2 ? s1 + ":" + s2 : s2 + ":" + s1;
}

int VgFZManager::GetObjectType() const
{
    return IObject::VgFZ;
}

IObject *VgFZManager::PrcsProtoBuff(ISocket *s, const google::protobuf::Message *proto)
{
    const string name = proto->GetTypeName();
    if (name == d_p_ClassName(RequestFZUserIdentity))
        return prcsPBLogin(s, (const RequestFZUserIdentity *)proto);
    else if (name == d_p_ClassName(RequestNewFZUser))
        return prcsPBNewGs(s, (const RequestNewFZUser *)proto);
    else if (name == d_p_ClassName(PostGetFZPswd))
        return prcsPostGetFZPswd(s, (const PostGetFZPswd *)proto);

    return NULL;
}

bool VgFZManager::PrcsPublicMsg(const IMessage &ms)
{
    switch (ms.GetMessgeType())
    {
    case IMessage::User2User:
        processGSMessage(*(FZ2FZMessage*)&ms);
        break;
    default:
        break;
    }
    return true;
}

void VgFZManager::ToCurrntLog(int err, const std::string &obj, int evT, const std::string &dscb)
{
    if (auto msg = new DBMessage(this, "DBLog"))
    {
        msg->SetSql("insertLog");

        msg->SetWrite("event", evT);
        msg->SetWrite("error", err);
        msg->SetWrite("object", obj);
        msg->SetWrite("evntdesc", dscb);
        if (!SendMsg(msg))
            delete msg;
    }
}

void VgFZManager::processGSMessage(const FZ2FZMessage &gsM)
{
    if (FZ2FZMessage *msg = new FZ2FZMessage(this, gsM.GetSenderID()))
    {
        auto gsmsg = (const FZUserMessage *)gsM.GetProtobuf();
        auto ack = new AckFZUserMessage;
        ack->set_seqno(gsmsg->seqno());
        ack->set_res(0);
        ack->set_user(gsM.GetReceiverID());
        msg->AttachProto(ack);
        SendMsg(msg);
    }
}

IObject *VgFZManager::prcsPBLogin(ISocket *s, const RequestFZUserIdentity *rgi)
{
    if (!rgi)
        return NULL;

    string usr = Utility::Lower(rgi->user());
    string pswd = rgi->pswd();
    ObjectVgFZ *o = dynamic_cast<ObjectVgFZ *>(GetObjectByID(usr));
    if (o)
    {
        bool bLogin = !o->IsConnect() && o->IsInitaled() && o->m_pswd==pswd;
        if (bLogin)
            o->SetLogined(true, s);
        else
            Log(0, IObjectManager::GetObjectFlagID(o), 0, "[%s:%d]%s", s->GetHost().c_str(), s->GetPort(), "login fail");

        int rslt = !bLogin ? -2 : ((o->GetAuth(ObjectVgFZ::Type_Manager) || rgi->pcsn() == o->GetPCSn()) ? 1 : 0);
        if (rslt != 0)
        {
            AckFZUserIdentity ack;
            ack.set_seqno(rgi->seqno());
            ack.set_result(rslt);
            ack.set_ver(o->GetCurVer(rgi->pcsn()));
            o->SendData2Link(&ack, s);
        }

        o->SetPcsn(rgi->pcsn());
    }
    else if(o==NULL)
    {
        o = new ObjectVgFZ(usr, rgi->seqno());
        o->SetPcsn(rgi->pcsn());
    }

    if (o && !o->IsInitaled())
        o->SetPswd(pswd);

    return o;
}

IObject *VgFZManager::prcsPBNewGs(ISocket *s, const das::proto::RequestNewFZUser *msg)
{
    if(!s || !msg || msg->user().empty())
        return NULL;

    string userId = Utility::Lower(msg->user());
    ObjectVgFZ *o = dynamic_cast<ObjectVgFZ *>(GetObjectByID(userId));
    if (o && (o->IsConnect() || !o->IsAllowRelease()))
    {
        AckNewFZUser ack;
        ack.set_seqno(msg->seqno());
        ack.set_result(0);
        o->SendData2Link(&ack, s);
        return NULL;
    }
    if (!o)
        o = new ObjectVgFZ(userId);

    if (o)
    {
        o->SetAuth(IObject::Type_ReqNewUser);
        o->SetCheck(Utility::RandString());
        o->_checkFZ(userId, msg->seqno());
    }
    return o;
}

IObject * VgFZManager::prcsPostGetFZPswd(ISocket *s, const PostGetFZPswd *msg)
{
    if (!s || !msg || msg->user().empty() || msg->email().empty())
        return NULL;

    string userId = Utility::Lower(msg->user());
    ObjectVgFZ *o = dynamic_cast<ObjectVgFZ *>(GetObjectByID(userId));
    if (o && (o->IsConnect() || !o->IsAllowRelease()))
    {
        AckGetFZPswd ack;
        ack.set_seqno(msg->seqno());
        ack.set_rslt(-2);
        o->SendData2Link(&ack, s);
        return NULL;
    }

    if (!o)
        o = new ObjectVgFZ(userId);

    if (o)
    {
        o->SetAuth(IObject::Type_GetPswd);
        o->m_seq = msg->seqno();
        o->CheckMail(msg->email());
    }
    return o;
}

void VgFZManager::LoadConfig(const TiXmlElement *root)
{
    const TiXmlElement *cfg = root ? root->FirstChildElement("VgFZManager") : NULL;
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
                auto obj = new ObjectVgFZ(id);
                if (!obj)
                    return;
                obj->SetPswd(pswd);
                const char *auth = e->Attribute("auth");
                obj->m_auth = auth ? int(Utility::str2int(auth)) : 3;
                AddObject(obj);
                m_mgrs.push_back(obj);
            }

            e = e->NextSiblingElement("Object");
        }
    }
}

bool VgFZManager::IsHasReuest(const char *buf, int len) const
{
    return Utility::FindString(buf, len, d_p_ClassName(RequestFZUserIdentity)) >= 8
        || Utility::FindString(buf, len, d_p_ClassName(RequestNewFZUser)) >= 8
        || Utility::FindString(buf, len, d_p_ClassName(PostGetFZPswd)) >= 8;
}

DECLARE_MANAGER_ITEM(VgFZManager)
DeclareRcvPB(PostHeartBeat);
DeclareRcvPB(AckHeartBeat);
DeclareRcvPB(RequestFZUserIdentity);
DeclareRcvPB(RequestNewFZUser);
DeclareRcvPB(SyncFZUserList);
DeclareRcvPB(FZUserMessage);
DeclareRcvPB(RequestFriends);
DeclareRcvPB(UpdateSWKey);
DeclareRcvPB(ReqSWKeyInfo);
DeclareRcvPB(SWRegist);
DeclareRcvPB(PostFZResult);
DeclareRcvPB(RequestFZResults);
DeclareRcvPB(AckFZResults);
DeclareRcvPB(PostFZInfo);
DeclareRcvPB(RequestFZInfo);
DeclareRcvPB(PostGetFZPswd);
DeclareRcvPB(PostChangeFZPswd);