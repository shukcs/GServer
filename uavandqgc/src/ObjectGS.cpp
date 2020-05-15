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

#define WRITE_BUFFLEN 1024 * 8
#define MAXLANDRECORDS 200
#define MAXPLANRECORDS 200
using namespace das::proto;
using namespace google::protobuf;
using namespace std;
#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif
////////////////////////////////////////////////////////////////////////////////
//ObjectUav
////////////////////////////////////////////////////////////////////////////////
ObjectGS::ObjectGS(std::string &id): ObjectAbsPB(id)
, m_auth(1), m_bInitFriends(false), m_countLand(0)
, m_countPlan(0)
{
    SetBuffSize(WRITE_BUFFLEN);
}

ObjectGS::~ObjectGS()
{
}

void ObjectGS::OnConnected(bool bConnected)
{
    ObjectAbsPB::OnConnected(bConnected);
    if (bConnected && m_sock)
        m_sock->ResizeBuff(WRITE_BUFFLEN);

    if (!bConnected && !m_check.empty())
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

bool ObjectGS::GetAuth(GSAuthorizeType auth) const
{
    return (auth & m_auth) == auth;
}

void ObjectGS::SetSeq(int seq)
{
    m_seq = seq;
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
            send(ack);
        }
    }
}

void ObjectGS::_prcsHeartBeat(das::proto::PostHeartBeat *msg)
{
    if (auto ahb = new AckHeartBeat)
    {
        ahb->set_seqno(msg->seqno());
        send(ahb);
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
        send(ack);
    }
}

void ObjectGS::ProcessMessage(IMessage *msg)
{
    int tp = msg->GetMessgeType();
    if(m_p)
    {
        switch (tp)
        {
        case IMessage::BindUavRes:
        case IMessage::QueryUavRes:
        case IMessage::ControlUavRes:
        case IMessage::SychMissionRes:
        case IMessage::PostORRes:
        case IMessage::UavAllocationRes:
        case IMessage::PushUavSndInfo:
        case IMessage::ControlGs:
            process2GsMsg(((GSOrUavMessage *)msg)->GetProtobuf());
            break;
        case IMessage::Gs2GsMsg:
        case IMessage::Gs2GsAck:
            processGs2Gs(*(Message*)msg->GetContent(), tp);
            break;
        case IMessage::UavsQueryRslt:
            processUavsInfo(*(DBMessage*)msg);
            break;
        case IMessage::UavBindRslt:
            processBind(*(DBMessage*)msg);
            break;
        case IMessage::GSInsertRslt:
            processGSInsert(*(DBMessage*)msg);
            break;
        case IMessage::GSQueryRslt:
            processGSInfo(*(DBMessage*)msg);
            break;
        case IMessage::GSCheckRslt:
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

void ObjectGS::PrcsProtoBuff()
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
    else if (strMsg == d_p_ClassName(RequestBindUav))
        _prcsReqBind((RequestBindUav *)m_p->DeatachProto());
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
}

void ObjectGS::process2GsMsg(const Message *msg)
{
    if (!msg)
        return;

    if (Message *ms = msg->New())
    {
        ms->CopyFrom(*msg);
        send(ms, true);
    }
}

void ObjectGS::processGs2Gs(const Message &msg, int tp)
{
    if (tp == IMessage::Gs2GsMsg)
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
        else if (gsmsg->type() == AgreeFriend)
            m_friends.push_back(gsmsg->from());
    }

    process2GsMsg(&msg);
}

void ObjectGS::processBind(const DBMessage &msg)
{
    auto proto =new AckRequestBindUav;
    bool res = msg.GetRead(EXECRSLT, 1).ToBool();

    if(proto)
    {
        proto->set_seqno(msg.GetSeqNomb());
        proto->set_opid(msg.GetRead("binded").ToInt8());
        proto->set_result(res ? 0 : 1);
    }
    if (res)
    { 
        if(UavStatus *s = new UavStatus)
        {
            ObjectUav uav(msg.GetRead("id").ToString());
            ObjectUav::InitialUAV(msg, uav);
            uav.TransUavStatus(*s);
            if (proto)
                proto->set_allocated_status(s);
        }
    }
    send(proto, true);
}

void ObjectGS::processUavsInfo(const DBMessage &msg)
{
    auto ack = new AckRequestUavStatus;
    if (!ack)
        return;

    auto ids = msg.GetRead("id").GetVarList<string>();
    auto bindeds = msg.GetRead("binded").GetVarList<int8_t>();
    auto binders = msg.GetRead("binder").GetVarList<string>();
    auto lats = msg.GetRead("lat").GetVarList<double>();
    auto lons = msg.GetRead("lon").GetVarList<double>();
    auto timeBinds = msg.GetRead("timeBind").GetVarList<int64_t>();
    auto valids = msg.GetRead("valid").GetVarList<int64_t>();
    auto authChecks = msg.GetRead("authCheck").GetVarList<string>();
    auto sims = msg.GetRead("simID").GetVarList<string>();

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
        us->set_uavid(*idItr);
        string binder;
        if (binderItr != binders.end())
        {
            binder = *binderItr;
            us->set_binder(binder);
            ++binderItr;
        }
        bool bBind = bindedItr != bindeds.end() ? *bindedItr == 1 : false;
        if (bindedItr != bindeds.end())
        {
            bBind = *bindedItr == 1;
            ++bindedItr;
        }
        us->set_binded(bBind);
        us->set_time(timeBindItr != timeBinds.end() ? *timeBindItr : 0);
        if (timeBindItr != timeBinds.end())
            ++timeBindItr;
        us->set_online(false);

        bool bAuth = (binder == m_id && bBind) || GetAuth(ObjectGS::Type_UavManager);
        if (authCheckItr != authChecks.end() && bAuth)
        {
            us->set_authstring(*authCheckItr);
            ++authCheckItr;
        }
        if (validItr != valids.end())
        {
            us->set_deadline(*validItr);
            ++validItr;
        }

        if (latItr != lats.end() && lonItr != lons.end())
        {
            GpsInformation *gps = new GpsInformation;
            gps->set_latitude(int(*latItr*1e7));
            gps->set_longitude(int(*lonItr*1e7));
            gps->set_altitude(0);
            us->set_allocated_pos(gps);
            latItr++;
            lonItr++;
        }
        if (simItr != sims.end())
        {
            us->set_simid(*simItr);
            ++simItr;
        }
    }
    send(ack, true);
}

void ObjectGS::processGSInfo(const DBMessage &msg)
{
    m_stInit = msg.GetRead(EXECRSLT).ToBool() ? Initialed : InitialFail;
    string pswd = msg.GetRead("pswd").ToString();
    m_auth = msg.GetRead("auth").ToInt32();
    bool bLogin = pswd == m_pswd;
    if (!bLogin)
        Release();

    OnLogined(bLogin);
    if (auto ack = new AckGSIdentityAuthentication)
    {
        ack->set_seqno(m_seq);
        ack->set_result(bLogin ? 1 : -1);
        ack->set_auth(m_auth);
        send(ack, true);
    }
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
            m_check = GSOrUavMessage::GenCheckString();
            ack->set_check(m_check);
        }
        ack->set_result(bExist ? 0 : 1);
        send(ack, true);
        if (bExist)
            Release();
    }
}

void ObjectGS::processPostLandRslt(const DBMessage &msg)
{
    int64_t nCon = msg.GetRead(INCREASEField, 1).ToInt64();
    int64_t nLand = msg.GetRead(INCREASEField, 2).ToInt64();
    if (auto appd = new AckPostParcelDescription)
    {
        appd->set_seqno(msg.GetSeqNomb());
        appd->set_result((nLand > 0 && nCon > 0) ? 1 : 0);
        appd->set_psiid(Utility::bigint2string(nLand));
        appd->set_pcid(Utility::bigint2string(nCon));
        appd->set_pdid(Utility::bigint2string(nLand));
        send(appd, true);
    }
    if (nLand > 0)
        GetManager()->Log(0, GetObjectID(), 0, "Upload land %d!", nLand);

}

void ObjectGS::processFriends(const DBMessage &msg)
{
    const Variant &vUsr1 = msg.GetRead("usr1");
    const Variant &vUsr2 = msg.GetRead("usr2");
    if (vUsr1.GetType() == Variant::Type_StringList && vUsr2.GetType() == Variant::Type_StringList)
    {
        for (const string &itr : vUsr1.GetVarList<string>())
        {
            if (!itr.empty() && itr != m_id)
                m_friends.push_back(itr);
        }
        for (const string &itr : vUsr2.GetVarList<string>())
        {
            if (!itr.empty() && itr != m_id)
                m_friends.push_back(itr);
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
    auto ids = msg.GetRead("LandInfo.id").GetVarList<int64_t>();
    StringList namesLand = msg.GetRead("LandInfo.name").ToStringList();
    StringList users = msg.GetRead("LandInfo.gsuser").ToStringList();
    auto lats = msg.GetRead("LandInfo.lat").GetVarList<double>();
    auto lons = msg.GetRead("LandInfo.lon").GetVarList<double>();
    auto acreages = msg.GetRead("LandInfo.acreage").GetVarList<float>();
    StringList boundary = msg.GetRead("LandInfo.boundary").ToStringList();

    StringList namesPc = msg.GetRead("OwnerInfo.name").ToStringList();
    auto birthdayPc = msg.GetRead("OwnerInfo.birthdate").GetVarList<int64_t>();
    StringList addresses = msg.GetRead("OwnerInfo.address").ToStringList();
    StringList mobilenos = msg.GetRead("OwnerInfo.mobileno").ToStringList();
    StringList phonenos = msg.GetRead("OwnerInfo.phoneno").ToStringList();
    StringList weixins = msg.GetRead("OwnerInfo.weixin").ToStringList();

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
        if (ack->ByteSize() > 3072)
        {
            send(ack, true);
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
                pc->set_birthdate(*itrbirthday);
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
        pd->set_acreage(itracreages!=acreages.end() ? (*itracreages) : 0);
        if (itracreages != acreages.end())
            ++itracreages;
        if (itrlats != lats.end() && itrlons != lons.end())
        {
            double lat = *itrlats++;
            double lon = *itrlons++;
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
            psi->set_id(Utility::bigint2string(*itrids));
            pd->set_id(Utility::bigint2string(*itrids));
            pd->set_allocated_psi(psi);
            ++itrboundary;
        }
    }
    
    if (!m_protosSend.IsEmpty() && m_protosSend.Last()!=ack)
        send(ack, true);
}

void ObjectGS::processGSInsert(const DBMessage &msg)
{
    bool bSuc = msg.GetRead(EXECRSLT).ToBool();
    if (auto ack = new AckNewGS)
    {
        ack->set_seqno(msg.GetSeqNomb());
        ack->set_result(bSuc ? 1 : -1);
        send(ack, true);
    }
    m_check.clear();
    if (!bSuc)
        m_stInit = IObject::Uninitial;
}

void ObjectGS::processMissions(const DBMessage &msg)
{
    auto sd = new AckUavMission;
    if (!sd)
        return;
    sd->set_seqno(msg.GetSeqNomb());
    auto lands = msg.GetRead("landId").GetVarList<int64_t>();
    auto plans = msg.GetRead("planID").GetVarList<int64_t>();
    auto uavs = msg.GetRead("uavID").ToStringList();
    auto ridges = msg.GetRead("curRidge").GetVarList<int32_t>();
    auto begins = msg.GetRead("begin").GetVarList<int32_t>();
    auto ends = msg.GetRead("end").GetVarList<int32_t>();
    auto ftms = msg.GetRead("finishTime").GetVarList<int64_t>();
    auto vayages = msg.GetRead("vayage").GetVarList<float>();
    auto users = msg.GetRead("userID").ToStringList();

    auto landsItr = lands.begin();
    auto tmItr = ftms.begin();
    auto rdItr = ridges.begin();
    for (auto itr : msg.GetRead("planID").GetVarList<int64_t>())
    {
        if (landsItr == lands.end() || tmItr== ftms.end() || rdItr == ridges.end())
        {
            delete sd;
            return;
        }

        if (UavRoute *rt = sd->add_routes())
        {
            rt->set_optm(*tmItr);
            rt->set_land(Utility::bigint2string(*landsItr));
            rt->set_plan(Utility::bigint2string(itr));
        }
        ++landsItr;
    }
    send(sd, true);
}

void ObjectGS::processMissionsAcreage(const DBMessage &msg)
{
    auto sd = new AckUavMissionAcreage;
    auto acreage = msg.GetRead("sum(acreage)").ToDouble();
    if (!sd)
        return;

    sd->set_seqno(msg.GetSeqNomb());
    sd->set_acreage((float)acreage);
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
        send(ack, true);
    }

    GetManager()->Log(0, GetObjectID(), 0, "Upload mission plan %ld!", id);
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

    auto ids = msg.GetRead("id").GetVarList<int64_t>();
    auto landIds = msg.GetRead("landId").GetVarList<int64_t>();
    auto planTimes = msg.GetRead("planTime").GetVarList<int64_t>();
    auto planusers = msg.GetRead("planuser").GetVarList<string>();
    auto notes = msg.GetRead("notes").GetVarList<string>();
    auto crops = msg.GetRead("crop").GetVarList<string>();
    auto drugs = msg.GetRead("drug").GetVarList<string>();
    auto ridgeSzs = msg.GetRead("ridgeSz").GetVarList<int>();
    auto prizes = msg.GetRead("prize").GetVarList<float>();
    auto planParams = msg.GetRead("planParam").GetVarList<string>();

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
        if (ack->ByteSize() >= 4096)
        {
            send(ack, true);
            ack = new AckRequestOperationDescriptions;
            if (!ack)
                break;
            ack->set_seqno(msg.GetSeqNomb());
            ack->set_result(suc ? 1 : 0);
        }
        auto od = ack->add_ods();
        od->set_odid(Utility::bigint2string(*idItr));
        if (landIdsItr != landIds.end())
        {
            od->set_pdid(Utility::bigint2string(*landIdsItr));
            ++landIdsItr;
        }
        if (planTimesItr != planTimes.end())
        {
            od->set_plantime(*planTimesItr);
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
            od->set_ridge(*ridgeItr);
            ++ridgeItr;
        }
        if (drugsItr != drugs.end())
        {
            od->set_drug(*drugsItr);
            ++drugsItr;
        }
        if (prizesItr != prizes.end())
        {
            od->set_prize(*prizesItr);
            ++prizesItr;
        }
        if (planParamsItr != planParams.end())
        {
            OperationPlan *op = new OperationPlan;
            op->ParseFromArray(planParamsItr->c_str(), planParamsItr->size());
            od->set_allocated_op(op);
            ++planParamsItr;
        }
    }
    if (!m_protosSend.IsEmpty() && ack!=m_protosSend.Last())
        send(ack, true);
}

void ObjectGS::InitObject()
{
    if (IObject::Uninitial == m_stInit)
    {
        if (!m_check.empty())
        {
            m_stInit = IObject::Initialed;
            return;
        }

        DBMessage *msg = new DBMessage(this, IMessage::GSQueryRslt);
        if (!msg)
            return;

        if (m_check.empty())
        {
            msg->SetSql("queryGSInfo");
            msg->SetCondition("user", m_id);
            SendMsg(msg);
            m_stInit = IObject::Initialing;
            return;
        }
        m_stInit = IObject::Initialed;
    }
}

void ObjectGS::CheckTimer(uint64_t ms)
{
    ObjectAbsPB::CheckTimer(ms);
    if (ms - m_tmLastInfo > 3600000)
        Release();
    else if (m_sock && ms - m_tmLastInfo > 30000)//超时关闭
        m_sock->Close();
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

void ObjectGS::_prcsReqBind(das::proto::RequestBindUav *msg)
{
    if (!msg)
        return;

    if (GS2UavMessage *ms = new GS2UavMessage(this, msg->uavid()))
    {
        ms->AttachProto(msg);
        SendMsg(ms);
    }
    if (GetAuth(ObjectGS::Type_UavManager) && !msg->uavid().empty())
    {
        if (IObjectManager *mgr = GetManager())
        {
            if (msg->opid() == 1)
                mgr->Subcribe(m_id, msg->uavid(), IMessage::PushUavSndInfo);
            else
                mgr->Unsubcribe(m_id, msg->uavid(), IMessage::PushUavSndInfo);
        }
    }
}

void ObjectGS::_prcsControl2Uav(das::proto::PostControl2Uav *msg)
{
    if (!msg)
        return;

    if (GS2UavMessage *ms = new GS2UavMessage(this, msg->uavid()))
    {
        ms->AttachProto(msg);
        SendMsg(ms);
    }
}

void ObjectGS::_prcsPostLand(PostParcelDescription *msg)
{
    if (!msg)
        return;
    if (m_countLand >= MAXLANDRECORDS)
    {
        if (auto appd = new AckPostParcelDescription)
        { 
            appd->set_seqno(msg->seqno());
            appd->set_result(-1); //地块太多了
            send(appd);
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
        db->SetWrite("id", nCon, 1);
        db->SetWrite(INCREASEField, nCon, 1);
    }
    if (pc.has_name())
        db->SetWrite("name", pc.name(), 1);
    if (pc.has_birthdate())
        db->SetWrite("birthdate", pc.birthdate(), 1);
    if (pc.has_address())
        db->SetWrite("address", pc.address(), 1);
    if (pc.has_mobileno())
        db->SetWrite("mobileno", pc.mobileno(), 1);
    if (pc.has_phoneno())
        db->SetWrite("phoneno", pc.phoneno(), 1);
    if (pc.has_weixin())
        db->SetWrite("weixin", pc.weixin(), 1);

    nCon = Utility::str2int(land.has_pc() ? land.pc().id() : "", 10, &bSuc);
    db->AddSql(bSuc ? "updateLand" : "insertLand");
    db->SetRefFiled("ownerID");
    if (land.has_name())
        db->SetWrite("name", land.name(), 2);
    if (land.has_registerid())
        db->SetWrite("gsuser", land.registerid(), 2);
    if (land.has_acreage())
        db->SetWrite("acreage", land.acreage(), 2);
    if (bSuc)
    {
        db->SetWrite("id", nCon, 2);
        db->SetWrite(INCREASEField, nCon, 2);
    }
    if (land.has_coordinate())
    {
        db->SetWrite("lat", double(land.coordinate().latitude() / 1e7), 2);
        db->SetWrite("lat", double(land.coordinate().longitude() / 1e7), 2);
    }
    if (land.has_psi())
    {
        string buff;
        buff.resize(land.psi().ByteSize());
        land.psi().SerializeToArray(&buff.front(), buff.size());
        db->SetWrite("boundary", Variant(buff.size(), &buff.front()), 2);
    }

    SendMsg(db);
}

void ObjectGS::_prcsReqLand(RequestParcelDescriptions *msg)
{
    if (!msg || msg->registerid().empty())
        return;

    DBMessage *db = new DBMessage(this, IMessage::LandQueryRslt);
    if (!db)
        return;
    db->SetSeqNomb(msg->seqno());
    db->SetSql("queryLand", true);
    db->SetCondition("LandInfo.gsuser", m_id);

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
    {
        m_check.clear();
        m_pswd = pswd;
    }
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
    msgDb->AddSql("deletePlan");
    msgDb->SetCondition("LandInfo.id", id, 1);
    if (!GetAuth(ObjectGS::Type_ALL))
        msgDb->SetCondition("LandInfo.gsuser", m_id, 1);
    msgDb->SetCondition("landId", id, 2);
    SendMsg(msgDb);

    if (auto ackDD = new AckDeleteParcelDescription)
    {
        ackDD->set_seqno(msg->seqno());
        ackDD->set_result(1);
        send(ackDD);
    }
    GetManager()->Log(0, GetObjectID(), 0, "Delete land %s!", id.c_str());
}

void ObjectGS::_prcsUavIDAllication(das::proto::RequestIdentityAllocation *msg)
{
    if (!msg)
        return;

    if (GetAuth(Type_UavManager))
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
        send(ack);
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
            send(ack);
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
    msgDb->SetCondition("planuser", m_id, 2);
    SendMsg(msgDb);

    if (auto ackDP = new AckDeleteOperationDescription)
    {
        ackDP->set_seqno(msg->seqno());
        ackDP->set_result(1);
        send(ackDP);
    }
    GetManager()->Log(0, m_id, 0, "Delete mission plan %s!", id.c_str());
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
                m_check = GSOrUavMessage::GenCheckString();
                ack->set_check(-3 == res ? m_check : "");
            }
            ack->set_result(res);
            send(ack);
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
            send(ack);
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
            db->SetSql("insertGSFriends");
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
        send(ack);
    }
}

void ObjectGS::_prcsReqMissons(das::proto::RequestUavMission &msg)
{
    DBMessage *msgDb = new DBMessage(this, IMessage::QueryMissionsRslt);

    if (!msgDb)
        return;
    msgDb->SetSeqNomb(msg.seqno());
    msgDb->SetSql("queryMission", true);
    if (!GetAuth(ObjectGS::Type_UavManager))
        msgDb->SetCondition("userID", m_id);
    msgDb->SetCondition("uavID", msg.uav());
    if (!GetAuth(ObjectGS::Type_UavManager))
        msgDb->SetCondition("userID", m_id);
    msgDb->SetCondition("landId", msg.uav());
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
    if (!GetAuth(ObjectGS::Type_UavManager))
        msgDb->SetCondition("userID", m_id);
    msgDb->SetCondition("uavID", msg.uav());
    if (!GetAuth(ObjectGS::Type_UavManager))
        msgDb->SetCondition("userID", m_id);
    msgDb->SetCondition("landId", msg.uav());
    if (msg.has_beg())
        msgDb->SetCondition("finishTime:>", msg.beg());
    if (msg.has_end())
        msgDb->SetCondition("finishTime:<", msg.end());

    SendMsg(msgDb);
}

void ObjectGS::_checkGS(const string &user, int ack)
{
    DBMessage *msgDb = new DBMessage(this, IMessage::GSCheckRslt);
    if (!msgDb)
        return;

    msgDb->SetSeqNomb(ack);
    msgDb->SetSql("checkGS");
    msgDb->SetCondition("user", user);
    SendMsg(msgDb);
}
