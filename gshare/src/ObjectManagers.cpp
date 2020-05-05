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
//ManagerThread
////////////////////////////////////////////////////////////
class ManagerThread : public Thread
{
public:
    ManagerThread() : Thread(false)
    {
    }
protected:
    bool RunLoop()
    {
        ObjectManagers &ms = ObjectManagers::Instance();
        ms.PrcsMessages();

        return false;
    }
private:
};
////////////////////////////////////////////////////////////////////////
//SubcribeStruct
////////////////////////////////////////////////////////////////////////
class SubcribeStruct
{
public:
    SubcribeStruct(const IObject &subcr, const string &sender, int msgType, bool bSubcribe)
        : m_sender(sender), m_msgType(msgType), m_subcribeId(subcr.GetObjectID())
        , m_subcribeType(subcr.GetObjectType()), m_bSubcribe(bSubcribe)
    {
    }
public:
    string  m_sender;
    int     m_msgType;
    string  m_subcribeId;
    int     m_subcribeType;
    bool    m_bSubcribe;
};
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
ObjectManagers::ObjectManagers():m_mtx(new Mutex)
, m_thread(new ManagerThread)
{
    if (m_thread)
        m_thread->SetRunning();
}

ObjectManagers::~ObjectManagers()
{
    delete m_mtx;
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

void ObjectManagers::AddManager(IObjectManager *m)
{
    if (m)
    {
        int type = m->GetObjectType();
        map<int, IObjectManager*>::iterator itr = m_managersMap.find(type);
        if (itr != m_managersMap.end() && m != itr->second)
            delete itr->second;

        m_managersMap[type] = m;
        m->LoadConfig();
    }
}

void ObjectManagers::RemoveManager(int type)
{
    m_mtx->Lock();
    m_mgrsRemove.Push(type);
    m_mtx->Unlock();
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

    if (ILink *link = sock->GetOwnObject())
    {
        link->Receive(buf, len);
        return;
    }

    len = sock->CopyData(m_buff, sizeof(m_buff));
    for (const pair<int, IObjectManager*> &m : m_managersMap)
    {
        if (m.second->ParseRequest(sock, m_buff, len))
            break;
    }
}

void ObjectManagers::PrcsMessages()
{
    while (!m_mgrsRemove.IsEmpty())
    {
        int type = m_mgrsRemove.Pop();
        map<int, IObjectManager*>::iterator itr = m_managersMap.find(type);
        if (itr != m_managersMap.end())
            m_managersMap.erase(itr);
    }

    map<int, IObjectManager*>::iterator itr = m_managersMap.begin();
    for (; itr != m_managersMap.end(); ++itr)
    {
        _prcsSendMessages(itr->second);
        _prcsReleaseMessages(itr->second);
    }
}

void ObjectManagers::_prcsSendMessages(IObjectManager *mgr)
{
    int n = 0;
    while (MessageQue *que = mgr->GetSendQue(n++))
    {
        while (!que->IsEmpty())
        {
            IMessage *msg = que->Pop();
            if (!msg)
                continue;

            if (IObjectManager *mgr = GetManagerByType(msg->GetReceiverType()))
                mgr->PushManagerMessage(msg);
            else
                delete msg;
        }
    }
}

void ObjectManagers::_prcsReleaseMessages(IObjectManager *mgr)
{
    if (!mgr)
        return;
    while (IMessage *msg = mgr->PopRecycleMessage())
    {
        if (IObjectManager *m = GetManagerByType(msg->GetSenderType()))
            m->PushReleaseMsg(msg);
    }
}
