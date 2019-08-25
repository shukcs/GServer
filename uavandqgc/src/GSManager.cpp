#include "GSManager.h"
#include "socketBase.h"
#include "ObjectManagers.h"
#include "das.pb.h"
#include "ProtoMsg.h"
#include "Messages.h"
#include "VGMysql.h"
#include "VGDBManager.h"
#include "DBExecItem.h"
#include "Utility.h"
#include "IMutex.h"
#include "IMessage.h"
#include "tinyxml.h"

using namespace das::proto;
using namespace google::protobuf;
using namespace std;
////////////////////////////////////////////////////////////////////////////////
//ObjectUav
////////////////////////////////////////////////////////////////////////////////

class UAVMessage : public IMessage
{
public:
    UAVMessage(IObject *sender, const std::string &idRcv)
        :IMessage(sender, idRcv, IObject::Plant, Unknown), m_msg(NULL)
    {
    }
    ~UAVMessage()
    {
        delete m_msg;
    }
    void *GetContent() const
    {
        return m_msg;
    }

    void SetContent(const google::protobuf::Message &msg)
    {
        delete m_msg;
        m_msg = NULL;
        m_tpMsg = Unknown;
        const string &name = msg.GetTypeName();
        if (name == d_p_ClassName(RequestBindUav))
        {
            m_msg = new RequestBindUav;
            m_tpMsg = BindUav;
        }
        else if (name == d_p_ClassName(PostControl2Uav))
        {
            m_msg = new PostControl2Uav;
            m_tpMsg = ControlUav;
        }
        else if (name == d_p_ClassName(RequestUploadOperationRoutes))
        {
            m_msg = new RequestUploadOperationRoutes;
            m_tpMsg = SychMission;
        }
        else if (name == d_p_ClassName(RequestUavStatus))
        {
            m_msg = new RequestUavStatus;
            m_tpMsg = QueryUav;
        }

        if (m_tpMsg != Unknown)
            m_msg->CopyFrom(msg);
    }

    void AttachProto(google::protobuf::Message *msg)
    {
        delete m_msg;
        m_msg = NULL;
        if (!msg)
            return;
        const string &name = msg->GetTypeName();
        if (name == d_p_ClassName(RequestBindUav))
            m_tpMsg = BindUav;
        else if (name == d_p_ClassName(PostControl2Uav))
            m_tpMsg = ControlUav;
        else if (name == d_p_ClassName(RequestUploadOperationRoutes))
            m_tpMsg = SychMission;
        else if (name == d_p_ClassName(RequestUavStatus))
            m_tpMsg = QueryUav;
        else
            m_tpMsg = Unknown;

        m_msg = msg;
    }

    int GetContentLength() const
    {
        return m_msg ? m_msg->ByteSize() : 0;
    }
private:
    google::protobuf::Message  *m_msg;
};
////////////////////////////////////////////////////////////////////////////////
//ObjectUav
////////////////////////////////////////////////////////////////////////////////
ObjectGS::ObjectGS(const std::string &id)
: IObject(NULL, id), m_bConnect(false)
, m_p(new ProtoMsg)
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

void ObjectGS::RespondLogin(int seqno, int res)
{
    if (m_p)
    {
        AckGSIdentityAuthentication msg;
        msg.set_seqno(seqno);
        msg.set_result(res);
        _send(msg);
    }
}

void ObjectGS::ProcessMassage(const IMessage &msg)
{
    int tp = msg.GetMessgeType();
    if ((tp == BindUavRes || tp == QueryUavRes || ControlUavRes==tp) && m_p)
    {
        if (Message *ms = (Message *)msg.GetContent())
            _send(*ms);
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
        {
            RequestGSIdentityAuthentication *msg = (RequestGSIdentityAuthentication*)m_p->GetProtoMessage();
            RespondLogin(msg->seqno(), m_pswd == msg->password() ? 1 : -1);
        }
        else if (strMsg == d_p_ClassName(PostHeartBeat))
        {
            PostHeartBeat *msg = (PostHeartBeat *)m_p->GetProtoMessage();
            AckHeartBeat hb;
            hb.set_seqno(msg->seqno());
            _send(*msg);
        }
        else if (strMsg == d_p_ClassName(RequestBindUav))
        {
            RequestBindUav *msg = (RequestBindUav *)m_p->GetProtoMessage();
            if (UAVMessage *ms = new UAVMessage(this, msg->uavid()))
            {
                ms->SetContent(*msg);
                SendMsg(ms);
            }
        }
        else if (strMsg == d_p_ClassName(RequestUavStatus))
        {
            RequestUavStatus *msg = (RequestUavStatus *)m_p->GetProtoMessage();
            _prcsReqUavs(*msg);
        }
        else if (strMsg == d_p_ClassName(PostParcelDescription))
        {
            PostParcelDescription *msg = (PostParcelDescription *)m_p->GetProtoMessage();
            _prcsPostLand(msg->pd(), msg->seqno());
        }
        else if (strMsg == d_p_ClassName(RequestParcelDescriptions))
        {
            RequestParcelDescriptions *msg = (RequestParcelDescriptions *)m_p->GetProtoMessage();
            _ackLandOfGs(msg->registerid(), msg->seqno());
        }
        else if (strMsg == d_p_ClassName(DeleteParcelDescription))
        {
            DeleteParcelDescription *msg = (DeleteParcelDescription *)m_p->GetProtoMessage();
            _prcsDeleteLand(msg->pdid(), msg->delpc()&&msg->delpsi(), msg->seqno());
        }
        else if (strMsg == d_p_ClassName(PostControl2Uav))
        {
            PostControl2Uav *msg = (PostControl2Uav *)m_p->GetProtoMessage();
            if (UAVMessage *ms = new UAVMessage(this, msg->uavid()))
            {
                ms->SetContent(*msg);
                SendMsg(ms);
            }
        }
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
    {
        m_mtxSend->Lock();
        m_p->SetSended(sended);
        m_mtxSend->Unlock();
    }
}

void ObjectGS::_prcsReqUavs(const RequestUavStatus &msg)
{
    if (UAVMessage *ms = new UAVMessage(this, string()))
    {
        ms->SetContent(msg);
        SendMsg(ms);
    }
}

void ObjectGS::_prcsPostLand(const das::proto::ParcelDescription &msg, int ack)
{
    ExecutItem *itemLand = NULL;
    ExecutItem *itemOwner = NULL;
    bool bSuc;
    uint64_t nLand = Utility::str2int(msg.has_psiid() ? msg.psiid() : "", 10, &bSuc);
    if (bSuc)
        itemLand = VGDBManager::Instance().GetSqlByName("updateLand");
    else
        itemLand = VGDBManager::Instance().GetSqlByName("insertLand");

    uint64_t nCon = Utility::str2int(msg.has_pc() ? msg.pc().id() : "", 10, &bSuc);
    if (bSuc)
        itemOwner = VGDBManager::Instance().GetSqlByName("updateOwner");
    else
        itemOwner = VGDBManager::Instance().GetSqlByName("insertOwner");

    if (itemOwner && itemLand)
    {
        itemOwner->ClearData();
        itemLand->ClearData();
        nCon = _saveContact(msg, *itemOwner, nCon);
        if (nCon)
        {
            if (FiledValueItem *fd = itemLand->GetWriteItem("ownerID"))
                fd->InitOf(nCon);
            nLand = _saveLand(msg, *itemLand, nLand);
        }
    }

    AckPostParcelDescription appd;
    appd.set_seqno(ack);
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
        fd->InitBuff(pc.name().length()+1, pc.name().c_str());
    fd = item.GetWriteItem("birthdate");
    if (fd && pc.has_birthdate())
        fd->InitOf(pc.birthdate());
    fd = item.GetWriteItem("address");
    if (fd && pc.has_address())
        fd->InitBuff(pc.address().length() + 1, pc.address().c_str());
    fd = item.GetWriteItem("mobileno");
    if (fd && pc.has_mobileno())
        fd->SetParam(pc.mobileno());
    fd = item.GetWriteItem("phoneno");
    if (fd && pc.has_phoneno())
        fd->SetParam(pc.phoneno());
    fd = item.GetWriteItem("weixin");
    if (fd && pc.has_weixin())
        fd->InitBuff(pc.weixin().length() + 1, pc.weixin().c_str());

    if (item.GetType()==ExecutItem::Update && (fd=item.GetWriteItem("id")))
        fd->InitOf(uint64_t(id));

    VGMySql *sql = ((GSManager*)GetManager())->GetMySql();
    if (sql && sql->Execut(&item) && item.GetIncrement())
        return item.GetIncrement()->GetValue();

    return id;
}

uint64_t ObjectGS::_saveLand(const das::proto::ParcelDescription &msg, ExecutItem &item, uint64_t id)
{
    FiledValueItem *fd = item.GetWriteItem("name");
    if (fd && msg.has_name())
        fd->InitBuff(msg.name().length() + 1, msg.name().c_str());
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

    VGMySql *sql = ((GSManager*)GetManager())->GetMySql();
    if (sql && sql->Execut(&item) && item.GetIncrement())
        return item.GetIncrement()->GetValue();
    return id;
}

void ObjectGS::_ackLandOfGs(const std::string &user, int id)
{
    AckRequestParcelDescriptions ack;
    ack.set_seqno(id);
    int res = 0;
    ExecutItem *item = VGDBManager::Instance().GetSqlByName("queryLand");
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
    if (fd && fd->GetValidLen() > 0)
        pd->set_acreage(fd->GetValue<float>());
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

void ObjectGS::_prcsDeleteLand(const std::string &id, bool del, int ack)
{
    AckDeleteParcelDescription ackDD;
    ackDD.set_seqno(ack);
    int res = 0;
    if (del && id.length()>0)
    {
        if (ExecutItem *item = VGDBManager::Instance().GetSqlByName("deleteLand"))
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

void ObjectGS::_send(const google::protobuf::Message &msg)
{
    if (m_p)
    {
        m_mtxSend->Lock();
        m_p->SendProto(msg, m_sock);
        m_mtxSend->Unlock();
    }
}
////////////////////////////////////////////////////////////////////////////////
//GSManager
////////////////////////////////////////////////////////////////////////////////
GSManager::GSManager() : IObjectManager(0)
    ,m_sqlEng(new VGMySql), m_p(new ProtoMsg)
{
    _parseConfig();
}

GSManager::~GSManager()
{
    delete m_sqlEng;
    delete m_p;
}

VGMySql *GSManager::GetMySql() const
{
    return m_sqlEng;
}

int GSManager::GetObjectType() const
{
    return IObject::GroundStation;
}

IObject *GSManager::ProcessReceive(ISocket *s, const char *buf, int &len)
{
    int pos = 0;
    int l = len;
    len = 0;
    IObject *o = NULL;
    while (m_p && m_p->Parse(buf + pos, l))
    {
        pos = l;
        l = len - pos;
        if (m_p->GetMsgName() == d_p_ClassName(RequestGSIdentityAuthentication))
        {
            RequestGSIdentityAuthentication *msg = (RequestGSIdentityAuthentication *)m_p->GetProtoMessage();
            o = _checkLogin(s, *msg);
            len = pos;

            if (ISocketManager *m = s->GetManager())
                m->Log(0, msg->userid(), 0, o ? "login success" : "login fail");

            break;
        }
    }
    return o;
}

IObject *GSManager::_checkLogin(ISocket *s, const das::proto::RequestGSIdentityAuthentication &rgi)
{
    string usr = rgi.userid();
    string pswd = rgi.password();
    ObjectGS *o = (ObjectGS*)GetObjectByID(usr);
    int res = -3;
    if (o)
    {
        if (!o->IsConnect() && o->m_pswd == pswd)
        {
            res = 1;
            o->SetSocket(s);
        }
    }
    else if (ExecutItem *item = VGDBManager::Instance().GetSqlByName("queryGSInfo"))
    {
        item->ClearData();
        if (FiledValueItem *fd = item->GetConditionItem("user"))
            fd->SetParam(usr);

        if (m_sqlEng->Execut(item))
        {
            res = -1;
            FiledValueItem *fd = item->GetReadItem("pswd");
            if (fd && string((char*)fd->GetBuff(), fd->GetValidLen())==pswd)
            {
                o = new ObjectGS(usr);
                AddObject(o);
                o->SetPswd(pswd);
                res = 1;
            }

            while (m_sqlEng->GetResult());
        }
    }

    AckGSIdentityAuthentication msg;
    msg.set_seqno(rgi.seqno());
    msg.set_result(res);
    ProtoMsg::SendProtoTo(msg, s);

    return o;
}

void GSManager::_parseConfig()
{
    TiXmlDocument doc;
    doc.LoadFile("GSInfo.xml");
    _parseMySql(doc);

    const TiXmlElement *rootElement = doc.RootElement();
    const TiXmlNode *node = rootElement ? rootElement->FirstChild("GSManager") : NULL;
    const TiXmlElement *cfg = node ? node->ToElement() : NULL;
    if (cfg)
    {
        const char *tmp = cfg->Attribute("thread");
        int n = tmp ? (int)Utility::str2int(tmp) : 1;
        InitThread(n);
    }
}

void GSManager::_parseMySql(const TiXmlDocument &doc)
{
    StringList tables;
    VGDBManager &dbMgr = VGDBManager::Instance();
    string db = dbMgr.Load(doc, tables);
    if (!m_sqlEng)
        return;

    m_sqlEng->ConnectMySql(dbMgr.GetMysqlHost(db),
        dbMgr.GetMysqlPort(db),
        dbMgr.GetMysqlUser(db),
        dbMgr.GetMysqlPswd(db));

    m_sqlEng->EnterDatabase(db, dbMgr.GetMysqlCharSet(db));
    for (const string &tb : tables)
    {
        m_sqlEng->CreateTable(VGDBManager::Instance().GetTableByName(tb));
    }
    for (const string &trigger : VGDBManager::Instance().GetTriggers())
    {
        m_sqlEng->CreateTrigger(VGDBManager::Instance().GetTriggerByName(trigger));
    }
}

DECLARE_MANAGER_ITEM(GSManager)