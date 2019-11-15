#include "ObjectAbsPB.h"
#include "socketBase.h"
#include "das.pb.h"
#include "ProtoMsg.h"
#include "Messages.h"
#include "Utility.h"
#include "VGMysql.h"
#include "VGDBManager.h"
#include "DBExecItem.h"

using namespace das::proto;
using namespace google::protobuf;
using namespace std;
#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif
////////////////////////////////////////////////////////////////////////////////
//ObjectUav
////////////////////////////////////////////////////////////////////////////////
ObjectAbsPB::ObjectAbsPB(const std::string &id)
: IObject(NULL, id), m_bConnect(false)
, m_p(NULL)
{
}

ObjectAbsPB::~ObjectAbsPB()
{
    delete m_p;
}

bool ObjectAbsPB::IsConnect() const
{
    return m_bConnect;
}

VGMySql *ObjectAbsPB::GetMySql() const
{
    return NULL;
}

void ObjectAbsPB::OnConnected(bool bConnected)
{
    m_bConnect = bConnected;
    if (bConnected)
    {
        if (!m_p)
        {
            m_p = new ProtoMsg;
            if (m_p)
                m_p->InitSize();
        }
        return;
    }

    IObjectManager *mgr = GetManager();
    if (m_sock && mgr)
    {
        mgr->Log(0, GetObjectID(), 0, "(%s:%d) disconnect", m_sock->GetHost().c_str(), m_sock->GetPort());
        m_sock->Close();
        m_sock = NULL;
    }
}

int ObjectAbsPB::GetSenLength() const
{
    if (m_p)
        return m_p->RemaimedLength();

    return 0;
}

int ObjectAbsPB::CopySend(char *buf, int sz, unsigned form)
{
    if (m_p)
        return m_p->CopySend(buf, sz, form);

    return 0;
}

void ObjectAbsPB::SetSended(int sended /*= -1*/)
{
    if (m_p)
        m_p->SetSended(sended);
}

bool ObjectAbsPB::send(const google::protobuf::Message &msg)
{
    if (m_p && m_sock)
    {
        m_p->SendProto(msg, m_sock);
        return true;
    }

    return false;
}

int64_t ObjectAbsPB::executeInsertSql(ExecutItem *item)
{
    if (!item)
        return -1;

    VGMySql *sql = GetMySql();
    if (sql && sql->Execut(item))
        return item->GetIncrement() ? item->GetIncrement()->GetValue() : 0;

    return -1;
}