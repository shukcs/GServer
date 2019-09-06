#include "ObjectGS.h"
#include "socketBase.h"
#include "das.pb.h"
#include "ProtoMsg.h"
#include "Messages.h"
#include "Utility.h"
#include "IMutex.h"
#include "IMessage.h"
#include "GSManager.h"
#include "VGMysql.h"
#include "VGDBManager.h"
#include "DBExecItem.h"

using namespace das::proto;
using namespace google::protobuf;
using namespace std;

enum GSAuthorizeType
{
    Type_Common = 1,
    Type_UavManager = Type_Common << 1,
};

////////////////////////////////////////////////////////////////////////////////
//ObjectUav
////////////////////////////////////////////////////////////////////////////////
ObjectGS::ObjectGS(const std::string &id)
: IObject(NULL, id), m_bConnect(false)
, m_auth(1), m_p(new ProtoMsg)
{
    if (m_p)
        m_p->InitSize();
}

ObjectGS::~ObjectGS()
{
    delete m_p;
}

bool ObjectGS::IsConnect() const
{
    return m_sock!=NULL;
}

int ObjectGS::GetObjectType() const
{
    return IObject::GroundStation;
}

void ObjectGS::OnConnected(bool bConnected)
{
    if (m_sock && !bConnected)
    {
        if(ISocketManager *m = m_sock->GetManager())
        {
            m->Log(0, GetObjectID(), 0, "disconnect");
            m_sock->Close();
            m_sock = NULL;
        }
    }
    m_bConnect = bConnected;
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

void ObjectGS::_prcsLogin(RequestGSIdentityAuthentication *msg)
{
    if (msg && m_p)
    {
        AckGSIdentityAuthentication ack;
        ack.set_seqno(msg->seqno());
        ack.set_result(m_pswd == msg->password() ? 1 : -1);
        _send(ack);
    }
}

void ObjectGS::_prcsHeartBeat(das::proto::PostHeartBeat *msg)
{
    AckHeartBeat ahb;
    ahb.set_seqno(msg->seqno());
    _send(ahb);
}

void ObjectGS::ProcessMassage(const IMessage &msg)
{
    int tp = msg.GetMessgeType();
    if(m_p)
    {
        switch (tp)
        {
        case BindUavRes:
        case QueryUavRes:
        case ControlUavRes:
        case UavAllocationRes:
        case PushUavSndInfo:
        case ControlGs:
            if (Message *ms = (Message *)msg.GetContent())
                _send(*ms);
            break;
        default:
            break;
        }
    }
}

int ObjectGS::ProcessReceive(void *buf, int len)
{
    int pos = 0;
    int l = len;
    while (m_p && m_p->Parse((char*)buf + pos, l))
    {
        pos += l;
        l = len - pos;
        string strMsg = m_p->GetMsgName();

        if (strMsg == d_p_ClassName(RequestGSIdentityAuthentication))
            _prcsLogin((RequestGSIdentityAuthentication*)m_p->GetProtoMessage());
        else if (strMsg == d_p_ClassName(PostHeartBeat))
            _prcsHeartBeat((PostHeartBeat *)m_p->GetProtoMessage());
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
            _prcsDelPlan(( DeleteOperationDescription*)m_p->GetProtoMessage());
    }
    pos += l;
    return pos;
}

int ObjectGS::GetSenLength() const
{
    if (m_p)
        return m_p->RemaimedLength();

    return 0;
}

int ObjectGS::CopySend(char *buf, int sz, unsigned form)
{
    if (m_p)
        return m_p->CopySend(buf, sz, form);

    return 0;
}

void ObjectGS::SetSended(int sended /*= -1*/)
{
    if (m_p)
        m_p->SetSended(sended);
}

void ObjectGS::_prcsReqUavs(RequestUavStatus *msg)
{
    if (!msg)
        return;

    if (UAVMessage *ms = new UAVMessage(this, string()))
    {
        ms->AttachProto(msg);
        SendMsg(ms);
    }
}

void ObjectGS::_prcsReqBind(das::proto::RequestBindUav *msg)
{
    if (!msg)
        return;

    if (UAVMessage *ms = new UAVMessage(this, msg->uavid()))
    {
        ms->AttachProto(msg);
        SendMsg(ms);
    }
}

void ObjectGS::_prcsControl2Uav(das::proto::PostControl2Uav *msg)
{
    if (!msg)
        return;

    if (UAVMessage *ms = new UAVMessage(this, msg->uavid()))
    {
        ms->AttachProto(msg);
        SendMsg(ms);
    }
}

void ObjectGS::_prcsPostLand(PostParcelDescription *msg)
{
    if (!msg)
        return;

    const ParcelDescription &land = msg->pd();
    ExecutItem *itemLand = NULL;
    ExecutItem *itemOwner = NULL;
    bool bSuc;
    uint64_t nLand = Utility::str2int(land.has_psiid() ? land.psiid() : "", 10, &bSuc);
    if (bSuc)
        itemLand = VGDBManager::GetSqlByName("updateLand");
    else
        itemLand = VGDBManager::GetSqlByName("insertLand");

    uint64_t nCon = Utility::str2int(land.has_pc() ? land.pc().id() : "", 10, &bSuc);
    if (bSuc)
        itemOwner = VGDBManager::GetSqlByName("updateOwner");
    else
        itemOwner = VGDBManager::GetSqlByName("insertOwner");

    if (itemOwner && itemLand)
    {
        itemOwner->ClearData();
        itemLand->ClearData();
        nCon = _saveContact(land, *itemOwner, nCon);
        if (nCon)
        {
            if (FiledValueItem *fd = itemLand->GetWriteItem("ownerID"))
                fd->InitOf(nCon);
            nLand = _saveLand(land, *itemLand, nLand);
        }
    }

    AckPostParcelDescription appd;
    appd.set_seqno(msg->seqno());
    appd.set_result((nLand > 0 && nCon > 0) ? 1 : 0);
    appd.set_psiid(Utility::bigint2string(nLand));
    appd.set_pcid(Utility::bigint2string(nCon));
    appd.set_pdid(Utility::bigint2string(nLand));
    _send(appd);
}

uint64_t ObjectGS::_saveContact(const das::proto::ParcelDescription &msg, ExecutItem &item, uint64_t id)
{
    const ParcelContracter &pc = msg.pc();
    FiledValueItem *fd = item.GetWriteItem("name");
    if (fd && pc.has_name())
        fd->SetParam(pc.name());
    fd = item.GetWriteItem("birthdate");
    if (fd && pc.has_birthdate())
        fd->InitOf(pc.birthdate());
    fd = item.GetWriteItem("address");
    if (fd && pc.has_address())
        fd->SetParam(pc.address());
    fd = item.GetWriteItem("mobileno");
    if (fd && pc.has_mobileno())
        fd->SetParam(pc.mobileno());
    fd = item.GetWriteItem("phoneno");
    if (fd && pc.has_phoneno())
        fd->SetParam(pc.phoneno());
    fd = item.GetWriteItem("weixin");
    if (fd && pc.has_weixin())
        fd->SetParam(pc.weixin());

    if (item.GetType()==ExecutItem::Update && (fd=item.GetWriteItem("id")))
        fd->InitOf(uint64_t(id));

    return _executeInsertSql(&item);
}

uint64_t ObjectGS::_saveLand(const das::proto::ParcelDescription &msg, ExecutItem &item, uint64_t id)
{
    FiledValueItem *fd = item.GetWriteItem("name");
    if (fd && msg.has_name())
        fd->SetParam(msg.name());
    fd = item.GetWriteItem("gsuser");
    if (fd && msg.has_registerid())
        fd->SetParam(msg.registerid());
    fd = item.GetWriteItem("acreage");
    if (fd && msg.has_acreage())
        fd->InitOf(msg.acreage());

    if (msg.has_coordinate())
    {
        if (FiledValueItem *tdTmp = item.GetWriteItem("lat"))
            tdTmp->InitOf(double(msg.coordinate().latitude() / 1e7));
        if (FiledValueItem *tdTmp = item.GetWriteItem("lon"))
            tdTmp->InitOf(double(msg.coordinate().longitude() / 1e7));
    }

    fd = item.GetWriteItem("boundary");
    if (fd && msg.has_psi())
    {
        fd->InitBuff(msg.psi().ByteSize());
        msg.psi().SerializeToArray(fd->GetBuff(), fd->GetMaxLen());
    }

    int64_t tmp = _executeInsertSql(&item);
    return tmp > 0 ? tmp : id;
}

void ObjectGS::_prcsReqLand(RequestParcelDescriptions *msg)
{
    if (!msg)
        return;

    AckRequestParcelDescriptions ack;
    ack.set_seqno(msg->seqno());

    string user = msg->registerid();
    int res = 0;
    ExecutItem *item = VGDBManager::GetSqlByName("queryLand");
    if (user.length()>0 && item)
    {
        item->ClearData();
        if (FiledValueItem *tdTmp = item->GetConditionItem("LandInfo.gsuser"))
            tdTmp->SetParam(user);

        VGMySql *sql = ((GSManager*)GetManager())->GetMySql();
        if (sql->Execut(item))
        {
            do {
                _initialParcelDescription(ack.add_pds(), *item);
                item->ClearData();
            } while (sql->GetResult());
            res = 1;
        }
    }
    ack.set_result(res);
    _send(ack);
}

void ObjectGS::_initialParcelDescription(ParcelDescription *pd, const ExecutItem &item)
{
    if (!pd)
        return;
    
    if (ParcelContracter *pc = _transPC(item))
        pd->set_allocated_pc(pc);

    FiledValueItem *fd = item.GetReadItem("LandInfo.name");
    if (fd && fd->GetValidLen() > 0)
        pd->set_name((char*)fd->GetBuff());
    fd = item.GetReadItem("LandInfo.gsuser");
    if (fd && fd->GetValidLen() > 0)
        pd->set_registerid((char*)fd->GetBuff());
    fd = item.GetReadItem("LandInfo.acreage");
    pd->set_acreage((fd && fd->GetValidLen() > 0)?fd->GetValue<float>():0);
    fd = item.GetReadItem("LandInfo.lat");
    FiledValueItem *tdTmp = item.GetReadItem("LandInfo.lon");
    if (fd && fd->GetValidLen() > 0 && tdTmp&&tdTmp->GetValidLen() > 0)
    {
        double lat = fd->GetValue<double>();
        double lon = fd->GetValue<double>();
        if(fabs(lat)<=90 && fabs(180))
        {
            Coordinate *c = new Coordinate;
            c->set_altitude(0);
            c->set_latitude(int(lat*1e7));
            c->set_longitude(int(lon*1e7));
            pd->set_allocated_coordinate(c);
        }
    }

    fd = item.GetReadItem("LandInfo.id");
    string id = (fd && fd->GetValidLen() > 0) ? Utility::bigint2string(fd->GetValue()) : "0";

    fd = item.GetReadItem("LandInfo.boundary");
    if (fd && fd->GetValidLen() > 0)
    {
        ParcelSurveyInformation *psi = new ParcelSurveyInformation;
        psi->ParseFromArray(fd->GetBuff(), fd->GetValidLen());
        psi->set_id(id);
        pd->set_id(id);
        pd->set_allocated_psi(psi);
    }
}

ParcelContracter *ObjectGS::_transPC(const ExecutItem &item)
{
    ParcelContracter *pc = new ParcelContracter();
    if (!pc)
        return NULL;

    FiledValueItem *fd = item.GetReadItem("OwnerInfo.name");
    if (fd && fd->GetValidLen() > 0)
        pc->set_name(string((char*)fd->GetBuff(), fd->GetValidLen()));

    fd = item.GetReadItem("OwnerInfo.birthdate");
    int64_t tm = (fd && fd->GetValidLen() > 0) ? fd->GetValue() : Utility::secTimeCount();
    pc->set_birthdate(tm);

    fd = item.GetReadItem("OwnerInfo.address");
    string str = (fd && fd->GetValidLen() > 0) ? string((char*)fd->GetBuff(), fd->GetValidLen()) : "";
    pc->set_address(str);

    fd = item.GetReadItem("OwnerInfo.mobileno");
    str = (fd && fd->GetValidLen() > 0) ? string((char*)fd->GetBuff(), fd->GetValidLen()) : "";
    pc->set_mobileno(str);
    if (fd && fd->GetValidLen() > 0)
    fd = item.GetReadItem("OwnerInfo.phoneno");
    if (fd && fd->GetValidLen() > 0)
        pc->set_phoneno(string((char*)fd->GetBuff(), fd->GetValidLen()));
    fd = item.GetReadItem("weixin");
    if (fd && fd->GetValidLen() > 0)
        pc->set_weixin(string((char*)fd->GetBuff(), fd->GetValidLen()));

    return pc;
}

int64_t ObjectGS::_savePlan(ExecutItem *item, const OperationDescription &msg)
{
    if (!item)
        return -1;

    item->ClearData();
    FiledValueItem *fd = item->GetWriteItem("landId");
    if (!fd || msg.pdid().empty())
        return -1;
    fd->SetParam(msg.pdid());

    fd = item->GetWriteItem("planuser");
    if (!fd || msg.registerid().empty())
        return -1;
    fd->SetParam(msg.registerid());

    fd = item->GetWriteItem("crop");
    if (!fd || msg.crop().empty())
        return -1;
    fd->SetParam(msg.crop());

    fd = item->GetWriteItem("drug");
    if (!fd || msg.drug().empty())
        return -1;
    fd->SetParam(msg.drug());

    fd = item->GetWriteItem("prize");
    if (!fd)
        return -1;
    fd->InitOf(msg.prize());

    fd = item->GetWriteItem("notes");
    if (fd && msg.has_notes())
        fd->SetParam(msg.notes());
    if (FiledValueItem *fdTmp = item->GetWriteItem("planTime"))
        fdTmp->InitOf(msg.has_plantime() ? msg.plantime() : Utility::msTimeTick());
    if (FiledValueItem *fdTmp = item->GetWriteItem("planParam"))
    {
        fdTmp->InitBuff(msg.op().ByteSize());
        msg.op().SerializeToArray(fdTmp->GetBuff(), fdTmp->GetMaxLen());
    }
    return _executeInsertSql(item);
}

void ObjectGS::_initialPlan(das::proto::OperationDescription *msg, const ExecutItem &item)
{
    FiledValueItem *fd = item.GetReadItem("id");
    if (fd && fd->GetValidLen() > 0)
        msg->set_odid(Utility::bigint2string(fd->GetValue()));
    fd = item.GetReadItem("landId");
    if (fd && fd->GetValidLen() > 0)
        msg->set_pdid(Utility::bigint2string(fd->GetValue()));
    fd = item.GetReadItem("planTime");
    if (fd && fd->GetValidLen() > 0)
        msg->set_plantime(fd->GetValue());
    fd = item.GetReadItem("planuser");
    if (fd && fd->GetValidLen() > 0)
        msg->set_registerid((char*)fd->GetBuff());
    fd = item.GetReadItem("notes");
    if (fd && fd->GetValidLen() > 0)
        msg->set_notes((char*)fd->GetBuff());
    fd = item.GetReadItem("crop");
    if (fd && fd->GetValidLen() > 0)
        msg->set_crop((char*)fd->GetBuff());
    fd = item.GetReadItem("drug");
    if (fd && fd->GetValidLen() > 0)
        msg->set_drug((char*)fd->GetBuff());
    fd = item.GetReadItem("prize");
    if (fd && fd->GetValidLen() > 0)
        msg->set_prize(fd->GetValue<float>());

    fd = item.GetReadItem("planParam");
    if (fd && fd->GetValidLen() > 0)
    {
        OperationPlan *op = new OperationPlan;
        op->ParseFromArray(fd->GetBuff(), fd->GetValidLen());
        msg->set_allocated_op(op);
    }
}

void ObjectGS::_prcsDeleteLand(DeleteParcelDescription *msg)
{
    if (msg)
        return;

    const std::string &id = msg->pdid();
    AckDeleteParcelDescription ackDD;
    ackDD.set_seqno(msg->seqno());
    int res = 0;
    if (msg->delpc() && msg->delpsi() && !id.empty())
    {
        if (ExecutItem *item = VGDBManager::GetSqlByName("deleteLand"))
        {
            item->ClearData();
            FiledValueItem *fd = item->GetConditionItem("LandInfo.id");
            if (fd)
                fd->SetParam(id);

            VGMySql *sql = ((GSManager*)GetManager())->GetMySql();
            if (sql && sql->Execut(item))
                res = 1;
        }
    }

    ackDD.set_result(res);
    _send(ackDD);
}

void ObjectGS::_prcsUavIDAllication(das::proto::RequestIdentityAllocation *msg)
{
    if (!msg)
        return;

    if (m_auth & Type_UavManager)
    {
        if (UAVMessage *ms = new UAVMessage(this, string()))
        {
            ms->AttachProto(msg);
            SendMsg(ms);
            return;
        }
    }

    AckIdentityAllocation ack;
    ack.set_seqno(msg->seqno());
    ack.set_result(0);
    _send(ack);
}

void ObjectGS::_prcsPostPlan(PostOperationDescription *msg)
{
    if (!msg)
        return;

    int64_t res = _savePlan(VGDBManager::GetSqlByName("InsertPlan"), msg->od());
    AckPostOperationDescription ack;
    ack.set_seqno(msg->seqno());
    ack.set_result(res > 0 ? 1 : 0);

    if(res>0)
        ack.set_odid(Utility::bigint2string(res));

    _send(ack);
}

void ObjectGS::_prcsReqPlan(RequestOperationDescriptions *msg)
{
    if (!msg || (!msg->has_odid() && !msg->has_pdid() && !msg->has_registerid()))
        return;

    AckRequestOperationDescriptions ack;
    ack.set_seqno(msg->seqno());
    int res = 0;
    if (ExecutItem *item = VGDBManager::GetSqlByName("queryPlan"))
    {
        item->ClearData();
        if (msg->has_odid())
        {
            if (FiledValueItem *tdTmp = item->GetConditionItem("id"))
                tdTmp->SetParam(msg->odid());
        }
        if (msg->has_pdid())
        {
            if (FiledValueItem *tdTmp = item->GetConditionItem("landId"))
                tdTmp->SetParam(msg->pdid());
        }
        if (msg->has_registerid())
        {
            if (FiledValueItem *tdTmp = item->GetConditionItem("planuser"))
                tdTmp->SetParam(msg->registerid());
        }
       
        VGMySql *sql = ((GSManager*)GetManager())->GetMySql();
        if (sql->Execut(item))
        {
            do {
                _initialPlan(ack.add_ods(), *item);
                item->ClearData();
            } while (sql->GetResult());
            res = 1;
        }
    }
    ack.set_result(res);
    _send(ack);
}

void ObjectGS::_prcsDelPlan(das::proto::DeleteOperationDescription *msg)
{
    if (!msg)
        return;

    const std::string &id = msg->odid();
    AckDeleteOperationDescription ackDD;
    ackDD.set_seqno(msg->seqno());
    int res = 0;
    ExecutItem *item = VGDBManager::GetSqlByName("deletePlan");
    if (item && !id.empty())
    {
        item->ClearData();
        FiledValueItem *fd = item->GetConditionItem("id");
        if (fd)
            fd->SetParam(id);

        VGMySql *sql = ((GSManager*)GetManager())->GetMySql();
        if (sql && sql->Execut(item))
            res = 1;
    }

    ackDD.set_result(res);
    _send(ackDD);
}

void ObjectGS::_send(const google::protobuf::Message &msg)
{
    if (m_p)
        m_p->SendProto(msg, m_sock);
}

int64_t ObjectGS::_executeInsertSql(ExecutItem *item)
{
    if (!item)
        return 0;

    VGMySql *sql = ((GSManager*)GetManager())->GetMySql();
    if (sql && sql->Execut(item) && item->GetIncrement())
        return item->GetIncrement()->GetValue();

    return 0;
}