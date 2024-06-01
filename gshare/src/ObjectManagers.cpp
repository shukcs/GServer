#include "ObjectManagers.h"
#include "ObjectBase.h"
#include "net/socketBase.h"
#include "IMessage.h"
#include "GOutLog.h"
#include "common/Lock.h"
#include <mutex>

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
//////////////////////////////////////////////////////////////////////
//ObjectManagers
//////////////////////////////////////////////////////////////////////
ObjectManagers::ObjectManagers():m_cfg(NULL), m_mutex(new std::mutex)
{
}

ObjectManagers::~ObjectManagers()
{
    delete m_mutex;
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


void ObjectManagers::Run(bool b)
{
    for (auto itr = Instance().m_managersMap.begin(); itr != Instance().m_managersMap.end(); ++itr)
    {
        itr->second->LoadConfig(Instance().m_cfg);
        itr->second->Run(b);
    }
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

int ObjectManagers::ProcessReceive(ISocket *sock, IObjectManager *mgr)
{
    if (!sock)
        return 0;

    int ret = 0;
    Lock l(m_mutex);
    auto len = sock->CopyData(m_buff, sizeof(m_buff));
    if (mgr)
        return mgr->AddLoginData(sock, (const char *)m_buff, len);

    for (const pair<int, IObjectManager*> &m : m_managersMap)
    {
        if (m.second->IsHasReuest(m_buff, len))
        {
            mgr = m.second;
            sock->SetLogin(mgr);
            ret = mgr->AddLoginData(sock, (const char *)m_buff, len);
            break;
        }
    }
    if (mgr==NULL && len==sizeof(m_buff))
        sock->Close();

    return ret;
}
