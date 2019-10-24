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

bool GSManager::PrcsRemainMsg(const IMessage &)
{
    return true;
}

IObject *GSManager::prcsPBLogin(ISocket *s, const RequestGSIdentityAuthentication *rgi)
{
    if (!rgi)
        return NULL;

    string usr = rgi->userid();
    string pswd = rgi->password();
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

    if (ISocketManager *m = s->GetManager())
        m->Log(0, usr, 0, "[%s]%s", s->GetHost().c_str(), 1 == res ? "login success" : "login fail");

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

    int res = ExecutNewGsSql(this, msg->userid());
    ObjectGS *o = NULL;
    AckNewGS ack;
    ack.set_seqno(msg->seqno());
    ack.set_result(res);
    if (1 == res)
    {
        string check = GSOrUavMessage::GenCheckString();
        o = new ObjectGS(msg->userid());
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
    m_bInit = true;
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