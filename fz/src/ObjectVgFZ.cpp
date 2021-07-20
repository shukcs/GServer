#include "ObjectVgFZ.h"
#include "socketBase.h"
#include "das.pb.h"
#include "ProtoMsg.h"
#include "Messages.h"
#include "Utility.h"
#include "MD5lib.h"
#include "IMessage.h"
#include "DBMessages.h"
#include "VgFZManager.h"
#include "DBExecItem.h"

#define MD5KEY "$VIGA-CHANGSHA802"

enum {
    WRITE_BUFFLEN = 1024 * 8,
    MaxSend = 3 * 1024,
    MAXLANDRECORDS = 200,
    MAXPLANRECORDS = 200,
};

using namespace das::proto;
using namespace google::protobuf;
using namespace std;
////////////////////////////////////////////////////////////////////////////////
//ObjectUav
////////////////////////////////////////////////////////////////////////////////
ObjectVgFZ::ObjectVgFZ(const std::string &id, int seq): ObjectAbsPB(id)
, m_auth(Type_Common), m_bInitFriends(false), m_seq(seq), m_ver(-1)
, m_tmSpace(-1)
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

int ObjectVgFZ::GetVer() const
{
    return m_ver;
}

bool ObjectVgFZ::GetAuth(AuthorizeType auth) const
{
    return (auth & m_auth) == auth;
}

int ObjectVgFZ::FZType()
{
    return IObject::VgFZ;
}

bool ObjectVgFZ::CheckSWKey(const std::string &swk)
{
    auto ls = Utility::SplitString(swk, "-");
    if (ls.size() != 2 || ls.front().length() != 8 || ls.back().length() != 16)
        return false;

    static const char *sLTab = "0123456789QWERTYUIOPASDFGHJKLZXCVBNM";
    auto md5 = MD5::MD5DigestString((ls.front() + MD5KEY).c_str());
    char tmp[17] = {0};
    auto strMd5 = (const uint8_t *)md5.c_str();
    for (int i = 0; i < 16; ++i)
    {
        tmp[i] = sLTab[strMd5[i] % 36];
    }
    return ls.back() == tmp;
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
        if (bSuc)
        {
            SetPcsn(msg->pcsn());
        }
        else if (auto ack = new AckFZUserIdentity)
        {
            ack->set_seqno(msg->seqno());
            ack->set_result(-1);
            ack->set_swver(-1);
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
        case IMessage::UserInsertRslt:
            processFZInsert(*(DBMessage*)msg);
            break;
        case IMessage::UserQueryRslt:
            processFZInfo(*(DBMessage*)msg);
            break;
        case IMessage::UserCheckRslt:
            processCheckUser(*(DBMessage*)msg);
            break;
        case IMessage::FriendQueryRslt:
            processFriends(*(DBMessage*)msg);
            break;
        case IMessage::InserSWSNRslt:
            processInserSWSN(*(DBMessage*)msg);
            break;
        case IMessage::UpdateSWSNRslt:
            processSWRegist(*(DBMessage*)msg);
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
    else if (strMsg == d_p_ClassName(AddSWKey))
        _prcsAddSWKey((AddSWKey *)m_p->GetProtoMessage());
    else if (strMsg == d_p_ClassName(SWRegist))
        _prcsSWRegist((SWRegist *)m_p->GetProtoMessage());

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

void ObjectVgFZ::processFZInfo(const DBMessage &msg)
{
    bool bLogin = true;
    if (!msg.GetRead("pswd").IsNull())
    {
        m_stInit = msg.GetRead(EXECRSLT).ToBool() ? Initialed : ReleaseLater;
        string pswd = msg.GetRead("pswd").ToString();
        bLogin = pswd == m_pswd && !pswd.empty();
        m_pswd = pswd;
    }
    else if (Initialed != m_stInit)
    {
        bLogin = false;
    }

    auto var = msg.GetRead("ver", 1);
    m_ver = var.IsNull() ? -1 : var.ToInt32();
    AckFZUserIdentity ack;
    ack.set_seqno(m_seq);
    ack.set_result(bLogin ? 1 : -1);
    ack.set_swver(m_ver);
    ObjectAbsPB::SendProtoBuffTo(GetSocket(), ack);
    if (m_stInit==Initialed)
        OnLogined(bLogin);

    m_seq = 0;
    m_tmLastInfo = Utility::msTimeTick();
    if (Initialed == m_stInit)
        initFriend();
}

void ObjectVgFZ::processFZInsert(const DBMessage &msg)
{
    bool bSuc = msg.GetRead(EXECRSLT).ToBool();
    if (auto ack = new AckNewFZUser)
    {
        ack->set_seqno(msg.GetSeqNomb());
        ack->set_result(bSuc ? 1 : -1);
        WaitSend(ack);
    }

    if (!m_check.empty())
        m_stInit = ReleaseLater;

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

void ObjectVgFZ::processInserSWSN(const DBMessage &msg)
{
    bool bSuc = msg.GetRead(EXECRSLT).ToBool();
    if (auto ack = new AckAddSWKey)
    {
        ack->set_seqno(msg.GetSeqNomb());
        ack->set_result(bSuc ? 1 : 0);
        WaitSend(ack);
    }
}

void ObjectVgFZ::processSWRegist(const DBMessage &msg)
{
    bool bSuc = msg.GetRead(EXECRSLT).ToBool();
    if (auto ack = new AckSWRegist)
    {
        ack->set_seqno(msg.GetSeqNomb());
        ack->set_result(bSuc ? 1 : 0);
        WaitSend(ack);
    }

}

void ObjectVgFZ::processCheckUser(const DBMessage &msg)
{
    bool bExist = msg.GetRead("count(*)").ToInt32()>0;
    if (auto ack = new AckNewFZUser)
    {
        ack->set_seqno(msg.GetSeqNomb());
        m_check = Utility::RandString();
        ack->set_check(m_check);
        ack->set_result(bExist ? 0 : 1);
        WaitSend(ack);
    }
    if (bExist)
        m_stInit = IObject::ReleaseLater;
}

void ObjectVgFZ::InitObject()
{
    if (IObject::Uninitial == m_stInit)
    {
        if (m_check.empty())
        {
            DBMessage *msg = new DBMessage(this, IMessage::UserQueryRslt);
            if (!msg)
                return;

            msg->SetSql("queryFZInfo");
            msg->SetCondition("user", m_id);
            if (!m_pcsn.empty())
            {
                msg->SetSql("queryFZPCReg");
                msg->SetCondition("pcsn", m_pcsn);
            }
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
    if (ReleaseLater == m_stInit && ms > 100)
        Release();
    else if (!GetAuth(Type_UserManager) && (ms > 600000 || (!m_check.empty() && ms > 60000)))
        Release();
    else if (m_check.empty() && ms > 10000)//³¬Ê±¹Ø±Õ
        IsInitaled() ? CloseLink() : Release();
    else
        _sendHeartBeat(ms);
}

bool ObjectVgFZ::IsAllowRelease() const
{
    return !GetAuth(ObjectVgFZ::Type_UserManager);
}

void ObjectVgFZ::FreshLogin(uint64_t ms)
{
    m_tmLastInfo = ms;
}

void ObjectVgFZ::SetCheck(const std::string &str)
{
    m_check = str;
}

void ObjectVgFZ::SetPcsn(const std::string &str)
{
    if (!str.empty() && str!=m_pcsn)
    {
        m_pcsn = str;
        if (!GetAuth(Type_UserManager))
        {
            m_ver = -1;
            if (Initialed == m_stInit)
            {
                DBMessage *msg = new DBMessage(this, IMessage::UserQueryRslt);
                if (!msg)
                    return;

                msg->SetSql("queryFZPCReg");
                msg->SetCondition("pcsn", m_pcsn);
                SendMsg(msg);
                m_stInit = IObject::Initialing;
            }
        }
    }
}

const std::string & ObjectVgFZ::GetPCSn() const
{
    return m_pcsn;
}

void ObjectVgFZ::_prcsSyncDeviceList(SyncFZUserList *ms)
{
    if (ms && GetAuth(ObjectVgFZ::Type_UserManager))
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
                m_check = Utility::RandString();
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

void ObjectVgFZ::_prcsAddSWKey(AddSWKey *pb)
{
    if (!pb)
        return;

    int ret = !GetAuth(ObjectVgFZ::Type_UserManager) ? -1 : (!CheckSWKey(pb->swkey()) ? -2 : 0);
    if (ret < 0)
    {
        if (auto ack = new AckAddSWKey)
        {
            ack->set_seqno(m_seq++);
            ack->set_result(ret);
            WaitSend(ack);
        }
        return;
    }
    DBMessage *msg = new DBMessage(this, IMessage::InserSWSNRslt);
    if (!msg)
        return;

    msg->SetSeqNomb(pb->seqno());
    msg->SetSql("insertFZKey");
    msg->SetWrite("swsn", pb->swkey());
    if (pb->has_ver())
        msg->SetWrite("ver", Utility::l2string(pb->ver()));
    SendMsg(msg);
}

void ObjectVgFZ::_prcsSWRegist(SWRegist *pb)
{
    if (!pb)
        return;

    DBMessage *msg = new DBMessage(this, IMessage::UpdateSWSNRslt);
    if (!msg)
        return;

    msg->SetSql("updateFZPCReg");
    msg->SetSeqNomb(pb->seqno());
    msg->SetWrite("pcsn", pb->pcsn());
    msg->SetCondition("swsn", pb->swkey());
    SendMsg(msg);
}

void ObjectVgFZ::_sendHeartBeat(uint64_t ms)
{
    if (ms - m_tmSpace < 5000)
        return;

    m_tmSpace = ms;
    if (PostHeartBeat *hb = new PostHeartBeat)
    {
        hb->set_seqno(m_seq++);
        WaitSend(hb);
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
