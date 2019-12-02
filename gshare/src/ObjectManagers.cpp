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
        ms.PrcsObjectsDestroy();
        bool ret = ms.PrcsMangerData();
        ms.PrcsSubcribes();
        ms.PrcsMessages();
        if(ms.PrcsRcvBuff())
            ret = true;

        return ret;
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
    delete m_mgr;
    Unregister();
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
, m_mtxObj(new Mutex), m_mtxMsg(new Mutex)
, m_thread(new ManagerThread)
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

bool ObjectManagers::SendMsg(IMessage *msg)
{
    if (!msg)
        return false;

    m_mtxMsg->Lock();
    m_messages.push_back(msg);
    m_mtxMsg->Unlock();
    return true;
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
    map<int, IObjectManager*>::iterator itr = m_managersMap.find(type);
    if (itr != m_managersMap.end())
    {
        m_managersMap.erase(itr);
    }
}

void ObjectManagers::RemoveManager(const IObjectManager *m)
{
    map<int, IObjectManager*>::iterator itr = m_managersMap.begin();
    for (; itr != m_managersMap.end(); ++itr)
    {
        if (m == itr->second)
        {
            m_managersMap.erase(itr);
            break;
        }
    }

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

void ObjectManagers::Subcribe(IObject *o, const std::string &sender, int tpMsg)
{
    if (!o || o->GetObjectID().empty() || sender.empty() || tpMsg < 0)
        return;
    if (SubcribeStruct *sub = new SubcribeStruct(*o, sender, tpMsg, true))
    {
        m_mtxObj->Lock();
        m_subcribeMsgs.push_back(sub);
        m_mtxObj->Unlock();
    }
}

void ObjectManagers::Unsubcribe(IObject *o, const std::string &sender, int tpMsg)
{
    if (!o || o->GetObjectID().empty() || sender.empty() || tpMsg < 0)
        return;

    if (SubcribeStruct *sub = new SubcribeStruct(*o, sender, tpMsg, false))
    {
        m_mtxObj->Lock();
        m_subcribeMsgs.push_back(sub);
        m_mtxObj->Unlock();
    }
}

void ObjectManagers::DestroyCloneMsg(IMessage *msg)
{
    if(msg)
    {
        m_mtxMsg->Lock();
        m_releaseMsgs.push_back(msg);
        m_mtxMsg->Unlock();
    }
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

    Lock l(m_mtx);
    for (ISocket *s : m_keysRemove)
    {
        _removeBuff(s);
    }
    m_keysRemove.clear();
}

void ObjectManagers::PrcsObjectsDestroy()
{
    if (m_objectsDestroy.empty())
        return;

    Lock l(m_mtxObj);
    while (m_objectsDestroy.size() > 0)
    {
        delete m_objectsDestroy.front();
        m_objectsDestroy.pop_front();
    }
}

void ObjectManagers::PrcsSubcribes()
{
    while (m_subcribeMsgs.size() > 0)
    {
        SubcribeStruct *sub = m_subcribeMsgs.front();
        m_subcribeMsgs.pop_front();
        ObjectDsc dsc(sub->m_subcribeType, sub->m_subcribeId);
        if (sub->m_bSubcribe)
        {
            SubcribeList &ls = m_subcribes[sub->m_sender][sub->m_msgType];
            if (!IObject::IsContainsInList(ls, dsc))
                ls.push_back(dsc);
        }
        else
        {
            MessageSubcribes::iterator itr = m_subcribes.find(sub->m_sender);
            if (itr != m_subcribes.end())
            {
                SubcribeMap::iterator it = itr->second.find(sub->m_msgType);
                if (it != itr->second.end())
                {
                    it->second.remove(dsc);
                    if (it->second.empty())
                    {
                        itr->second.erase(it);
                        if (itr->second.empty())
                            m_subcribes.erase(itr);
                    }
                }
            }
        }
        delete sub;
    }
}

void ObjectManagers::PrcsMessages()
{
    if (m_messages.empty() && m_releaseMsgs.empty())
        return;
     
    while (m_messages.size() > 0)
    {
        m_mtxMsg->Lock();
        IMessage *msg = m_messages.front();
        m_messages.pop_front();
        m_mtxMsg->Unlock();
        for (const ObjectDsc &s : getMessageSubcribes(msg))
        {
            if (IObjectManager *mgr = GetManagerByType(s.first))
                mgr->PushMessage(msg->Clone(s.second, s.first));
        }
        if (IObjectManager *mgr = GetManagerByType(msg->GetReceiverType()))
        {
            mgr->PushMessage(msg);
            return;
        }
        msg->Release();
    }

    while (m_releaseMsgs.size() > 0)
    {
        m_mtxMsg->Lock();
        IMessage *msg = m_releaseMsgs.front();
        m_releaseMsgs.pop_front();
        m_mtxMsg->Unlock();
        delete msg;
    }
}

ObjectManagers::SubcribeList &ObjectManagers::getMessageSubcribes(IMessage *msg)
{
    return _getSubcribes(msg ? msg->GetSenderID() : string(), msg ? msg->GetMessgeType() : -1);
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

ObjectManagers::SubcribeList &ObjectManagers::_getSubcribes(const string &sender, int tpMsg)
{
    static SubcribeList sEpty;
    if(!sender.empty() || tpMsg>=0)
    {
        MessageSubcribes::iterator itr = m_subcribes.find(sender);
        if (itr != m_subcribes.end())
        {
            SubcribeMap::iterator it = itr->second.find(tpMsg);
            if (it != itr->second.end())
                return it->second;
        }
    }
    return sEpty;
}