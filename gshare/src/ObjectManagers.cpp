#include "ObjectManagers.h"
#include "ObjectBase.h"
#include "Thread.h"
#include "Mutex.h"
#include "Lock.h"
#include "socketBase.h"
#include "IMessage.h"
#include "GOutLog.h"

using namespace std;

#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE
#endif
////////////////////////////////////////////////////////////
//ManagerAbstractItem
////////////////////////////////////////////////////////////
ManagerAbstractItem::ManagerAbstractItem():m_type(-1)
, m_mgr(NULL)
{
}

ManagerAbstractItem::~ManagerAbstractItem()
{
    Unregister();
    delete m_mgr;
}

void ManagerAbstractItem::Register()
{
    if (IObjectManager *m = CreateManager())
    {
        m_type = m->GetObjectType();
        ObjectManagers::Instance().AddManager(m);
    }
}

void ManagerAbstractItem::Unregister()
{
    ObjectManagers::Instance().RemoveManager(m_type);
}
////////////////////////////////////////////////////////////
//ObjectManagers
////////////////////////////////////////////////////////////
ObjectManagers::ObjectManagers():m_cfg(NULL)
{
}

ObjectManagers::~ObjectManagers()
{
}

ObjectManagers &ObjectManagers::Instance()
{
    static ObjectManagers sOM;
    return sOM;
}

ILog &ObjectManagers::GetLog()
{
    static GOutLog sLog;
    return sLog;
}

bool ObjectManagers::SndMessage(IMessage *msg)
{
    if (!msg || !msg->IsValid())
        return false;

    if (auto mgr = Instance().GetManagerByType(msg->GetReceiverType()))
    {
        mgr->PushManagerMessage(msg);
        return true;
    }

    return false;
}

bool ObjectManagers::RlsMessage(IMessage *msg)
{
    if (!msg || !msg->IsValid())
        return false;

    if (auto mgr = Instance().GetManagerByType(msg->GetSenderType()))
    {
        mgr->PushReleaseMsg(msg);
        return true;
    }

    return false;
}

void ObjectManagers::SetManagersXml(TiXmlElement *msg)
{
    Instance().m_cfg = msg;
}

void ObjectManagers::AddManager(IObjectManager *m)
{
    if (m)
    {
        int type = m->GetObjectType();
        map<int, IObjectManager*>::iterator itr = m_managersMap.find(type);
        if (itr != m_managersMap.end() && m != itr->second)
            delete itr->second;

        m_managersMap[type] = m;
        m->LoadConfig(m_cfg);
    }
}

void ObjectManagers::RemoveManager(int type)
{
    m_managersMap.erase(type);
}

IObjectManager *ObjectManagers::GetManagerByType(int tp) const
{
    map<int, IObjectManager*>::const_iterator itr = m_managersMap.find(tp);
    if (itr != m_managersMap.end())
        return itr->second;

    return NULL;
}

void ObjectManagers::ProcessReceive(ISocket *sock, void const *buf, int len)
{
    if (!sock)
        return;

    if (ILink *link = sock->GetHandleLink())
    {
        link->Receive(buf, len);
        return;
    }

    len = sock->CopyData(m_buff, sizeof(m_buff));
    for (const pair<int, IObjectManager*> &m : m_managersMap)
    {
        IObjectManager *mgr = m.second;
        if (mgr->IsHasReuest(m_buff, len))
        {
            sock->SetLogin(mgr);
            mgr->AddLoginData(sock, buf, len);
            sock->ClearBuff();
            return;
        }
    }
}
