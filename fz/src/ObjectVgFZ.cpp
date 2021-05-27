#include "ObjectVgFZ.h"
#include "socketBase.h"
#include "das.pb.h"
#include "ProtoMsg.h"
#include "Messages.h"
#include "Utility.h"
#include "IMessage.h"
#include "DBMessages.h"
#include "VgFZManager.h"
#include "DBExecItem.h"

enum {
    WRITE_BUFFLEN = 1024 * 8,
    MaxSend = 3 * 1024,
    MAXLANDRECORDS = 200,
    MAXPLANRECORDS = 200,
};

using namespace das::proto;
using namespace google::protobuf;
using namespace std;
#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif
////////////////////////////////////////////////////////////////////////////////
//ObjectUav
////////////////////////////////////////////////////////////////////////////////
ObjectVgFZ::ObjectVgFZ(const std::string &id, int seq): ObjectAbsPB(id)
, m_auth(1), m_bInitFriends(false), m_seq(seq), m_bExist(false)
{
    SetBuffSize(WRITE_BUFFLEN);
}

ObjectVgFZ::~ObjectVgFZ()
{
}

void ObjectVgFZ::OnConnected(bool bConnected)
{
    if (bConnected)
        SetSocketBuffSize(WRITE_BUFFLEN);
    else if (!m_check.empty())
        Release();
}

void ObjectVgFZ::SetPswd(const std::string &pswd)
{
    m_pswd = pswd;
}

const std::string &ObjectVgFZ::GetPswd() const
{
    return m_pswd;
}

void ObjectVgFZ::SetAuth(int a)
{
    m_auth = a;
}

int ObjectVgFZ::Authorize() const
{
    return m_auth;
}

bool ObjectVgFZ::GetAuth(GSAuthorizeType auth) const
{
    return (auth & m_auth) == auth;
}

int ObjectVgFZ::FZType()
{
    return IObject::VgFZ;
}

int ObjectVgFZ::GetObjectType() const
{
    return FZType();
}

void ObjectVgFZ::_prcsLogin(RequestFZUserIdentity *msg)
{
    if (msg && m_p)
    {
        bool bSuc = m_pswd == msg->pswd();
        OnLogined(bSuc);
        if (auto ack = new AckFZUserIdentity)
        {
            ack->set_seqno(msg->seqno());
            ack->set_result(bSuc ? 1 : -1);
            WaitSend(ack);
        }
    }
}

void ObjectVgFZ::_prcsHeartBeat(das::proto::AckHeartBeat *)
{
}

void ObjectVgFZ::ProcessMessage(IMessage *msg)
{
    int tp = msg->GetMessgeType();
    if(m_p)
    {
        switch (tp)
        {
        case IMessage::User2User:
        case IMessage::User2UserAck:
            processFZ2FZ(*(Message*)msg->GetContent(), tp);
            break;
        case IMessage::SyncDeviceis:
            ackSyncDeviceis();
            break;
            break;
        case IMessage::UserInsertRslt:
            processGSInsert(*(DBMessage*)msg);
            break;
        case IMessage::UserQueryRslt:
            processGSInfo(*(DBMessage*)msg);
            break;
        case IMessage::UserCheckRslt:
            processCheckUser(*(DBMessage*)msg);
            break;
        case IMessage::FriendQueryRslt:
            processFriends(*(DBMessage*)msg);
            break;
        default:
            break;
        }
    }
}

void ObjectVgFZ::PrcsProtoBuff(uint64_t ms)
{
    if (!m_p)
        return;

    string strMsg = m_p->GetMsgName();
    if (strMsg == d_p_ClassName(RequestFZUserIdentity))
        _prcsLogin((RequestFZUserIdentity*)m_p->GetProtoMessage());
    else if (strMsg == d_p_ClassName(RequestNewFZUser))
        _prcsReqNewFz((RequestNewFZUser*)m_p->GetProtoMessage());
    else if (strMsg == d_p_ClassName(AckHeartBeat))
        _prcsHeartBeat((AckHeartBeat *)m_p->GetProtoMessage());
    else if (strMsg == d_p_ClassName(FZUserMessage))
        _prcsFzMessage((FZUserMessage *)m_p->GetProtoMessage());
    else if (strMsg == d_p_ClassName(SyncFZUserList))
        _prcsSyncDeviceList((SyncFZUserList *)m_p->GetProtoMessage());
    else if (strMsg == d_p_ClassName(RequestFriends))
        _prcsReqFriends((RequestFriends *)m_p->GetProtoMessage());

    if (m_stInit == IObject::Initialed)
        m_tmLastInfo = ms;
}

void ObjectVgFZ::processFZ2FZ(const Message &msg, int tp)
{
    if (tp == IMessage::User2User)
    {
        auto gsmsg = (const FZUserMessage *)&msg;
        if (FZ2FZMessage *ms = new FZ2FZMessage(this, gsmsg->from()))
        {
            auto ack = new AckFZUserMessage;
            ack->set_seqno(gsmsg->seqno());
            ack->set_res(1);
            ack->set_user(gsmsg->to());
            ms->AttachProto(ack);
            SendMsg(ms);
        }

        if (gsmsg->type() == DeleteFriend)
            m_friends.remove(gsmsg->from());
        else if (gsmsg->type() == AgreeFriend && !IsContainsInList(m_friends, gsmsg->from()))
            m_friends.push_back(gsmsg->from());
    }

    CopyAndSend(msg);
}

void ObjectVgFZ::processGSInfo(const DBMessage &msg)
{
    m_stInit = msg.GetRead(EXECRSLT).ToBool() ? Initialed : InitialFail;
    string pswd = msg.GetRead("pswd").ToString();
    m_auth = msg.GetRead("auth").ToInt32();
    bool bLogin = pswd == m_pswd && !pswd.empty();
    OnLogined(bLogin);
    if (auto ack = new AckFZUserIdentity)
    {
        ack->set_seqno(m_seq);
        ack->set_result(bLogin ? 1 : -1);
        WaitSend(ack);
    }
    m_seq = -1;
    m_pswd = pswd;
    if (Initialed == m_stInit)
        initFriend();
    else if (Initialed != m_stInit)
        Release();
}

void ObjectVgFZ::processGSInsert(const DBMessage &msg)
{
    bool bSuc = msg.GetRead(EXECRSLT).ToBool();
    if (auto ack = new AckNewFZUser)
    {
        ack->set_seqno(msg.GetSeqNomb());
        ack->set_result(bSuc ? 1 : -1);
        WaitSend(ack);
    }
    if (!bSuc)
        m_stInit = IObject::Uninitial;

    m_auth = 0x100;
    m_tmLastInfo = Utility::msTimeTick();
}

void ObjectVgFZ::processFriends(const DBMessage &msg)
{
    const Variant &vUsr1 = msg.GetRead("usr1");
    const Variant &vUsr2 = msg.GetRead("usr2");
    if (vUsr1.GetType() == Variant::Type_StringList && vUsr2.GetType() == Variant::Type_StringList)
    {
        for (const Variant &itr : vUsr1.GetVarList())
        {
            if (!itr.IsNull() && itr.ToString() != m_id)
                m_friends.push_back(itr.ToString());
        }
        for (const Variant &itr : vUsr2.GetVarList())
        {
            if (!itr.IsNull() && itr.ToString() != m_id)
                m_friends.push_back(itr.ToString());
        }
    }
}

void ObjectVgFZ::processCheckUser(const DBMessage &msg)
{
    m_bExist = msg.GetRead("count(*)").ToInt32()>0;
    if (auto ack = new AckNewFZUser)
    {
        ack->set_seqno(msg.GetSeqNomb());
        m_check = ExecutItem::GenCheckString();
        ack->set_check(m_check);
        ack->set_result(m_bExist ? 0 : 1);
        WaitSend(ack);
    }
}

void ObjectVgFZ::InitObject()
{
    if (IObject::Uninitial == m_stInit)
    {
        if (!m_check.empty())
        {
            m_stInit = IObject::Initialed;
            return;
        }

        DBMessage *msg = new DBMessage(this, IMessage::UserQueryRslt);
        if (!msg)
            return;

        if (m_check.empty())
        {
            msg->SetSql("queryFZInfo");
            msg->SetCondition("user", m_id);
            SendMsg(msg);
            m_stInit = IObject::Initialing;
            return;
        }
        m_stInit = IObject::Initialed;
    }
}

void ObjectVgFZ::CheckTimer(uint64_t ms, char *buf, int len)
{
    ObjectAbsPB::CheckTimer(ms, buf, len);
    ms -= m_tmLastInfo;
    if (m_auth > Type_ALL && ms > 50)
        Release();
    else if (m_bExist && ms > 100)
        Release();
    else if (ms > 600000 || (!m_check.empty() && ms > 60000))
        Release();
    else if (m_check.empty() && ms > 10000)//³¬Ê±¹Ø±Õ
        CloseLink();   
}

bool ObjectVgFZ::IsAllowRelease() const
{
    return !GetAuth(ObjectVgFZ::Type_UavManager);
}

void ObjectVgFZ::FreshLogin(uint64_t ms)
{
    m_tmLastInfo = ms;
}

void ObjectVgFZ::SetCheck(const std::string &str)
{
    m_check = str;
}

void ObjectVgFZ::_prcsSyncDeviceList(SyncFZUserList *ms)
{
    if (ms && GetAuth(ObjectVgFZ::Type_UavManager))
    {
        m_seq = ms->seqno();
        if (auto syn = new ObjectSignal(this, FZType(), IMessage::SyncDeviceis, GetObjectID()))
            SendMsg(syn);
    }
    else if (ms)
    {
        if (auto ack = new AckSyncFZUserList)
        {
            ack->set_seqno(m_seq);
            ack->set_result(0);
            WaitSend(ack);
        }
    }
}

int ObjectVgFZ::_addDatabaseUser(const std::string &user, const std::string &pswd, int seq)
{
    VgFZManager *mgr = (VgFZManager *)(GetManager());
    int ret = -1;
    if (mgr)
        ret = mgr->AddDatabaseUser(user, pswd, this, seq) > 0 ? 2 : -1;

    if (ret > 0)
        m_pswd = pswd;

    return ret;
}

void ObjectVgFZ::ackSyncDeviceis()
{
    auto mgr = (VgFZManager *)GetManager();
    AckSyncFZUserList ack;
    ack.set_seqno(m_seq);
    ack.set_result(1);
    int i = 0;
    for (const string &itr : mgr->Uavs())
    {
        if (++i < 100)
        {
            ack.add_id(itr);
            continue;
        }
        CopyAndSend(ack);
        ack.clear_id();
        i = 0;
    }
    if (ack.id_size() > 0)
        CopyAndSend(ack);

    m_seq = -1;
}

void ObjectVgFZ::initFriend()
{
    if (m_bInitFriends)
        return;

    DBMessage *msg = new DBMessage(this, IMessage::FriendQueryRslt);
    if (!msg)
        return;
    m_bInitFriends = true;
    msg->SetSql("queryFZFriends", true);
    msg->SetCondition("usr1", m_id);
    msg->SetCondition("usr2", m_id);
    SendMsg(msg);
}

void ObjectVgFZ::addDBFriend(const string &user1, const string &user2)
{
    if (DBMessage *msg = new DBMessage(this, IMessage::Unknown))
    {
        msg->SetSql("insertFZFriends");
        msg->SetWrite("id", VgFZManager::CatString(user1, user2));
        msg->SetWrite("usr1", user1);
        msg->SetWrite("usr2", user2);
        SendMsg(msg);
    }
}

void ObjectVgFZ::_prcsReqNewFz(RequestNewFZUser *msg)
{
    if (!msg)
        return;

    int res = 1;
    string user = Utility::Lower(msg->user());
    if (user != m_id)
        res = -4;
    else if (!GSOrUavMessage::IsGSUserValide(user))
        res = -2;
    else if (msg->check().empty())
        return _checkFZ(m_id, msg->seqno());
    else if (msg->check() != m_check)
        res = -3;
    else if (msg->check() == m_check && !msg->password().empty())
        res = _addDatabaseUser(m_id, msg->password(), msg->seqno());

    if (res <= 1)
    {
        if (auto ack = new AckNewFZUser)
        {
            ack->set_seqno(msg->seqno());
            if (1 != res || msg->check().empty())
            {
                m_check = ExecutItem::GenCheckString();
                ack->set_check(-3 == res ? m_check : "");
            }
            ack->set_result(res);
            WaitSend(ack);
        }
    }
}

void ObjectVgFZ::_prcsFzMessage(FZUserMessage *msg)
{
    if (!msg)
        return;

    if (msg->type()>RejectFriend && !IsContainsInList(m_friends, msg->to()))
    {
        if (auto ack = new AckFZUserMessage)
        {
            ack->set_seqno(msg->seqno());
            ack->set_res(-1);
            WaitSend(ack);
        }
        return;
    }
    if (msg->type() == AgreeFriend && !IsContainsInList(m_friends, msg->to()))
    {
        m_friends.push_back(msg->to());
        addDBFriend(m_id, msg->to());
    }
    else if (msg->type() == DeleteFriend)
    {
        m_friends.remove(msg->to());
        if (DBMessage *db = new DBMessage(this))
        {
            db->SetSql("deleteFZFriends");
            db->SetCondition("id", VgFZManager::CatString(m_id, msg->to()));
            SendMsg(db);
        }
    }

    if (FZ2FZMessage *ms = new FZ2FZMessage(this, msg->to()))
    {
        ms->AttachProto(msg);
        SendMsg(ms);
    }
}

void ObjectVgFZ::_prcsReqFriends(das::proto::RequestFriends *msg)
{
    if (!msg)
        return;

    if (auto ack = new AckFriends)
    {
        ack->set_seqno(msg->seqno());
        for (const string &itr : m_friends)
        {
            ack->add_friends(itr);
        }
        WaitSend(ack);
    }
}

void ObjectVgFZ::_checkFZ(const string &user, int ack)
{
    DBMessage *msgDb = new DBMessage(this, IMessage::UserCheckRslt);
    if (!msgDb)
        return;

    msgDb->SetSeqNomb(ack);
    msgDb->SetSql("checkFZ");
    msgDb->SetCondition("user", user);
    SendMsg(msgDb);
}
