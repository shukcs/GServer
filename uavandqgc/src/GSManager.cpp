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
#include "IMessage.h"
#include "tinyxml.h"
#include "ObjectGS.h"
#include "ObjectManagers.h"

using namespace das::proto;
using namespace google::protobuf;
using namespace std;
////////////////////////////////////////////////////////////////////////////////
//GSManager
////////////////////////////////////////////////////////////////////////////////
GSManager::GSManager() : IObjectManager(0),m_sqlEng(new VGMySql)
, m_p(new ProtoMsg)
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

IObject *GSManager::PrcsReceiveByMrg(ISocket *s, const char *buf, int &len)
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

bool GSManager::PrcsRemainMsg(const IMessage &)
{
    return true;
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
    else if (ExecutItem *item = VGDBManager::GetSqlByName("queryGSInfo"))
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
                o->SetPswd(pswd);
                if (FiledValueItem *fd = item->GetReadItem("auth"))
                    o->SetAuth(fd->GetValue<int>());
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