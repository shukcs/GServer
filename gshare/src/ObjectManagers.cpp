#include "ObjectManagers.h"
#include "ObjectBase.h"
#include "Thread.h"
#include "Mutex.h"
#include "Lock.h"
#include "socketBase.h"
#include "IMessage.h"

using namespace std;

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

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
        ms.PrcsObjectsDestroy();
        bool ret = ms.PrcsMangerData();
        if(ms.PrcsRcvBuff())
            ret = true;

        return ret;
    }
private:
    list<IObject *> m_objects;
};
////////////////////////////////////////////////////////////
//ManagerAbstractItem
////////////////////////////////////////////////////////////
ManagerAbstractItem::~ManagerAbstractItem()
{
    Unregister();
}

void ManagerAbstractItem::Register()
{
    if (IObjectManager *m = CreateManager())
    {
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
, m_mtxObj(new Mutex), m_thread(new ManagerThread)
{
}

ObjectManagers::~ObjectManagers()
{
    m_thread->SetRunning(false);
}

ObjectManagers &ObjectManagers::Instance()
{
    static ObjectManagers sOM;
    return sOM;
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
    map<int, IObjectManager*>::iterator itr = m_managersMap.find(type);
    if (itr != m_managersMap.end())
    {
        delete itr->second;
        m_managersMap.erase(itr);
    }
}

void ObjectManagers::ProcessReceive(ISocket *sock, void const *buf, int len)
{
    if (!m_thread->IsRunning())
    {
        m_thread->SetRunning();
        printf("ObjectManagers::%s: managers size %d\n", __FUNCTION__, (int)m_managersMap.size());
    }

    if (!sock)
        return;

    sock->ResetSendBuff(256);
    m_mtx->Lock();
    map<ISocket *, BaseBuff*>::iterator itr = m_socksRcv.find(sock);
    BaseBuff *bb = NULL;
    if (itr == m_socksRcv.end())
    {
        bb = new BaseBuff(2048);
        if (bb)
            m_socksRcv[sock] = bb;
    }
    else
    {
        bb = itr->second;
    }

    if (!bb->Add(buf, len))
    {
        if (len < 2048)
        {
            bb->Clear();
            bb->Add(buf, len);
        }
        else if (bb->ReMained() > 0)
        {
            bb->Add(buf, bb->ReMained());
        }
    }
    m_mtx->Unlock();
}

bool ObjectManagers::SendMsg(IMessage *msg)
{
    if (!msg)
        return false;

    if (IObjectManager *mgr = GetManagerByType(msg->GetReceiverType()))
    {
        mgr->ReceiveMessage(msg);
        return true;
    }
    msg->Release();
    return false;
}

IObjectManager *ObjectManagers::GetManagerByType(int tp) const
{
    map<int, IObjectManager*>::const_iterator itr = m_managersMap.find(tp);
    if (itr != m_managersMap.end())
        return itr->second;

    return NULL;
}

void ObjectManagers::Destroy(IObject *o)
{
    if (!o)
        return;

    Lock l(m_mtxObj);
    m_objectsDestroy.push_back(o);
}

void ObjectManagers::OnSocketClose(ISocket *sock)
{
    map<ISocket *, BaseBuff*>::iterator itr = m_socksRcv.find(sock);
    if (itr != m_socksRcv.end())
    {
        Lock l(m_mtx);
        m_keysRemove.push_back(sock);
    }
}

bool ObjectManagers::PrcsRcvBuff()
{
    PrcsCloseSocket();
    bool ret = false;
    for (const pair<ISocket *, BaseBuff*> &itr : m_socksRcv)
    {
        BaseBuff *buff = itr.second;
        if (buff && buff->IsChanged())
        {
            int prcs = buff->Count();
            for (const pair<int, IObjectManager*> &mgr : m_managersMap)
            {
                int n=0;
                if (mgr.second->Receive(itr.first, *buff, n))
                {
                    m_mtx->Lock();
                    m_keysRemove.push_back(itr.first);
                    m_mtx->Unlock();
                    ret = true;
                    break;
                }
                if (n < prcs)
                    prcs = n;
            }
            buff->Clear(prcs);
            buff->SetNoChange();
        }
    }
    return ret;
}

void ObjectManagers::PrcsCloseSocket()
{
    if (m_keysRemove.size() <= 0)
        return;

    m_mtx->Lock();
    for (ISocket *s : m_keysRemove)
    {
        _removeBuff(s);
    }
    m_keysRemove.clear();
    m_mtx->Unlock();
}

void ObjectManagers::PrcsObjectsDestroy()
{
    if (m_objectsDestroy.size() < 1)
        return;

    for (IObject *o : m_objectsDestroy)
    {
        delete o;
    }

    Lock l(m_mtxObj);
    m_objectsDestroy.clear();
}

bool ObjectManagers::PrcsMangerData()
{
    bool ret = false;

    for (const pair<int, IObjectManager*> &mgr : m_managersMap)
    {
        IObjectManager *om = mgr.second;
        if (om->HasIndependThread())
            continue;

        if (om->ProcessBussiness())
            ret = true;
    }
    return ret;
}

void ObjectManagers::_removeBuff(ISocket *sock)
{
    map<ISocket *, BaseBuff*>::iterator itr = m_socksRcv.find(sock);
    if (itr != m_socksRcv.end())
    {
        delete itr->second;
        m_socksRcv.erase(sock);
    }
}

#ifdef SOCKETS_NAMESPACE
}
#endif