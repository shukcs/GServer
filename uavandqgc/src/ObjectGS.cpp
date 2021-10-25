#include "ObjectGS.h"
#include "socketBase.h"
#include "das.pb.h"
#include "ProtoMsg.h"
#include "Messages.h"
#include "Utility.h"
#include "IMessage.h"
#include "DBMessages.h"
#include "GSManager.h"
#include "FWAssist.h"
#include "ObjectUav.h"
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
ObjectGS::ObjectGS(const std::string &id, int seq): ObjectAbsPB(id)
, m_auth(1), m_bInitFriends(false), m_countLand(0), m_countPlan(0)
, m_seq(seq)
{
    SetBuffSize(WRITE_BUFFLEN);
}

ObjectGS::~ObjectGS()
{
}

void ObjectGS::OnConnected(bool bConnected)
{
    if (bConnected)
        SetSocketBuffSize(WRITE_BUFFLEN);
    else if (!m_check.empty())
        Release();
}

void ObjectGS::SetPswd(const std::string &pswd)
{
    m_pswd = pswd;
}

const std::string &ObjectGS::GetPswd() const
{
    return m_pswd;
}

void ObjectGS::SetAuth(int a)
{
    m_auth = a;
}

int ObjectGS::Authorize() const
{
    return m_auth;
}

bool ObjectGS::GetAuth(AuthorizeType auth) const
{
    return (auth & m_auth) == auth;
}

int ObjectGS::GSType()
{
    return IObject::GroundStation;
}

int ObjectGS::GetObjectType() const
{
    return GSType();
}

void ObjectGS::_prcsLogin(RequestGSIdentityAuthentication *msg)
{
    if (msg && m_p)
    {
        bool bSuc = m_pswd == msg->password();
        OnLogined(bSuc);
        if (auto ack = new AckGSIdentityAuthentication)
        {
            ack->set_seqno(msg->seqno());
            ack->set_result(bSuc ? 1 : -1);
            ack->set_auth(m_auth);
            WaitSend(ack);
        }
    }
}

void ObjectGS::_prcsHeartBeat(das::proto::PostHeartBeat *msg)
{
    if (auto ahb = new AckHeartBeat)
    {
        ahb->set_seqno(msg->seqno());
        WaitSend(ahb);
    }
}

void ObjectGS::_prcsProgram(PostProgram *msg)
{
    if (!msg)
        return;

    int res = (m_auth&Type_Admin) ? 1 : -1;
    if (res > 0 && msg->has_name())
    {
        if (msg->has_crc32())
            FWAssist::SetFWCrc32(msg->name(), msg->crc32());
        if (msg->has_release())
            FWAssist::SetFWRelease(msg->name(), msg->release());
        int ret = FWAssist::ProcessFW( msg->name()
            , msg->has_data() ? msg->data().c_str() : NULL
            , msg->has_data() ? msg->data().size() : 0
            , msg->offset()
            , msg->fwtype()
            , msg->has_length() ? msg->length() : 0 );

        if (ret == FWAssist::Stat_UploadError)
            res = 0;
        else if (ret == FWAssist::Stat_Uploaded)
            notifyUavNewFw(msg->name(), msg->seqno());
    }
    if (auto ack = new AckPostProgram)
    {
        ack->set_seqno(msg->seqno());
        ack->set_result(res);
        WaitSend(ack);
    }
}

void ObjectGS::ProcessMessage(const IMessage *msg)
{
    int tp = msg->GetMessgeType();
    if(m_p)
    {
        switch (tp)
        {
        case IMessage::BindUavRslt:
        case IMessage::QueryDeviceRslt:
        case IMessage::ControlDeviceRslt:
        case IMessage::SychMissionRslt:
        case IMessage::PostORRslt:
        case IMessage::DeviceAllocationRslt:
        case IMessage::PushUavSndInfo:
        case IMessage::ControlUser:
            processControlUser(*((GSOrUavMessage *)msg)->GetProtobuf());
            break;
        case IMessage::User2User:
        case IMessage::User2UserAck:
            processGs2Gs(*(Message*)msg->GetContent(), tp);
            break;
        case IMessage::SyncDeviceis:
            ackSyncDeviceis();
            break;
        case IMessage::SuspendRslt:
            processSuspend(*(DBMessage*)msg);
            break;
        case IMessage::DeviceisQueryRslt:
            processUavsInfo(*(DBMessage*)msg);
            break;
        case IMessage::DeviceBindRslt:
            processBind(*(DBMessage*)msg);
            break;
        case IMessage::UserInsertRslt:
            processGSInsert(*(DBMessage*)msg);
            break;
        case IMessage::UserQueryRslt:
            processGSInfo(*(DBMessage*)msg);
            break;
        case IMessage::UserCheckRslt:
            processCheckGS(*(DBMessage*)msg);
            break;
        case IMessage::LandInsertRslt:
            processPostLandRslt(*(DBMessage*)msg);
            break;
        case IMessage::LandQueryRslt:
            processQueryLands(*(DBMessage*)msg);
            break;
        case IMessage::CountLandRslt:
            processCountLandRslt(*(DBMessage*)msg);
            break;
        case IMessage::CountPlanRslt:
            processCountPlanRslt(*(DBMessage*)msg);
            break;
        case IMessage::DeleteLandRslt:
            processDeleteLandRslt(*(DBMessage*)msg);
            break;
        case IMessage::DeletePlanRslt:
            processDeletePlanRslt(*(DBMessage*)msg);
            break;
        case IMessage::PlanInsertRslt:
            processPostPlanRslt(*(DBMessage*)msg);
            break;
        case IMessage::PlanQueryRslt:
            processQueryPlans(*(DBMessage*)msg);
            break;
        case IMessage::FriendQueryRslt:
            processFriends(*(DBMessage*)msg);
            break;
        case IMessage::QueryMissionsRslt:
            processMissions(*(DBMessage*)msg);
            break;
        case IMessage::QueryMissionsAcreageRslt:
            processMissionsAcreage(*(DBMessage*)msg);
            break;
        default:
            break;
        }
    }
}

void ObjectGS::PrcsProtoBuff(uint64_t ms)
{
    if (!m_p)
        return;

    string strMsg = m_p->GetMsgName();
    if (strMsg == d_p_ClassName(RequestGSIdentityAuthentication))
        _prcsLogin((RequestGSIdentityAuthentication*)m_p->GetProtoMessage());
    else if (strMsg == d_p_ClassName(RequestNewGS))
        _prcsReqNewGs((RequestNewGS*)m_p->GetProtoMessage());
    else if (strMsg == d_p_ClassName(PostHeartBeat))
        _prcsHeartBeat((PostHeartBeat *)m_p->GetProtoMessage());
    else if (strMsg == d_p_ClassName(PostProgram))
        _prcsProgram((PostProgram *)m_p->GetProtoMessage());
    else if (strMsg == d_p_ClassName(SyncDeviceList))
        _prcsSyncDeviceList((SyncDeviceList *)m_p->GetProtoMessage());
    else if (strMsg == d_p_ClassName(RequestBindUav))
        _prcsReqBind(*(RequestBindUav *)m_p->GetProtoMessage());
    else if (strMsg == d_p_ClassName(PostControl2Uav))
        _prcsControl2Uav((PostControl2Uav*)m_p->DeatachProto());
    else if (strMsg == d_p_ClassName(RequestIdentityAllocation))
        _prcsUavIDAllication((RequestIdentityAllocation *)m_p->DeatachProto());
    else if (strMsg == d_p_ClassName(RequestUavStatus))
        _prcsReqUavs((RequestUavStatus *)m_p->DeatachProto());
    else if (strMsg == d_p_ClassName(PostParcelDescription))
        _prcsPostLand((PostParcelDescription *)m_p->GetProtoMessage());
    else if (strMsg == d_p_ClassName(RequestParcelDescriptions))
        _prcsReqLand((RequestParcelDescriptions *)m_p->GetProtoMessage());
    else if (strMsg == d_p_ClassName(DeleteParcelDescription))
        _prcsDeleteLand((DeleteParcelDescription *)m_p->GetProtoMessage());
    else if (strMsg == d_p_ClassName(PostOperationDescription))
        _prcsPostPlan((PostOperationDescription *)m_p->GetProtoMessage());
    else if (strMsg == d_p_ClassName(RequestOperationDescriptions))
        _prcsReqPlan((RequestOperationDescriptions *)m_p->GetProtoMessage());
    else if (strMsg == d_p_ClassName(DeleteOperationDescription))
        _prcsDeletePlan((DeleteOperationDescription*)m_p->GetProtoMessage());
    else if (strMsg == d_p_ClassName(PostOperationRoute))
        _prcsPostMission((PostOperationRoute*)m_p->DeatachProto());
    else if (strMsg == d_p_ClassName(GroundStationsMessage))
        _prcsGsMessage((GroundStationsMessage*)m_p->DeatachProto());
    else if (strMsg == d_p_ClassName(RequestFriends))
        _prcsReqFriends((RequestFriends*)m_p->GetProtoMessage());
    else if (strMsg == d_p_ClassName(RequestUavMission))
        _prcsReqMissons(*(RequestUavMission*)m_p->GetProtoMessage());
    else if (strMsg == d_p_ClassName(RequestUavMissionAcreage))
        _prcsReqMissonsAcreage(*(RequestUavMissionAcreage*)m_p->GetProtoMessage());
    else if (strMsg == d_p_ClassName(RequestMissionSuspend))
        _prcsReqSuspend(*(RequestMissionSuspend*)m_p->GetProtoMessage());
    else if (strMsg == d_p_ClassName(RequestOperationAssist))
        _prcsReqAssists((RequestOperationAssist*)m_p->DeatachProto());
    else if (strMsg == d_p_ClassName(RequestABPoint))
        _prcsReqABPoint((RequestABPoint*)m_p->DeatachProto());
    else if (strMsg == d_p_ClassName(RequestOperationReturn))
        _prcsReqReturn((RequestOperationReturn*)m_p->DeatachProto());

    if (m_stInit == IObject::Initialed)
        m_tmLastInfo = ms;
}

void ObjectGS::processGs2Gs(const Message &msg, int tp)
{
    if (tp == IMessage::User2User)
    {
        auto gsmsg = (const GroundStationsMessage *)&msg;
        if (Gs2GsMessage *ms = new Gs2GsMessage(this, gsmsg->from()))
        {
            auto ack = new AckGroundStationsMessage;
            ack->set_seqno(gsmsg->seqno());
            ack->set_res(1);
            ack->set_gs(gsmsg->to());
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


void ObjectGS::processControlUser(const Message &msg)
{
    if (!GetAuth(Type_Manager) || msg.GetDescriptor()->full_name() == d_p_ClassName(PostStatus2GroundStation))
        CopyAndSend(msg);
}

void ObjectGS::processBind(const DBMessage &msg)
{
    auto proto =new AckRequestBindUav;
    bool res = msg.GetRead(EXECRSLT, 1).ToBool();

    if(proto)
    {
        proto->set_seqno(msg.GetSeqNomb());
        proto->set_opid(msg.GetRead("binded", 1).ToInt8());
        proto->set_result(res ? 0 : 1);
    }
    if (res)
    { 
        if(UavStatus *s = new UavStatus)
        {
            ObjectUav uav(msg.GetRead("id", 1).ToString());
            ObjectUav::InitialUAV(msg, uav, 1);
            uav.TransUavStatus(*s);
            if (proto)
                proto->set_allocated_status(s);

            if (auto gs2uav = new GS2UavMessage(this, uav.GetObjectID()))
            {
                gs2uav->SetPBContent(*s);
                SendMsg(gs2uav);
            }
        }
    }
    WaitSend(proto);
}

void ObjectGS::processUavsInfo(const DBMessage &msg)
{
    auto ack = new AckRequestUavStatus;
    int idx = msg.IndexofSql("queryUavInfo");
    if (!ack || idx<0)
        return;

    auto ids = msg.GetRead("id", idx).GetVarList();
    auto bindeds = msg.GetRead("binded", idx).GetVarList();
    auto binders = msg.GetRead("binder", idx).GetVarList();
    auto lats = msg.GetRead("lat", idx).GetVarList();
    auto lons = msg.GetRead("lon", idx).GetVarList();
    auto timeBinds = msg.GetRead("timeBind", idx).GetVarList();
    auto valids = msg.GetRead("valid", idx).GetVarList();
    auto authChecks = msg.GetRead("authCheck", idx).GetVarList();
    auto sims = msg.GetRead("simID", idx).GetVarList();

    ack->set_seqno(msg.GetSeqNomb());
    auto idItr = ids.begin();
    auto bindedItr = bindeds.begin();
    auto binderItr = binders.begin();
    auto latItr = lats.begin();
    auto lonItr = lons.begin();
    auto timeBindItr = timeBinds.begin();
    auto validItr = valids.begin();
    auto authCheckItr = authChecks.begin();
    auto simItr = sims.begin();

    for (; idItr != ids.end(); ++idItr)
    {
        UavStatus *us = ack->add_status();
        us->set_result(1);
        us->set_uavid(idItr->ToString());
        string binder;
        if (binderItr != binders.end())
        {
            binder = binderItr->ToString();
            us->set_binder(binder);
            ++binderItr;
        }
        bool bBind = bindedItr != bindeds.end() ? bindedItr->ToInt8() == 1 : false;
        if (bindedItr != bindeds.end())
            ++bindedItr;
        us->set_binded(bBind);
        us->set_time(timeBindItr != timeBinds.end() ? timeBindItr->ToInt64() : 0);
        if (timeBindItr != timeBinds.end())
            ++timeBindItr;
        us->set_online(false);

        bool bAuth = (binder == m_id && bBind) || GetAuth(Type_Manager);
        if (authCheckItr != authChecks.end() && bAuth)
        {
            us->set_authstring(authCheckItr->ToString());
            ++authCheckItr;
        }
        if (validItr != valids.end())
        {
            us->set_deadline(validItr->ToInt64());
            ++validItr;
        }

        if (latItr != lats.end() && lonItr != lons.end())
        {
            GpsInformation *gps = new GpsInformation;
            gps->set_latitude(int(latItr->ToDouble()*1e7));
            gps->set_longitude(int(lonItr->ToDouble()*1e7));
            gps->set_altitude(0);
            us->set_allocated_pos(gps);
            latItr++;
            lonItr++;
        }
        if (simItr != sims.end())
        {
            us->set_simid(simItr->ToString());
            ++simItr;
        }
    }
    WaitSend(ack);
}

void ObjectGS::processSuspend(const DBMessage &msg)
{
    bool bExist = msg.GetRead(EXECRSLT).ToBool();
    auto ack = new AckMissionSuspend;
    ack->set_seqno(msg.GetSeqNomb());
    ack->set_result(bExist ? 1 : 0);
    if (bExist)
    {
        auto sus = new MissionSuspend;
        if (!sus)
        {
            delete ack;
            return;
        }
        sus->set_planid(Utility::bigint2string(msg.GetRead("planID").ToInt64()));
        sus->set_uav(msg.GetRead("uavID").ToString());
        sus->set_user(msg.GetRead("userID").ToString());
        sus->set_curridge(msg.GetRead("end").ToInt32());
        int l = msg.GetRead("continiuLat").ToInt32();
        if (-180e7 <= l && l <= 180e7)
        {
            sus->set_continiulat(l);
            sus->set_continiulon(msg.GetRead("continiuLon").ToInt32());
        }
        ack->set_allocated_suspend(sus);
    }
    WaitSend(ack);
}

void ObjectGS::processGSInfo(const DBMessage &msg)
{
    m_stInit = msg.GetRead(EXECRSLT).ToBool() ? Initialed : ReleaseLater;
    string pswd = msg.GetRead("pswd").ToString();
    m_auth = msg.GetRead("auth").ToInt32();
    bool bLogin = pswd == m_pswd && !pswd.empty();
    if (m_stInit == Initialed)
        OnLogined(bLogin);

    AckGSIdentityAuthentication ack;
    ack.set_seqno(m_seq);
    ack.set_result(bLogin ? 1 : -1);
    ack.set_auth(m_auth);
    ObjectAbsPB::SendProtoBuffTo(GetSocket(), ack);
    m_seq = -1;
    m_pswd = pswd;
    if (Initialed == m_stInit)
    {
        initFriend();
        if (DBMessage *msg = new DBMessage(this, IMessage::CountLandRslt))
        {
            msg->SetSql("countLand");
            msg->SetCondition("gsuser", m_id);
            SendMsg(msg);
        }
        if (DBMessage *msg = new DBMessage(this, IMessage::CountPlanRslt))
        {
            msg->SetSql("countPlan");
            msg->SetCondition("planuser", m_id);
            SendMsg(msg);
        }
    }
}

void ObjectGS::processCheckGS(const DBMessage &msg)
{
    bool bExist = msg.GetRead("count(*)").ToInt32()>0;
    if (auto ack = new AckNewGS)
    {
        ack->set_seqno(msg.GetSeqNomb());
        if (!bExist)
        {
            m_check = Utility::RandString();
            ack->set_check(m_check);
        }
        ack->set_result(bExist ? 0 : 1);
        WaitSend(ack);
        if (bExist)
            m_stInit = ReleaseLater;
    }
}

void ObjectGS::processPostLandRslt(const DBMessage &msg)
{
    int64_t nCon = msg.GetRead(INCREASEField).ToInt64();
    int64_t nLand = msg.GetRead(INCREASEField, 1).ToInt64();
    if (auto appd = new AckPostParcelDescription)
    {
        appd->set_seqno(msg.GetSeqNomb());
        appd->set_result((nLand > 0 && nCon > 0) ? 1 : 0);
        appd->set_psiid(Utility::bigint2string(nLand));
        appd->set_pcid(Utility::bigint2string(nCon));
        appd->set_pdid(Utility::bigint2string(nLand));
        WaitSend(appd);
    }
    if (nLand > 0)
        GetManager()->Log(0, IObjectManager::GetObjectFlagID(this), 0, "Upload land %d!", nLand);

}

void ObjectGS::processFriends(const DBMessage &msg)
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

void ObjectGS::processQueryLands(const DBMessage &msg)
{
    auto ack = new AckRequestParcelDescriptions;
    if (!ack)
        return;

    ack->set_seqno(msg.GetSeqNomb());
    ack->set_result(1);
    const VariantList &ids = msg.GetRead("LandInfo.id").GetVarList();
    const StringList &namesLand = msg.GetRead("LandInfo.name").ToStringList();
    const StringList &users = msg.GetRead("LandInfo.gsuser").ToStringList();
    const VariantList &lats = msg.GetRead("LandInfo.lat").GetVarList();
    const VariantList &lons = msg.GetRead("LandInfo.lon").GetVarList();
    const VariantList &acreages = msg.GetRead("LandInfo.acreage").GetVarList();
    const StringList &boundary = msg.GetRead("LandInfo.boundary").ToStringList();

    const StringList &namesPc = msg.GetRead("OwnerInfo.name").ToStringList();
    const VariantList &birthdayPc = msg.GetRead("OwnerInfo.birthdate").GetVarList();
    const StringList &addresses = msg.GetRead("OwnerInfo.address").ToStringList();
    const StringList &mobilenos = msg.GetRead("OwnerInfo.mobileno").ToStringList();
    const StringList &phonenos = msg.GetRead("OwnerInfo.phoneno").ToStringList();
    const StringList &weixins = msg.GetRead("OwnerInfo.weixin").ToStringList();

    auto itrNamesPc = namesPc.begin();
    auto itrbirthday = birthdayPc.begin();
    auto itraddresses = addresses.begin();
    auto itrmobilenos = mobilenos.begin();
    auto itrphonenos = phonenos.begin();
    auto itrweixins = weixins.begin();
    auto itrnamesLand = namesLand.begin();
    auto itrusers = users.begin();
    auto itracreages = acreages.begin();
    auto itrlats = lats.begin();
    auto itrlons = lons.begin();
    auto itrids = ids.begin();
    auto itrboundary = boundary.begin();
    for (; itrids != ids.end(); ++itrids)
    {
        if (ack->ByteSize() > MaxSend)
        {
            WaitSend(ack);
            ack = new AckRequestParcelDescriptions;
            if (!ack)
                break;
            ack->set_seqno(msg.GetSeqNomb());
            ack->set_result(1);
        }

        ParcelDescription *pd = ack->add_pds();
        if (itrNamesPc != namesPc.end())
        {
            ParcelContracter *pc = new ParcelContracter();
            pc->set_name(*itrNamesPc);
            ++itrNamesPc;
            if (itrbirthday !=birthdayPc.end())
            {
                pc->set_birthdate(itrbirthday->ToInt64());
                ++itrbirthday;
            }
            if (itraddresses != addresses.end())
            {
                pc->set_address(*itraddresses);
                ++itraddresses;
            }
            if (itrmobilenos != mobilenos.end())
            {
                pc->set_mobileno(*itrmobilenos);
                ++itrmobilenos;
            }
            if (itrphonenos != phonenos.end())
            {
                pc->set_phoneno(*itrphonenos);
                ++itrphonenos;
            }
            if (itrweixins != weixins.end())
            {
                pc->set_phoneno(*itrweixins);
                ++itrweixins;
            }
            pd->set_allocated_pc(pc);
        }
        if (itrnamesLand != namesLand.end())
        {
            pd->set_name(*itrnamesLand);
            ++itrnamesLand;
        }
        if (itrusers != users.end())
        {
            pd->set_registerid(*itrusers);
            ++itrusers;
        }
        pd->set_acreage(itracreages!=acreages.end() ? (itracreages->ToFloat()) : 0);
        if (itracreages != acreages.end())
            ++itracreages;
        if (itrlats != lats.end() && itrlons != lons.end())
        {
            double lat = (itrlats++)->ToDouble();
            double lon = (itrlons++)->ToDouble();
            Coordinate *c = new Coordinate;
            c->set_altitude(0);
            c->set_latitude(int(lat*1e7));
            c->set_longitude(int(lon*1e7));
            pd->set_allocated_coordinate(c);

        }
        if (itrboundary != boundary.end())
        {
            ParcelSurveyInformation *psi = new ParcelSurveyInformation;
            psi->ParseFromArray(itrboundary->c_str(), itrboundary->size());
            psi->set_id(Utility::bigint2string(itrids->ToInt64()));
            pd->set_id(Utility::bigint2string(itrids->ToInt64()));
            pd->set_allocated_psi(psi);
            ++itrboundary;
        }
    }
    
    if (ack)
        WaitSend(ack);
}

void ObjectGS::processGSInsert(const DBMessage &msg)
{
    bool bSuc = msg.GetRead(EXECRSLT).ToBool();
    if (auto ack = new AckNewGS)
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

void ObjectGS::processMissions(const DBMessage &msg)
{
    auto ack = new AckUavMission;
    if (!ack)
        return;

    ack->set_seqno(msg.GetSeqNomb());
    const StringList &users = msg.GetRead("userID").ToStringList();
    const VariantList &lands = msg.GetRead("landId").GetVarList();
    const StringList &uavs = msg.GetRead("uavID").ToStringList();
    const VariantList &ftms = msg.GetRead("finishTime").GetVarList();
    const VariantList &begins = msg.GetRead("begin").GetVarList();
    const VariantList &ends = msg.GetRead("end").GetVarList();
    const VariantList &acreage = msg.GetRead("acreage").GetVarList();
    const VariantList &lats = msg.GetRead("continiuLat").GetVarList();
    const VariantList &lons = msg.GetRead("continiuLon").GetVarList();

    auto landsItr = lands.begin();
    auto tmItr = ftms.begin();
    auto areaItr = acreage.begin();
    auto itrBeg = begins.begin();
    auto itrEnd = ends.begin();
    auto itrUav = uavs.begin();
    auto itrUser = users.begin();
    auto itrLat = lats.begin();
    auto itrLon = lons.begin();
    for (const Variant &itr : msg.GetRead("planID").GetVarList())
    {
        if (   landsItr == lands.end()
            || tmItr == ftms.end()
            || areaItr == acreage.end()
            || itrUav == uavs.end()
            || itrBeg == begins.end()
            || itrEnd == ends.end() 
            || itrUser == users.end() )
        {
            delete ack;
            return;
        }

        if (ack->ByteSize() > MaxSend)
        {
            WaitSend(ack);
            ack = new AckUavMission;
            if (!ack)
                break;
            ack->set_seqno(msg.GetSeqNomb());
        }

        if (UavRoute *rt = ack->add_routes())
        {
            rt->set_plan(itr.IsNull() ? string() : itr.Val2String());
            rt->set_crttm(0);
            rt->set_optm(tmItr->ToInt64());
            rt->set_land(landsItr->IsNull()?string():landsItr->Val2String());
            rt->set_acreage(areaItr->ToFloat());
            rt->set_uav(*itrUav);
            rt->set_beg(itrBeg->ToInt32());
            rt->set_end(itrEnd->ToInt32());
            rt->set_user(*itrUser);

            int lat = (itrLat != lats.end() && itrLon != lons.end()) ? itrLat->ToInt32() : 2000000000;
            if (lat > -1800000000 && lat < 1800000000)
            {
                rt->set_continiulat(itrLat->ToInt32());
                rt->set_continiulon(lat);
                ++itrLat;
                ++itrLon;
            }
        }
        ++tmItr;
        ++landsItr;
        ++areaItr;
        ++itrUav;
        ++itrBeg;
        ++itrEnd;
        ++itrUser;
    }
    if (ack)
        WaitSend(ack);
}

void ObjectGS::processMissionsAcreage(const DBMessage &msg)
{
    auto sd = new AckUavMissionAcreage;
    auto acreage = msg.GetRead("sum(acreage)").ToDouble();
    if (!sd)
        return;

    sd->set_seqno(msg.GetSeqNomb());
    sd->set_acreage((float)acreage);
    WaitSend(sd);
}

void ObjectGS::processPostPlanRslt(const DBMessage &msg)
{
    const Variant &vRslt = msg.GetRead(EXECRSLT);
    int64_t id = msg.GetRead(INCREASEField).ToInt64();
    if (auto ack = new AckPostOperationDescription)
    {
        ack->set_seqno(msg.GetSeqNomb());
        ack->set_result(vRslt.ToBool() ? 1 : 0);
        if (vRslt.ToBool())
            ack->set_odid(Utility::bigint2string(id));
        WaitSend(ack);
    }

    GetManager()->Log(0, IObjectManager::GetObjectFlagID(this), 0, "Upload mission plan %ld!", id);
}

void ObjectGS::processCountLandRslt(const DBMessage &msg)
{
    if (msg.GetRead(EXECRSLT).ToBool())
        m_countLand = msg.GetRead("count(id)").ToInt32();
}

void ObjectGS::processCountPlanRslt(const DBMessage &msg)
{
    if (msg.GetRead(EXECRSLT).ToBool())
        m_countPlan = msg.GetRead("count(id)").ToInt32();
}

void ObjectGS::processDeleteLandRslt(const DBMessage &msg)
{
    if (msg.GetRead(EXECRSLT, 1).ToBool())
        m_countLand--;
    if (msg.GetRead(EXECRSLT, 2).ToBool())
        m_countPlan--;
}

void ObjectGS::processDeletePlanRslt(const DBMessage &msg)
{
    if (msg.GetRead(EXECRSLT).ToBool())
        m_countPlan--;
}

void ObjectGS::processQueryPlans(const DBMessage &msg)
{
    bool suc = msg.GetRead(EXECRSLT).ToBool();
    auto ack = new AckRequestOperationDescriptions;
    if (ack)
    {
        ack->set_seqno(msg.GetSeqNomb());
        ack->set_result(suc ? 1 : 0);
    }

    const VariantList &ids = msg.GetRead("id").GetVarList();
    const VariantList &landIds = msg.GetRead("landId").GetVarList();
    const VariantList &planTimes = msg.GetRead("planTime").GetVarList();
    const StringList &planusers = msg.GetRead("planuser").ToStringList();
    const StringList &notes = msg.GetRead("notes").ToStringList();
    const StringList &crops = msg.GetRead("crop").ToStringList();
    const StringList &drugs = msg.GetRead("drug").ToStringList();
    const VariantList &ridgeSzs = msg.GetRead("ridgeSz").GetVarList();
    const VariantList &prizes = msg.GetRead("prize").GetVarList();
    const StringList &planParams = msg.GetRead("planParam").ToStringList();

    auto landIdsItr = landIds.begin();
    auto planTimesItr = planTimes.begin();
    auto planusersItr = planusers.begin();
    auto notesItr = notes.begin();
    auto cropsItr = crops.begin();
    auto drugsItr = drugs.begin();
    auto prizesItr = prizes.begin();
    auto ridgeItr = ridgeSzs.begin();
    auto planParamsItr = planParams.begin();
    for (auto idItr = ids.begin(); idItr != ids.end(); ++idItr)
    {
        if (ack->ByteSize() >= MaxSend)
        {
            WaitSend(ack);
            ack = new AckRequestOperationDescriptions;
            if (!ack)
                break;
            ack->set_seqno(msg.GetSeqNomb());
            ack->set_result(suc ? 1 : 0);
        }
        auto od = ack->add_ods();
        od->set_odid(Utility::bigint2string(idItr->ToInt64()));
        if (landIdsItr != landIds.end())
        {
            od->set_pdid(Utility::bigint2string(landIdsItr->ToInt64()));
            ++landIdsItr;
        }
        if (planTimesItr != planTimes.end())
        {
            od->set_plantime(planTimesItr->ToInt64());
            ++planTimesItr;
        }
        if (planusersItr != planusers.end())
        {
            od->set_registerid(*planusersItr);
            ++planusersItr;
        }
        if (notesItr != notes.end())
        {
            od->set_notes(*notesItr);
            ++notesItr;
        }
        if (cropsItr != crops.end())
        {
            od->set_crop(*cropsItr);
            ++cropsItr;
        }
        if (ridgeItr != ridgeSzs.end())
        {
            od->set_ridge(ridgeItr->ToInt32());
            ++ridgeItr;
        }
        if (drugsItr != drugs.end())
        {
            od->set_drug(*drugsItr);
            ++drugsItr;
        }
        if (prizesItr != prizes.end())
        {
            od->set_prize(prizesItr->ToFloat());
            ++prizesItr;
        }
        if (planParamsItr != planParams.end())
        {
            auto op = new OperationPlan;
            op->ParseFromArray(planParamsItr->c_str(), planParamsItr->size());
            od->set_allocated_op(op);
            ++planParamsItr;
        }
    }
    if(ack)
        WaitSend(ack);
}

void ObjectGS::InitObject()
{
    if (IObject::Uninitial == m_stInit)
    {
        if (m_check.empty())
        {
            DBMessage *msg = new DBMessage(this, IMessage::UserQueryRslt);
            if (!msg)
                return;
            msg->SetSql("queryGSInfo");
            msg->SetCondition("user", m_id);
            SendMsg(msg);
            m_stInit = IObject::Initialing;
            return;
        }
        m_stInit = IObject::Initialed;
    }
}

void ObjectGS::CheckTimer(uint64_t ms, char *buf, int len)
{
    ObjectAbsPB::CheckTimer(ms, buf, len);
    ms -= m_tmLastInfo;
    if(m_auth > Type_ALL && ms>50)
        Release();
    if(ReleaseLater == m_stInit && ms>100)
        Release();
    else if (ms > 600000 || (!m_check.empty() && ms > 60000))
        Release();
    else if (m_check.empty() && ms > 10000)//超时关闭
        CloseLink();   
}

bool ObjectGS::IsAllowRelease() const
{
    return !GetAuth(Type_Manager);
}

void ObjectGS::FreshLogin(uint64_t ms)
{
    m_tmLastInfo = ms;
}

void ObjectGS::SetCheck(const std::string &str)
{
    m_check = str;
}

void ObjectGS::_prcsReqUavs(RequestUavStatus *msg)
{
    if (!msg)
        return;

    if (GS2UavMessage *ms = new GS2UavMessage(this, string()))
    {
        ms->AttachProto(msg);
        SendMsg(ms);
    }
}

void ObjectGS::_prcsSyncDeviceList(SyncDeviceList *ms)
{
    if (ms && GetAuth(Type_Manager))
    {
        m_seq = ms->seqno();
        if (auto syn = new ObjectSignal(this, GSType(), IMessage::SyncDeviceis, GetObjectID()))
            SendMsg(syn);
    }
    else if (ms)
    {
        if (auto ack = new AckSyncDeviceList)
        {
            ack->set_seqno(m_seq);
            ack->set_result(0);
            WaitSend(ack);
        }
    }
}

void ObjectGS::_prcsReqBind(const RequestBindUav&msg)
{
    if (GetAuth(Type_Manager) && !msg.uavid().empty())
    {
        if (msg.opid() == 1)
        { 
            Subcribe(msg.uavid(), IMessage::PushUavSndInfo);
            Subcribe(msg.uavid(), IMessage::ControlUser);
        }
        else if (msg.opid() == 0)
        { 
            Unsubcribe(msg.uavid(), IMessage::PushUavSndInfo);
            Unsubcribe(msg.uavid(), IMessage::ControlUser);
        }
    }

    saveBind(msg.uavid(), msg.opid() == 1);
}

void ObjectGS::_prcsControl2Uav(das::proto::PostControl2Uav *msg)
{
    if (!msg || GetAuth(Type_Manager))
        return;

    if (GS2UavMessage *ms = new GS2UavMessage(this, msg->uavid()))
    {
        ms->AttachProto(msg);
        SendMsg(ms);
    }
}

void ObjectGS::_prcsPostLand(PostParcelDescription *msg)
{
    if (!msg || GetAuth(Type_Manager))
        return;

    if (m_countLand >= MAXLANDRECORDS)
    {
        if (auto appd = new AckPostParcelDescription)
        { 
            appd->set_seqno(msg->seqno());
            appd->set_result(-1); //地块太多了
            WaitSend(appd);
        }
    }

    DBMessage *db = new DBMessage(this, IMessage::LandInsertRslt);
    if (!db)
        return;

    db->SetSeqNomb(msg->seqno());
    const ParcelDescription &land = msg->pd();
    const ParcelContracter &pc = land.pc();
    bool bSuc;
    uint64_t nCon = Utility::str2int(land.has_pc() ? land.pc().id() : "", 10, &bSuc);
    db->SetSql(bSuc ? "updateOwner" : "insertOwner");
    if (bSuc)
    {
        db->SetWrite("id", nCon);
        db->SetWrite(INCREASEField, nCon);
    }
    if (pc.has_name())
        db->SetWrite("name", pc.name());
    if (pc.has_birthdate())
        db->SetWrite("birthdate", pc.birthdate());
    if (pc.has_address())
        db->SetWrite("address", pc.address());
    if (pc.has_mobileno())
        db->SetWrite("mobileno", pc.mobileno());
    if (pc.has_phoneno())
        db->SetWrite("phoneno", pc.phoneno());
    if (pc.has_weixin())
        db->SetWrite("weixin", pc.weixin());

    nCon = Utility::str2int(land.has_pc() ? land.pc().id() : "", 10, &bSuc);
    db->AddSql(bSuc ? "updateLand" : "insertLand");
    db->SetRefFiled("ownerID");
    if (land.has_name())
        db->SetWrite("name", land.name(), 1);
    if (land.has_registerid())
        db->SetWrite("gsuser", land.registerid(), 1);
    if (land.has_acreage())
        db->SetWrite("acreage", land.acreage(), 1);
    if (bSuc)
    {
        db->SetWrite("id", nCon, 1);
        db->SetWrite(INCREASEField, nCon, 1);
    }
    if (land.has_coordinate())
    {
        db->SetWrite("lat", double(land.coordinate().latitude() / 1e7), 1);
        db->SetWrite("lon", double(land.coordinate().longitude() / 1e7), 1);
    }
    if (land.has_psi())
    {
        string buff;
        buff.resize(land.psi().ByteSize());
        land.psi().SerializeToArray(&buff.front(), buff.size());
        db->SetWrite("boundary", Variant(buff.size(), &buff.front()), 1);
    }

    SendMsg(db);
}

void ObjectGS::_prcsReqLand(RequestParcelDescriptions *msg)
{
    if (GetAuth(Type_Manager) || !msg || (!msg->has_registerid() && !msg->has_pdid()))
        return;

    DBMessage *db = new DBMessage(this, IMessage::LandQueryRslt);
    if (!db)
        return;

    if (msg->has_registerid())
        db->SetCondition("LandInfo.gsuser", msg->registerid());
    if (msg->has_pdid())
        db->SetCondition("LandInfo.id", msg->pdid());

    db->SetSeqNomb(msg->seqno());
    db->SetSql("queryLand", true);

    SendMsg(db);
    AckRequestParcelDescriptions ack;
    ack.set_seqno(msg->seqno());
}

int ObjectGS::_addDatabaseUser(const std::string &user, const std::string &pswd, int seq)
{
    GSManager *mgr = (GSManager *)(GetManager());
    int ret = -1;
    if (mgr)
        ret = mgr->AddDatabaseUser(user, pswd, this, seq) > 0 ? 2 : -1;

    if (ret > 0)
        m_pswd = pswd;

    return ret;
}

void ObjectGS::notifyUavNewFw(const std::string &fw, int seq)
{
    if (GS2UavMessage *ms = new GS2UavMessage(this, string()))
    {
        NotifyProgram *np = new NotifyProgram;
        np->set_seqno(seq);
        np->set_name(fw);
        np->set_fwtype(FWAssist::GetFWType(fw));
        np->set_length(FWAssist::GetFWLength(fw));
        np->set_crc32(FWAssist::GetFWCrc32(fw));
        ms->AttachProto(np);
        SendMsg(ms);
    }
}

void ObjectGS::ackSyncDeviceis()
{
    auto mgr = (GSManager *)GetManager();
    AckSyncDeviceList ack;
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

void ObjectGS::saveBind(const std::string &uav, bool bBind)
{
    if (GetAuth(Type_Manager) && bBind)
        return;

    if (DBMessage *msg = new DBMessage(this, IMessage::DeviceBindRslt, DBMessage::DB_Uav))
    {
        msg->SetSql("updateBinded");
        msg->SetWrite("timeBind", Utility::msTimeTick());
        msg->SetWrite("binded", bBind);
        msg->SetWrite("binder", GetObjectID());
        msg->SetCondition("id", uav);
        if (!GetAuth(Type_Manager))
        {
            msg->SetCondition("binded", false);
            msg->SetCondition("binder", GetObjectID());
        }

        msg->AddSql("queryUavInfo");
        msg->SetCondition("id", uav, 1);
        SendMsg(msg);
    }
    GetManager()->Log(0, IObjectManager::GetObjectFlagID(this), 0, "%s %s!", bBind?"bind": "unbind", uav.c_str());
}

void ObjectGS::initFriend()
{
    if (m_bInitFriends)
        return;

    DBMessage *msg = new DBMessage(this, IMessage::FriendQueryRslt);
    if (!msg)
        return;
    m_bInitFriends = true;
    msg->SetSql("queryGSFriends", true);
    msg->SetCondition("usr1", m_id);
    msg->SetCondition("usr2", m_id);
    SendMsg(msg);
}

void ObjectGS::addDBFriend(const string &user1, const string &user2)
{
    if (DBMessage *msg = new DBMessage(this))
    {
        msg->SetSql("insertGSFriends");
        msg->SetWrite("id", GSManager::CatString(user1, user2));
        msg->SetWrite("usr1", user1);
        msg->SetWrite("usr2", user2);
        SendMsg(msg);
    }
}

void ObjectGS::_prcsDeleteLand(DeleteParcelDescription *msg)
{
    if (!msg)
        return;

    const std::string &id = msg->pdid();
    DBMessage *msgDb = new DBMessage(this, IMessage::DeleteLandRslt);
    if (!msgDb || id.empty())
        return;

    msgDb->SetSql("deleteLand");
    msgDb->SetCondition("LandInfo.id", id);
    if (!GetAuth(ObjectGS::Type_ALL))
        msgDb->SetCondition("LandInfo.gsuser", m_id);

    msgDb->AddSql("deletePlan");
    msgDb->SetCondition("landId", id, 1);
    SendMsg(msgDb);

    if (auto ackDD = new AckDeleteParcelDescription)
    {
        ackDD->set_seqno(msg->seqno());
        ackDD->set_result(1);
        WaitSend(ackDD);
    }
    GetManager()->Log(0, IObjectManager::GetObjectFlagID(this), 0, "Delete land %s!", id.c_str());
}

void ObjectGS::_prcsUavIDAllication(das::proto::RequestIdentityAllocation *msg)
{
    if (!msg)
        return;

    if (GetAuth(Type_Manager))
    {
        if (GS2UavMessage *ms = new GS2UavMessage(this, string()))
        {
            ms->AttachProto(msg);
            SendMsg(ms);
            return;
        }
    }

    if (auto ack = new AckIdentityAllocation)
    {
        ack->set_seqno(msg->seqno());
        ack->set_result(0);
        WaitSend(ack);
    }
}

void ObjectGS::_prcsPostPlan(PostOperationDescription *msg)
{
    if (!msg)
        return;
    const OperationDescription &od = msg->od();
    if (od.pdid().empty() || od.registerid().empty())
    {
        if (auto ack = new AckPostOperationDescription)
        {
            ack->set_seqno(msg->seqno());
            ack->set_result(0);
            WaitSend(ack);
        }
        return;
    }
    DBMessage *msgDb = new DBMessage(this, IMessage::PlanInsertRslt);
    if (!msgDb)
        return;

    int64_t id = od.has_odid() ? Utility::str2int(od.odid()) : 0;

    msgDb->SetSeqNomb(msg->seqno());
    msgDb->SetSql(id > 0 ? "UpdatePlan" : "InsertPlan");
    if (id > 0)
    {
        msgDb->SetCondition("id", od.odid());
        msgDb->SetRead(INCREASEField, id);
    }

    msgDb->SetWrite("landId", od.pdid());
    msgDb->SetWrite("planuser", od.registerid());
    if(!od.crop().empty())
        msgDb->SetWrite("crop", od.crop());
    if (!od.drug().empty())
        msgDb->SetWrite("drug", od.drug());
    if (od.has_ridge())
        msgDb->SetWrite("ridgeSz", od.ridge());
    msgDb->SetWrite("prize", od.prize());
    if (od.has_notes())
        msgDb->SetWrite("notes", od.notes());
    msgDb->SetWrite("planTime", od.has_plantime() ? od.plantime() : Utility::msTimeTick());
    string buff;
    buff.resize(od.op().ByteSize());
    od.op().SerializeToArray(&buff.front(), buff.size());
    msgDb->SetWrite("planParam", Variant(buff.size(), buff.c_str()));

    SendMsg(msgDb);
}

void ObjectGS::_prcsReqPlan(RequestOperationDescriptions *msg)
{
    if (!msg || (!msg->has_odid() && !msg->has_pdid() && !msg->has_registerid()))
        return;

    DBMessage *msgDb = new DBMessage(this, IMessage::PlanQueryRslt);
    if (!msgDb)
        return;
    msgDb->SetSql("queryPlan", true);
    msgDb->SetSeqNomb(msg->seqno());
    bool bValid = false;
    if (msg->has_odid())
    {
        msgDb->SetCondition("id", msg->odid());
        bValid = true;
    }
    if (msg->has_pdid())
    {
        msgDb->SetCondition("landId", msg->pdid());
        bValid = true;
    }
    if (msg->has_registerid())
    {
        msgDb->SetCondition("planuser", msg->registerid());
        bValid = true;
    }

    if (bValid)
        SendMsg(msgDb);
    else
        delete msgDb;
}

void ObjectGS::_prcsDeletePlan(das::proto::DeleteOperationDescription *msg)
{
    if (!msg)
        return;

    const std::string &id = msg->odid();
    DBMessage *msgDb = new DBMessage(this, IMessage::DeletePlanRslt);
    if (!msgDb || id.empty())
        return;

    msgDb->SetSql("deletePlan");
    msgDb->SetCondition("id", id);
    msgDb->SetCondition("planuser", m_id);
    SendMsg(msgDb);

    if (auto ackDP = new AckDeleteOperationDescription)
    {
        ackDP->set_seqno(msg->seqno());
        ackDP->set_result(1);
        WaitSend(ackDP);
    }
    GetManager()->Log(0, IObjectManager::GetObjectFlagID(this), 0, "Delete mission plan %s!", id.c_str());
}

void ObjectGS::_prcsPostMission(PostOperationRoute *msg)
{
    if (!msg)
        return;
    const OperationRoute &ort = msg->or_();
    if (GS2UavMessage *ms = new GS2UavMessage(this, ort.uavid()))
    {
        ms->AttachProto(msg);
        SendMsg(ms);
        GetManager()->Log(0, IObjectManager::GetObjectFlagID(this), 0, "PostOperationRoute acreage:%f, ridges:%d!", ort.acreage(), ort.ridgebeg_size());
    }
    else
    {
        delete msg;
    }
}

void ObjectGS::_prcsReqNewGs(RequestNewGS *msg)
{
    if (!msg)
        return;

    int res = 1;
    string user = Utility::Lower(msg->userid());
    if (user != m_id)
        res = -4;
    else if (!GSOrUavMessage::IsGSUserValide(user))
        res = -2;
    else if (msg->check().empty())
        return _checkGS(m_id, msg->seqno());
    else if (msg->check() != m_check)
        res = -3;
    else if (msg->check() == m_check && !msg->password().empty())
        res = _addDatabaseUser(m_id, msg->password(), msg->seqno());

    if (res <= 1)
    {
        if (auto ack =new AckNewGS)
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

void ObjectGS::_prcsGsMessage(GroundStationsMessage *msg)
{
    if (!msg)
        return;

    if (msg->type()>RejectFriend && !IsContainsInList(m_friends, msg->to()))
    {
        if (auto ack = new AckGroundStationsMessage)
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
            db->SetSql("deleteGSFriends");
            db->SetCondition("id", GSManager::CatString(m_id, msg->to()));
            SendMsg(db);
        }
    }

    if (Gs2GsMessage *ms = new Gs2GsMessage(this, msg->to()))
    {
        ms->AttachProto(msg);
        SendMsg(ms);
    }
}

void ObjectGS::_prcsReqFriends(das::proto::RequestFriends *msg)
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

void ObjectGS::_prcsReqMissons(das::proto::RequestUavMission &msg)
{
    DBMessage *msgDb = new DBMessage(this, IMessage::QueryMissionsRslt);

    if (!msgDb)
        return;
    msgDb->SetSeqNomb(msg.seqno());
    msgDb->SetSql("queryMission", true);
    if (!GetAuth(Type_Manager))
        msgDb->SetCondition("userID", m_id);
    msgDb->SetCondition("uavID", msg.uav());
    if (msg.has_planid())
        msgDb->SetCondition("planID", msg.planid());
    if (msg.has_id())
        msgDb->SetCondition("landId", msg.id());
    if (msg.has_beg())
        msgDb->SetCondition("finishTime:>", msg.beg());
    if (msg.has_end())
        msgDb->SetCondition("finishTime:<", msg.end());

    SendMsg(msgDb);
}

void ObjectGS::_prcsReqMissonsAcreage(das::proto::RequestUavMissionAcreage &msg)
{
    DBMessage *msgDb = new DBMessage(this, IMessage::QueryMissionsAcreageRslt);
    if (!msgDb)
        return;

    msgDb->SetSeqNomb(msg.seqno());
    msgDb->SetSql("queryMissionAcreage");

    if (!GetAuth(Type_Manager))
        msgDb->SetCondition("userID", m_id);
    if (msg.has_uav())
        msgDb->SetCondition("uavID", msg.uav());
    if (msg.has_id())
        msgDb->SetCondition("landId", msg.id());
    if (msg.has_planid())
        msgDb->SetCondition("planID", msg.planid());
    if (msg.has_beg())
        msgDb->SetCondition("finishTime:>", (int64_t)msg.beg());
    if (msg.has_end())
        msgDb->SetCondition("finishTime:<", (int64_t)msg.end());

    SendMsg(msgDb);
}

void ObjectGS::_prcsReqSuspend(das::proto::RequestMissionSuspend &msg)
{
    DBMessage *msgDb = new DBMessage(this, IMessage::SuspendRslt);
    if (!msgDb)
        return;

    msgDb->SetSeqNomb(msg.seqno());
    msgDb->SetSql("querySuspend");
    msgDb->SetCondition("max.uavID", msg.uav());
    msgDb->SetCondition("max.planID", msg.planid());
    SendMsg(msgDb);
}

void ObjectGS::_prcsReqAssists(RequestOperationAssist *msg)
{
    if (!msg)
        return;

    GS2UavMessage *ms = new GS2UavMessage(this, msg->uavid());
    if (!ms)
    {
        delete msg;
        return;
    }
    ms->AttachProto(msg);
    SendMsg(ms);
}

void ObjectGS::_prcsReqABPoint(RequestABPoint *msg)
{
    if (!msg)
        return;

    GS2UavMessage *ms = new GS2UavMessage(this, msg->uavid());
    if (!ms)
    {
        delete msg;
        return;
    }
    ms->AttachProto(msg);
    SendMsg(ms);
}

void ObjectGS::_prcsReqReturn(RequestOperationReturn *msg)
{
    if (!msg)
        return;

    GS2UavMessage *ms = new GS2UavMessage(this, msg->uavid());
    if (!ms)
    {
        delete msg;
        return;
    }
    ms->AttachProto(msg);
    SendMsg(ms);
}

void ObjectGS::_checkGS(const string &user, int ack)
{
    DBMessage *msgDb = new DBMessage(this, IMessage::UserCheckRslt);
    if (!msgDb)
        return;

    msgDb->SetSeqNomb(ack);
    msgDb->SetSql("checkGS");
    msgDb->SetCondition("user", user);
    SendMsg(msgDb);
}
