#include "GSManager.h"
#include "socketBase.h"
#include "das.pb.h"
#include "ProtoMsg.h"
#include "Messages.h"
#include "VGMysql.h"
#include "VGDBManager.h"
#include "DBExecItem.h"
#include "Utility.h"
#include "IMutex.h"
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

static string catString(const string s1, const string s2)
{
    return s1 < s2 ? s1+":"+s1 : s2+":"+s1;
}
////////////////////////////////////////////////////////////////////////////////
//GSManager
////////////////////////////////////////////////////////////////////////////////
GSManager::GSManager() : IObjectManager(0),m_sqlEng(new VGMySql)
, m_p(new ProtoMsg), m_bInit(false)
{
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

int GSManager::AddDatabaseUser(const std::string &user, const std::string &pswd, int auth /*= 1*/)
{
    ExecutItem *sql = VGDBManager::GetSqlByName("insertGSInfo");
    if (!sql && (auth&ObjectGS::Type_ALL) != auth)
        return -1;

    sql->ClearData();
    FiledVal *fv = sql->GetWriteItem("user");
    if (!fv)
        return -1;
    fv->SetParam(user);

    fv = sql->GetWriteItem("pswd");
    if (!fv)
        return -1;
    fv->SetParam(pswd);
    fv = sql->GetWriteItem("auth");
    if (fv && auth>ObjectGS::Type_Common)
        fv->InitOf(auth);

    if (m_sqlEng && m_sqlEng->Execut(sql))
        return 1;

    return 0;
}

void GSManager::AddDBFriend(const string &user1, const string &user2)
{
    if (ExecutItem *item = VGDBManager::GetSqlByName("insertGSFrienfs"))
    {
        if (FiledVal *fd = item->GetWriteItem("id"))
            fd->SetParam(catString(user1, user2));
        if (FiledVal *fd = item->GetWriteItem("usr1"))
            fd->SetParam(user1);
        if (FiledVal *fd = item->GetWriteItem("usr2"))
            fd->SetParam(user2);

        m_sqlEng->Execut(item);
    }
}

void GSManager::RemoveDBFriend(const string &user1, const string &user2)
{
    if (ExecutItem *item = VGDBManager::GetSqlByName("deleteGSFrienfs"))
    {
        if (FiledVal *fd = item->GetConditionItem("id"))
        {
            fd->SetParam(catString(user1, user2));
            m_sqlEng->Execut(item);
        }
    }
}

list<string> GSManager::GetDBFriend(const std::string &user)
{
    list<string> ret;
    if (ExecutItem *item = VGDBManager::GetSqlByName("Delete"))
    {
        item->ClearData();
        if (FiledVal *fd = item->GetConditionItem("usr1"))
            fd->SetParam(user);
        if (FiledVal *fd = item->GetConditionItem("usr2"))
            fd->SetParam(user);
        if (!m_sqlEng->Execut(item))
            return ret;

        do {
            FiledVal *fd = item->GetReadItem("usr1");
            string usr = fd ? string((char*)fd->GetBuff(), fd->GetValidLen()) : string();
            if (!usr.empty() && usr != user)
            {
                ret.push_back(usr);
                continue;
            }

            fd = item->GetReadItem("usr1");
            usr = fd ? string((char*)fd->GetBuff(), fd->GetValidLen()) : string();
            if (!usr.empty() && usr != user)
                ret.push_back(usr);
        } while (m_sqlEng->GetResult());
    }
    return ret;
}

int GSManager::ExecutNewGsSql(GSManager *mgr, const string &gs)
{
    if (!mgr)
        return -1;

    //-2:名字不合法，-1:服务错误，0：名字存在
    int res = GSOrUavMessage::IsGSUserValide(gs) ? -1 : -2;
    ExecutItem *item = VGDBManager::GetSqlByName("checkGS");
    if (item && res != -2)
    {
        if (FiledVal *fd = item->GetConditionItem("user"))
        {
            fd->SetParam(gs);
            res = 1;
        }
        if (mgr->m_sqlEng->Execut(item))
        {
            while (mgr->m_sqlEng->GetResult());
            res = 0;
        }
    }
    return res;
}

int GSManager::GetObjectType() const
{
    return IObject::GroundStation;
}

IObject *GSManager::PrcsReceiveByMgr(ISocket *s, const char *buf, int &len)
{
    int pos = 0;
    int l = len;
    IObject *o = NULL;
    while (m_p && m_p->Parse(buf + pos, l))
    {
        pos += l;
        l = len - pos;
        const string &name = m_p->GetMsgName();
        if (name == d_p_ClassName(RequestGSIdentityAuthentication))
        {
            o = prcsPBLogin(s, (RequestGSIdentityAuthentication *)m_p->GetProtoMessage());
            len = pos;
        }
        else if(name == d_p_ClassName(RequestNewGS))
        {
            o = prcsPBNewGs(s, (RequestNewGS *)m_p->GetProtoMessage());
            len = pos;
        }
    }
    return o;
}

bool GSManager::PrcsPublicMsg(const IMessage &ms)
{
    if (ms.GetMessgeType() == Gs2GsMsg)
    {
        auto gsmsg = (const GroundStationsMessage *)ms.GetContent();
        if (Gs2GsMessage *msg = new Gs2GsMessage(this, ms.GetSenderID()))
        {
            auto ack = new AckGroundStationsMessage;
            ack->set_seqno(gsmsg->seqno());
            ack->set_res(0);
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
    int res = -3;
    if (o)
    {
        if (!o->IsConnect() && o->m_pswd==pswd)
        {
            res = 1;
            o->SetSocket(s);
        }
    }
    else if (ExecutItem *item = VGDBManager::GetSqlByName("queryGSInfo"))
    {
        item->ClearData();
        if (FiledVal *fd = item->GetConditionItem("user"))
            fd->SetParam(usr);

        if (m_sqlEng->Execut(item))
        {
            res = -1;
            FiledVal *fd = item->GetReadItem("pswd");
            if (fd && string((char*)fd->GetBuff(), fd->GetValidLen()) == pswd)
            {
                o = new ObjectGS(usr);
                o->SetPswd(pswd);
                if (FiledVal *fd = item->GetReadItem("auth"))
                    o->SetAuth(fd->GetValue<int>());
                res = 1;
            }

            while (m_sqlEng->GetResult());
        }
    }
    Log(0, usr, 0, "[%s]%s", s->GetHost().c_str(), 1 == res ? "login success" : "login fail");

    AckGSIdentityAuthentication msg;
    msg.set_seqno(rgi->seqno());
    msg.set_result(res);
    ProtoMsg::SendProtoTo(msg, s);

    return o;
}

IObject *GSManager::prcsPBNewGs(ISocket *s, const das::proto::RequestNewGS *msg)
{
    if(!msg || msg->userid().empty())
        return NULL;
    string userId = Utility::Lower(msg->userid());
    int res = ExecutNewGsSql(this, userId);
    ObjectGS *o = NULL;
    AckNewGS ack;
    ack.set_seqno(msg->seqno());
    ack.set_result(res);
    if (1 == res)
    {
        string check = GSOrUavMessage::GenCheckString();
        o = new ObjectGS(userId);
        if (o)
            o->SetCheck(check);
        ack.set_check(check);
    }
    ProtoMsg::SendProtoTo(ack, s);
    return o;
}

void GSManager::LoadConfig()
{
    if (m_bInit)
        return;

    m_bInit = true;
    TiXmlDocument doc;
    doc.LoadFile("GSInfo.xml");
    _parseMySql(doc);
    AddDatabaseUser("root", "NIhao666", ObjectGS::Type_ALL);

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
    string db = VGDBManager::Load(doc, tables);
    if (!m_sqlEng)
        return;

    m_sqlEng->ConnectMySql(VGDBManager::GetMysqlHost(db),
        VGDBManager::GetMysqlPort(db),
        VGDBManager::GetMysqlUser(db),
        VGDBManager::GetMysqlPswd(db));

    m_sqlEng->EnterDatabase(db, VGDBManager::GetMysqlCharSet(db));
    for (const string &tb : tables)
    {
        m_sqlEng->CreateTable(VGDBManager::GetTableByName(tb));
    }
    for (const string &trigger : VGDBManager::GetTriggers())
    {
        m_sqlEng->CreateTrigger(VGDBManager::GetTriggerByName(trigger));
    }
}

DECLARE_MANAGER_ITEM(GSManager)