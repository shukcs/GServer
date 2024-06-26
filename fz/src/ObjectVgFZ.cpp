﻿#include "ObjectVgFZ.h"
#include "net/socketBase.h"
#include "das.pb.h"
#include "ProtoMsg.h"
#include "Messages.h"
#include "common/Utility.h"
#include "common/MD5lib.h"
#include "IMessage.h"
#include "DBMessages.h"
#include "VgFZManager.h"
#include "DBExecItem.h"
#include "MailMessages.h"

#define MD5KEY "$VIGA-CHANGSHA802"
#define AutoReleaseMs 60000

enum {
    WRITE_BUFFLEN = 1024 * 8,
    MaxSend = 3 * 1024,
    MAXLANDRECORDS = 200,
    MAXPLANRECORDS = 200,
};

using namespace das::proto;
using namespace google::protobuf;
using namespace std;

static bool trans2TestInfo(TestInfo *tInfo, const string &id, const string &info)
{
    if (!tInfo || id.empty())
        return false;

    tInfo->set_id(id);

    auto strLs = Utility::SplitString(info, "$#$", false);
    tInfo->set_name(!strLs.empty() ? strLs.front() : string());
    if (!strLs.empty())
        strLs.pop_front();
    tInfo->set_sex(!strLs.empty() ? strLs.front() : string());
    if (!strLs.empty())
        strLs.pop_front();
    tInfo->set_birthday(!strLs.empty() ? strLs.front() : string());
    if (!strLs.empty())
        strLs.pop_front();
    tInfo->set_address(!strLs.empty() ? strLs.front() : string());
    if (!strLs.empty())
        strLs.pop_front();
    tInfo->set_nationality(!strLs.empty() ? strLs.front() : string());
    if (!strLs.empty())
        strLs.pop_front();
    tInfo->set_email(!strLs.empty() ? strLs.front() : string());
    if (!strLs.empty())
        strLs.pop_front();
    tInfo->set_examiner(!strLs.empty() ? strLs.front() : string());
    if (!strLs.empty())
        strLs.pop_front();
    tInfo->set_phone_number(!strLs.empty() ? strLs.front() : string());

    return true;
}              

static string packTestInfo(const TestInfo &tInfo)
{
    string ret = tInfo.has_name() ? tInfo.name() : string();
    ret += "$#$";
    if (tInfo.has_sex())
        ret += tInfo.sex();
    ret += "$#$";
    if (tInfo.has_address())
        ret += tInfo.address();
    ret += "$#$";
    if (tInfo.has_birthday())
        ret += tInfo.birthday();
    ret += "$#$";
    if (tInfo.has_nationality())
        ret += tInfo.nationality();
    ret += "$#$";
    if (tInfo.has_email())
        ret += tInfo.email();
    ret += "$#$";
    if (tInfo.has_examiner())
        ret += tInfo.examiner();
    ret += "$#$";
    if (tInfo.has_phone_number())
        ret += tInfo.phone_number();

    return ret;
}
////////////////////////////////////////////////////////////////////////////////
//ObjectUav
////////////////////////////////////////////////////////////////////////////////
ObjectVgFZ::ObjectVgFZ(const std::string &id, int seq): ObjectAbsPB(id)
, m_auth(Type_Common), m_seq(seq), m_ver(0), m_bInitFriends(false)
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
	else if (GetAuth(IObject::Type_ReqNewUser))
		ReleaseObject();
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

uint64_t ObjectVgFZ::GetVer() const
{
    return m_ver;
}

bool ObjectVgFZ::GetAuth(AuthorizeType auth) const
{
    return (auth & m_auth) == auth;
}

uint64_t ObjectVgFZ::GetCurVer(const std::string &pcsn)
{
    if (GetAuth(IObject::Type_Manager))
        return -1;

    return pcsn != m_pcsn ? 0 : m_ver;
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
    if (msg)
    {
        bool bSuc = m_pswd == msg->pswd();
        SetLogined(bSuc);
        if (bSuc)
        {
            SetPcsn(msg->pcsn());
        }
        else if (auto ack = new AckFZUserIdentity)
        {
            ack->set_seqno(msg->seqno());
            ack->set_result(-1);
            ack->set_ver(0);
            WaitSend(ack);
        }
    }
}

void ObjectVgFZ::_prcsHeartBeat(das::proto::PostHeartBeat *hb)
{
    if (!hb)
        return;

    if (auto ack = new AckHeartBeat)
    {
        ack->set_seqno(hb->seqno());
        WaitSend(ack);
    }
}

void ObjectVgFZ::ProcessMessage(const IMessage *msg)
{
    int tp = msg->GetMessgeType();
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
    case IMessage::UpdateSWKeysRslt:
        processInserSWSN(*(DBMessage*)msg);
        break;
    case IMessage::SWRegistRslt:
        processSWRegist(*(DBMessage*)msg);
        break;
    case IMessage::QuerySWKeyInfoRslt:
        processQuerySWKey(*(DBMessage*)msg);
        break;
    case IMessage::InserFZRslt:
        processAckFZRslt(*(DBMessage*)msg);
        break;
    case IMessage::QueryFZRslt:
        processAckFZRslts(*(DBMessage*)msg);
        break;
    case IMessage::MailRslt:
        processMailRslt(*(MailRsltMessage*)msg);
        break;
    default:
        break;
    }
}

bool ObjectVgFZ::ProcessRcvPack(const void *pack)
{
    auto proto = (Message*)pack;
    const string &strMsg = proto->GetDescriptor()->full_name();

    if (strMsg == d_p_ClassName(RequestFZUserIdentity))
        _prcsLogin((RequestFZUserIdentity*)proto);
    else if (strMsg == d_p_ClassName(RequestNewFZUser))
        _prcsReqNewFz((RequestNewFZUser*)proto);
    else if (strMsg == d_p_ClassName(PostHeartBeat))
        _prcsHeartBeat((PostHeartBeat *)proto);
    else if (strMsg == d_p_ClassName(FZUserMessage))
        _prcsFzMessage((FZUserMessage *)proto);
    else if (strMsg == d_p_ClassName(SyncFZUserList))
        _prcsSyncDeviceList((SyncFZUserList *)proto);
    else if (strMsg == d_p_ClassName(RequestFriends))
        _prcsReqFriends((RequestFriends *)proto);
    else if (strMsg == d_p_ClassName(UpdateSWKey))
        _prcsUpdateSWKey((UpdateSWKey *)proto);
    else if (strMsg == d_p_ClassName(ReqSWKeyInfo))
        _prcsReqSWKeyInfo((ReqSWKeyInfo *)proto);
    else if (strMsg == d_p_ClassName(SWRegist))
        _prcsSWRegist((SWRegist*)proto);
    else if (strMsg == d_p_ClassName(PostFZResult))
        _prcsPostFZResult((PostFZResult*)proto);
    else if (strMsg == d_p_ClassName(RequestFZResults))
        _prcsRequestFZResults(*(RequestFZResults*)proto);
    else if (strMsg == d_p_ClassName(PostFZInfo))
        _prcsPostFZInfo(*(PostFZInfo *)proto);
    else if (strMsg == d_p_ClassName(RequestFZInfo))
        _prcsRequestFZInfo(*(RequestFZInfo *)proto);
    else if (strMsg == d_p_ClassName(PostGetFZPswd))
        _prcsPostGetFZPswd(*(PostGetFZPswd *)proto);
    else if (strMsg == d_p_ClassName(PostChangeFZPswd))
        _prcsPostChangeFZPswd(*(PostChangeFZPswd *)proto);

    return IsInitaled();
}

void ObjectVgFZ::processFZ2FZ(const Message &msg, int tp)
{
    if (tp == IMessage::User2User)
    {
        auto gsmsg = (const FZUserMessage *)&msg;
        if (FZ2FZMessage *ms = new FZ2FZMessage(*this, gsmsg->from()))
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
        else if (gsmsg->type() == AgreeFriend && !Utility::IsContainsInList(m_friends, gsmsg->from()))
            m_friends.push_back(gsmsg->from());
    }

    CopyAndSend(msg);
}

void ObjectVgFZ::processFZInfo(const DBMessage &msg)
{
    int idx = msg.IndexofSql("queryFZPCReg");
    if (idx >= 0)
    {
        m_ver = msg.GetRead("ver", idx).ToInt64();
        if (m_pcsn.empty())
            m_ver = 0;

        if (m_ver == 0)
            m_pcsn = string();
    }

    if (GetAuth(IObject::Type_GetPswd))
        forGetPswd(msg);
    else
        ackLogin(msg);

    m_seq = 0;
    m_tmLastInfo = Utility::msTimeTick();
    if (!m_pswd.empty() && !GetAuth(IObject::Type_GetPswd))
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

	GetManager()->Log(0, GetObjectID(), 0, "(user: %s) register %s!"
		, GetObjectID().c_str(), bSuc?"success":"fail");

	SetAuth(Type_Common);
    m_check = string();
    m_tmLastInfo = Utility::msTimeTick();
	CloseLink();
	if(!bSuc)
		ReleaseObject();
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
    if (auto ack = new AckUpdateSWKey)
    {
        ack->set_seqno(msg.GetSeqNomb());
        ack->set_result(bSuc ? 1 : 0);
        ack->set_swkey(msg.GetRead("swsn").ToString());
        WaitSend(ack);
    }
}

void ObjectVgFZ::processQuerySWKey(const DBMessage &msg)
{
    auto ack = new AckSWKeyInfo;
    if (!ack)
        return;
    ack->set_seqno(msg.GetSeqNomb());
    const StringList &keys = msg.GetRead("swsn").ToStringList();
    const StringList &pcsns = msg.GetRead("pcsn").ToStringList();
    const StringList &dscrs = msg.GetRead("dscr").ToStringList();
    const VariantList &vers = msg.GetRead("ver").GetVarList();
    const VariantList &useds = msg.GetRead("used").GetVarList(); 
    const VariantList &regTms = msg.GetRead("regTm").GetVarList();
    const VariantList &genTms = msg.GetRead("genTm").GetVarList();

    auto pcsn = pcsns.begin();
    auto dscr = dscrs.begin();
    auto verItr = vers.begin();
    auto usedItr = useds.begin();
    auto regTmItr = regTms.begin();
    auto genTmItr = genTms.begin();
    for (const string &key : keys)
    {
        if (ack->ByteSize() > MaxSend)
        {
            WaitSend(ack);
            ack = new AckSWKeyInfo;
            if (!ack)
                break;
            ack->set_seqno(msg.GetSeqNomb());
        }

        auto swKey = ack->add_keys();
        if (!swKey)
            break;
        swKey->set_swkey(key);
        if (!pcsn->empty())
            swKey->set_pcsn(*pcsn);
        pcsn++;
        if (!dscr->empty())
            swKey->set_dscr(*dscr);
        dscr++;

        swKey->set_gentm(genTmItr++->ToInt64());
        swKey->set_ver(verItr++->ToInt64());
        swKey->set_used(usedItr++->ToInt32());
        if (regTmItr->IsValid())
            swKey->set_regtm(regTmItr->ToInt64());
        regTmItr++;
    }

    if (ack)
        WaitSend(ack);
}

void ObjectVgFZ::processSWRegist(const DBMessage &msg)
{
    bool bSuc = msg.GetRead(EXECRSLT).ToBool();
    if (auto ack = new AckSWRegist)
    {
        auto idx = msg.IndexofSql("queryFZPCReg");
        auto ver = msg.GetRead("ver", idx).ToInt64();
        ack->set_seqno(msg.GetSeqNomb());
        ack->set_result(bSuc ? 1 : 0);
        if (bSuc)
            m_ver = ver;

        ack->set_ver(m_ver);
        GetManager()->Log(0, IObjectManager::GetObjectFlagID(this), 0, "PC %s register %s", m_pcsn.c_str(), bSuc? "success":"fail");
        WaitSend(ack);
    }

    if (!bSuc)
    {
        m_ver = 0;
        m_pcsn = string();
    }
}

void ObjectVgFZ::processAckFZRslt(const DBMessage &msg)
{
    if (auto ack = new AckPostFZResult)
    {
        ack->set_seqno(msg.GetSeqNomb());
        ack->set_id(msg.GetRead(INCREASEField).ToInt64());
        WaitSend(ack);
    }
}

void ObjectVgFZ::processAckFZRslts(const DBMessage &msg)
{
    auto ack = new AckFZResults;
    if (!ack)
        return;

    ack->set_seqno(msg.GetSeqNomb());
    const VariantList &ids = msg.GetRead("id").GetVarList();
    const VariantList &begTms = msg.GetRead("begTm").GetVarList();
    const VariantList &usedTms = msg.GetRead("usedTm").GetVarList();
    const VariantList &types = msg.GetRead("type").GetVarList();
    const VariantList &results = msg.GetRead("result").GetVarList();
    const VariantList &angVaxs = msg.GetRead("angvax").GetVarList();
    const VariantList &altVaxs = msg.GetRead("altvax").GetVarList();
    const VariantList &horVaxs = msg.GetRead("horvax").GetVarList();
    const VariantList &maxVels = msg.GetRead("maxvel").GetVarList();
    const VariantList &minVels = msg.GetRead("minvel").GetVarList();
    const StringList &uids = msg.GetRead("uid").ToStringList();
    const StringList &infos = msg.GetRead("info").ToStringList();

    auto begItr = begTms.begin();
    auto usedItr = usedTms.begin();
    auto typeItr = types.begin();
    auto rsltItr = results.begin();
    auto uidItr = uids.begin();
    auto infoItr = infos.begin();
    auto angVaxItr = angVaxs.begin();
    auto altVaxItr = altVaxs.begin();
    auto horVaxItr = horVaxs.begin();
    auto maxVelItr = maxVels.begin();
    auto minVelItr = minVels.begin();
    for (const Variant &id : ids)
    {
        if (ack->ByteSize() > MaxSend)
        {
            WaitSend(ack);
            ack = new AckFZResults;
            if (!ack)
                break;
            ack->set_seqno(msg.GetSeqNomb());
        }

        auto result = ack->add_rslt();
        if (!result)
            break;
        result->set_id(id.ToInt64());
        result->set_begtm(begItr++->ToInt64());
        result->set_usedtm(usedItr++->ToInt32());
        result->set_type(typeItr++->ToInt32());
        result->set_rslt(rsltItr++->ToInt32());
        result->set_angvax(angVaxItr++->ToFloat());
        result->set_altvax(altVaxItr++->ToFloat());
        result->set_horvax(horVaxItr++->ToFloat());
        result->set_maxvel(maxVelItr++->ToFloat());
        result->set_minvel(minVelItr++->ToFloat());
        if (!uidItr->empty())
        {
            if (!trans2TestInfo(result->mutable_info(), *uidItr, *infoItr))
                delete result->release_info();
        }
        ++uidItr;
        ++infoItr;
    }

    if (ack)
        WaitSend(ack);
}

void ObjectVgFZ::processMailRslt(const MailRsltMessage &msg)
{
    if (auto ack = new AckGetFZPswd)
    {
        ack->set_seqno(msg.GetSeq());
        ack->set_rslt(msg.GetErrCode().empty() ? 1 : 0);
        WaitSend(ack);
    }
    ReleaseObject();
}

bool ObjectVgFZ::ackLogin(const DBMessage &msg)
{
    bool bLogin = true;
    if (msg.IndexofSql("queryFZInfo") >= 0)
    {
        auto pswd = msg.GetRead("pswd").ToString();
        bLogin = pswd==m_pswd && !pswd.empty();

        m_pswd = pswd;
        m_email = msg.GetRead("email").ToString();
        m_info = msg.GetRead("info").ToString();
    }

    SetLogined(bLogin);
    if (auto ack = new AckFZUserIdentity)
    {
        ack->set_seqno(m_seq);
        ack->set_result(bLogin ? 1 : -1);
        ack->set_ver(m_ver);
        WaitSend(ack);
    }
	if (m_pswd.empty()) ///不存在的FZ
		ReleaseObject();

    return bLogin;
}

void ObjectVgFZ::forGetPswd(const DBMessage &msg)
{
    string email = m_email;
	bool bOk = false;
    if (msg.IndexofSql("queryFZInfo") >= 0)
    {
        m_email = msg.GetRead("email").ToString();
        m_pswd = msg.GetRead("pswd").ToString();
        m_info = msg.GetRead("info").ToString();
		bOk = CheckMail(email);
    }
    
    if (!bOk)
    {
        if (auto ack = new AckGetFZPswd)
        {
            ack->set_seqno(m_seq);
            ack->set_rslt(-1);
            WaitSend(ack);
        }
        ReleaseObject();
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
	{
		GetManager()->Log(0, GetObjectID(), 0, "Exit (user: %s), register fail!"
			, GetObjectID().c_str());
		ReleaseObject();
	}
}

void ObjectVgFZ::InitObject()
{
    if (!IsInitaled())
    {
        if (!GetAuth(Type_ReqNewUser) && !GetAuth(Type_Manager))
		{
			if (DBMessage *msg = new DBMessage(*this, IMessage::UserQueryRslt))
			{
				msg->SetSql("queryFZInfo");
				msg->SetCondition("user", m_id);
				if (!GetAuth(Type_GetPswd) && (!m_pcsn.empty() || m_pcsn == "O.E.M"))
				{
					msg->AddSql("queryFZPCReg");
					msg->SetCondition("pcsn", m_pcsn, 1);
				}
				SendMsg(msg);
			}
        }   
		IObject::InitObject();
		StartTimer(500);
    }
}

void ObjectVgFZ::CheckTimer(uint64_t ms)
{
    ms -= m_tmLastInfo;

    if (!GetAuth(Type_Manager) && ms>AutoReleaseMs)
        ReleaseObject();
    else if (ms>15000 && IsLinked())//超时关闭
        CloseLink();
}

bool ObjectVgFZ::IsAllowRelease() const
{
    return !GetAuth(Type_Manager);
}

void ObjectVgFZ::RefreshRcv(int64_t ms)
{
    m_tmLastInfo = ms;
}

bool ObjectVgFZ::CheckMail(const std::string &str)
{
    if (!IsInitaled())
    {
        m_email = str;
        return false;
    }

    if (str != m_email || str.empty())
        return false;

    if (auto mailMsg = new MailMessage(*this, "hsj8262@163.com"))
    {
        mailMsg->SetSeq(m_seq);
        mailMsg->SetMailTo(m_email);
        mailMsg->SetTitle("AirSim user password");
        mailMsg->SetBody("User: " + GetObjectID() + "\r\nPassword: " + m_pswd + "\n");
        SendMsg(mailMsg);
    }
    return true;
}

void ObjectVgFZ::SetCheck(const std::string &str)
{
    m_check = str;
}

void ObjectVgFZ::SetPcsn(const std::string &str)
{
    if (!str.empty() && str!=m_pcsn)
    {
        if (!GetAuth(Type_Manager))
        {
            m_pcsn = str;// Utility::FindString(str, "O.E.M") >= 0 ? "O.E.M" : str;
            m_ver = 0;
            DBMessage *msg = new DBMessage(*this, IMessage::UserQueryRslt);
            if (!msg)
                return;

            msg->SetSql("queryFZPCReg");
            msg->SetCondition("pcsn", m_pcsn);
            SendMsg(msg);
        }
    }
}

const std::string & ObjectVgFZ::GetPCSn() const
{
    return m_pcsn;
}

void ObjectVgFZ::_prcsSyncDeviceList(SyncFZUserList *ms)
{
    if (ms && GetAuth(ObjectVgFZ::Type_Manager))
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

    DBMessage *msg = new DBMessage(*this, IMessage::FriendQueryRslt);
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
    if (DBMessage *msg = new DBMessage(*this, IMessage::Unknown))
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

    if (msg->type()>RejectFriend && !Utility::IsContainsInList(m_friends, msg->to()))
    {
        if (auto ack = new AckFZUserMessage)
        {
            ack->set_seqno(msg->seqno());
            ack->set_res(-1);
            WaitSend(ack);
        }
        return;
    }
    if (msg->type() == AgreeFriend && !Utility::IsContainsInList(m_friends, msg->to()))
    {
        m_friends.push_back(msg->to());
        addDBFriend(m_id, msg->to());
    }
    else if (msg->type() == DeleteFriend)
    {
        m_friends.remove(msg->to());
        if (DBMessage *db = new DBMessage(*this))
        {
            db->SetSql("deleteFZFriends");
            db->SetCondition("id", VgFZManager::CatString(m_id, msg->to()));
            SendMsg(db);
        }
    }

    if (FZ2FZMessage *ms = new FZ2FZMessage(*this, msg->to()))
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

void ObjectVgFZ::_prcsUpdateSWKey(UpdateSWKey *pb)
{
    if (!pb)
        return;

    int ret = !GetAuth(ObjectVgFZ::Type_Manager) ? -1 : (!CheckSWKey(pb->swkey()) ? -2 : 0);
    if (ret < 0)
    {
        if (auto ack = new AckUpdateSWKey)
        {
            ack->set_seqno(m_seq++);
            ack->set_result(ret < 0?ret : 0);
            ack->set_swkey(pb->swkey());
            WaitSend(ack);
        }
        return;
    }

    DBMessage *msg = new DBMessage(*this, IMessage::UpdateSWKeysRslt);
    msg->SetSeqNomb(pb->seqno());
    int op = pb->has_change() ? pb->change() : 0;
    msg->SetSql(op == 0 ? "insertFZKey" : "updateFZPCReg");
    if (op!=0)
    { 
        msg->SetCondition("swsn", pb->swkey());
    }
    else
    {
        msg->SetWrite("swsn", pb->swkey());
        msg->SetWrite("genTm", Utility::msTimeTick());
    }

    if (op == 2)
    {
        msg->SetWrite("used", 0);
        msg->SetWrite("pcsn", Variant());
    }
    else
    {
        if (pb->has_ver())
            msg->SetWrite("ver", pb->ver());
        if (pb->has_dscr())
            msg->SetWrite("dscr", pb->dscr());
    }

    msg->SetRead("swsn", pb->swkey());
    SendMsg(msg);
}

void ObjectVgFZ::_prcsReqSWKeyInfo(das::proto::ReqSWKeyInfo *msg)
{
    if (!msg)
        return;

    if (!GetAuth(IObject::Type_Manager))
    {
        if (auto ack = new AckSWKeyInfo)
        {
            ack->set_seqno(msg->seqno());
            WaitSend(ack);
        }
        return;
    }
    DBMessage *msgDB = new DBMessage(*this, IMessage::QuerySWKeyInfoRslt);
    if (!msgDB)
        return;

    msgDB->SetSeqNomb(msg->seqno());
    msgDB->SetSql("queryFZPCReg", true);
    if (msg->has_swkey())
        msgDB->SetCondition("swsn", msg->swkey());
    else if (msg->has_pcsn())
        msgDB->SetCondition("pcsn", msg->pcsn());
    else
        msgDB->SetCondition("genTm", msg->has_begtm() ? msg->begtm() : 0);

    SendMsg(msgDB);
}

void ObjectVgFZ::_prcsSWRegist(SWRegist *pb)
{
    if (!pb)
        return;

    m_pcsn = pb->pcsn();// Utility::FindString(pb->pcsn(), "O.E.M") >= 0 ? "O.E.M" : pb->pcsn();
    if (m_pcsn.empty() || m_pcsn=="O.E.M")
    {
        if (auto ack = new AckSWRegist)
        {
            ack->set_seqno(pb->seqno());
            ack->set_result(-1);
            WaitSend(ack);
            GetManager()->Log(0, IObjectManager::GetObjectFlagID(this), 0, "PC %s register key:%s fail!", m_pcsn.c_str(), pb->swkey().c_str());
        }
        return;
    }

    DBMessage *msg = new DBMessage(*this, IMessage::SWRegistRslt);
    if (!msg)
        return;

    msg->SetSeqNomb(pb->seqno());
    msg->SetSql("updateFZPCReg");
    msg->SetWrite("pcsn", m_pcsn);
    msg->SetWrite("used", 1);
    msg->SetWrite("regTm", Utility::msTimeTick());
    msg->SetCondition("swsn", pb->swkey());
    msg->SetCondition("used", 0);

    msg->AddSql("queryFZPCReg");
    msg->SetCondition("swsn", pb->swkey(), 1);
    SendMsg(msg);
}

void ObjectVgFZ::_prcsPostFZResult(PostFZResult *rslt)
{
    if (!rslt || !rslt->has_rslt())
        return;

    const FZResult &fzRslt = rslt->rslt();
    DBMessage *msg = new DBMessage(*this, IMessage::InserFZRslt);
    if (!msg)
        return;

    msg->SetSql("insertFZResult");
    msg->SetSeqNomb(rslt->seqno());
    msg->SetWrite("user", GetObjectID());
    msg->SetWrite("begTm", fzRslt.has_begtm() ? fzRslt.begtm() : Utility::msTimeTick());
    msg->SetWrite("usedTm", fzRslt.usedtm());
    msg->SetWrite("type", fzRslt.type());
    msg->SetWrite("result", fzRslt.rslt());
    msg->SetWrite("angvax", fzRslt.angvax());
    msg->SetWrite("horvax", fzRslt.horvax());
    msg->SetWrite("altvax", fzRslt.altvax());
    msg->SetWrite("maxvel", fzRslt.maxvel());
    msg->SetWrite("minvel", fzRslt.minvel());
    if (fzRslt.has_info())
    {
        msg->SetWrite("uid", fzRslt.info().id());
        msg->SetWrite("info", packTestInfo(fzRslt.info()));
    }

    SendMsg(msg);
}

void ObjectVgFZ::_prcsRequestFZResults(const RequestFZResults &req)
{
    DBMessage *msg = new DBMessage(*this, IMessage::QueryFZRslt);
    if (!msg)
        return;

    msg->SetSql("queryFZResult", true);
    msg->SetSeqNomb(req.seqno());
    msg->SetCondition("user", GetObjectID());
    if (req.has_tmbeg())
        msg->SetCondition("begTm:>", req.tmbeg());
    if (req.has_tmend())
        msg->SetCondition("begTm:<", (int64_t)req.tmend());

    SendMsg(msg);
}

void ObjectVgFZ::_prcsPostFZInfo(const PostFZInfo &msg)
{
    bool res = GetAuth(IObject::Type_Common) && msg.has_info();

    if (res)
        res = _saveInfo(msg.info());

    if (auto ack = new AckPostFZInfo)
    {
        ack->set_seqno(msg.seqno());
        ack->set_rslt(res ? 1 : 0);
        WaitSend(ack);
    }
}

void ObjectVgFZ::_prcsRequestFZInfo(const RequestFZInfo &msg)
{
    auto ack = new AckFZInfo;
    if (!ack)
        return;

    ack->set_seqno(msg.seqno());
    if (IsConnect())
    {
        auto info = ack->mutable_info();
        info->set_email(m_email);
        auto strLs = Utility::SplitString(m_info, "#;#", false);
        if (strLs.size() == 6)
        {
            auto itr = strLs.begin();
            info->set_name(*itr++);
            info->set_grade(*itr++);
            info->set_major(*itr++);
            info->set_id(*itr++);
            info->set_school(*itr++);
            info->set_births(*itr);
        }
    }
    WaitSend(ack);
}

void ObjectVgFZ::_prcsPostGetFZPswd(const PostGetFZPswd &)
{
    return;
}

void ObjectVgFZ::_prcsPostChangeFZPswd(const das::proto::PostChangeFZPswd &msg)
{
    int rslt = 0;
    if (!GetAuth(IObject::Type_Common))
        rslt = -2;
    else if (GetAuth(IObject::Type_Manager))
        rslt = -1;
    else if (msg.has_old() && msg.has_pswd() && msg.old() == m_pswd)
        rslt = 1;

    auto *msgDB = rslt == 1 ? new DBMessage(*this) : NULL;
    if (msgDB)
    {
        m_pswd = msg.pswd();
        msgDB->SetSql("changeUser");
        msgDB->SetCondition("user", GetObjectID());
        msgDB->SetWrite("pswd", m_pswd);
        SendMsg(msgDB);
    }

    if (auto ack = new AckChangeFZPswd)
    {
        ack->set_seqno(msg.seqno());
        ack->set_result(rslt);
        WaitSend(ack);
    }
}

bool ObjectVgFZ::_saveInfo(const FZInfo &info)
{
    string str = info.has_name() ? info.name() : string();
    str += "#;#";
    str += info.has_grade() ? info.grade() : string();
    str += "#;#";
    str += info.has_major() ? info.major() : string();
    str += "#;#";
    str += info.has_id() ? info.id() : string();
    str += "#;#";
    str += info.has_school() ? info.school() : string();
    str += "#;#";
    str += info.has_births() ? info.births() : string();

    if (m_info == str && m_email==info.email())
        return !m_email.empty();

    if (auto *msgDB = new DBMessage(*this))
    {
        msgDB->SetSql("changeUser");
        msgDB->SetCondition("user", GetObjectID());
        if (m_info != str)
        {
            m_info = str;
            msgDB->SetWrite("info", str);
        }
        if (m_email != info.email())
        {
            m_email = info.email();
            msgDB->SetWrite("email", info.email());
        }
        SendMsg(msgDB);
        return true;
    }
    return false;
}

void ObjectVgFZ::_checkFZ(const string &user, int ack)
{
    DBMessage *msgDb = new DBMessage(*this, IMessage::UserCheckRslt);
    if (!msgDb)
        return;

    msgDb->SetSeqNomb(ack);
    msgDb->SetSql("checkFZ");
    msgDb->SetCondition("user", user);
    SendMsg(msgDb);
}
