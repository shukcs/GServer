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
        ms.PrcsSubcribes();
        ms.PrcsMessages();
        if(ms.PrcsRcvBuff())
            return true;

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
ObjectManagers::ObjectManagers():m_mtxSock(new Mutex)
, m_mtxObj(new Mutex), m_mtxMsg(new Mutex)
, m_thread(new ManagerThread), m_log(NULL)
{
    if (m_thread)
        m_thread->SetRunning();
}

ObjectManagers::~ObjectManagers()
{
    delete m_mtxSock;
    delete m_mtxObj;
    delete m_mtxMsg;
}

ObjectManagers &ObjectManagers::Instance()
{
    static ObjectManagers sOM;
    return sOM;
}

ILog &ObjectManagers::GetLog()
{
    ILog *ret = Instance().m_log;
    if (NULL == ret)
    {
        static GOutLog sLog;
        return sLog;
    }
    return *ret;
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
    m_mtxObj->Lock();
    m_mgrsRemove.Push(type);
    m_mtxObj->Unlock();
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

    Lock l(m_mtxSock);
    if (IObject *obj = sock->GetOwnObject())
    {
        obj->Receive(buf, len);
        return;
    }

    map<ISocket *, LoopQueBuff*>::iterator itr = m_socksRcv.find(sock);
    LoopQueBuff *bb = NULL;
    if (itr == m_socksRcv.end())
    {
        bb = new LoopQueBuff(2048);
        if (bb)
            m_socksRcv[sock] = bb;
    }
    else
    {
        bb = itr->second;
    }
    bb->Push(buf, len, true);
}

void ObjectManagers::Subcribe(IObject *o, const std::string &sender, int tpMsg)
{
    if (!o || o->GetObjectID().empty() || sender.empty() || tpMsg < 0)
        return;
    if (SubcribeStruct *sub = new SubcribeStruct(*o, sender, tpMsg, true))
    {
        m_mtxMsg->Lock();
        m_subcribeMsgs.Push(sub);
        m_mtxMsg->Unlock();
    }
}

void ObjectManagers::Unsubcribe(IObject *o, const std::string &sender, int tpMsg)
{
    if (!o || o->GetObjectID().empty() || sender.empty() || tpMsg < 0)
        return;

    if (SubcribeStruct *sub = new SubcribeStruct(*o, sender, tpMsg, false))
    {
        m_mtxMsg->Lock();
        m_subcribeMsgs.Push(sub);
        m_mtxMsg->Unlock();
    }
}

void ObjectManagers::SetLog(ILog *log)
{
    if (!m_log)
        m_log = log;
}

void ObjectManagers::Destroy(IObject *o)
{
    if (!o)
        return;

    m_mtxObj->Lock();
    m_objectsDestroy.Push(o);
    m_mtxObj->Unlock();
}

void ObjectManagers::OnSocketClose(ISocket *sock)
{
    m_mtxSock->Lock();
    _removeBuff(sock);
    m_mtxSock->Unlock();
}

bool ObjectManagers::PrcsRcvBuff()
{
    bool ret = false;
    m_mtxSock->Lock();
    MapBuffRecieve::iterator itr = m_socksRcv.begin();
    while (itr != m_socksRcv.end())
    {
        LoopQueBuff *buff = itr->second;
        ISocket *s = itr->first;
        bool bPrcs = false;
        if (buff && buff->Count() > 10)
        {
            int copied = buff->CopyData(m_buff, sizeof(m_buff));
            for (const pair<int, IObjectManager*> &mgr : m_managersMap)
            {
                if (mgr.second->Receive(itr->first, copied, m_buff))
                {
                    itr++; 
                    bPrcs = true;
                    _removeBuff(s);
                    ret = true;
                    break;
                }
            }
        }
        if(!bPrcs)
            itr++;
    }
    m_mtxSock->Unlock();
    return ret;
}

void ObjectManagers::PrcsObjectsDestroy()
{
    while (!m_objectsDestroy.IsEmpty())
    {
        delete m_objectsDestroy.Pop();
    }
}

void ObjectManagers::PrcsSubcribes()
{
    while (!m_subcribeMsgs.IsEmpty())
    {
        SubcribeStruct *sub = m_subcribeMsgs.Pop();
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

ObjectManagers::SubcribeList &ObjectManagers::getMessageSubcribes(IMessage *msg)
{
    return _getSubcribes(msg ? msg->GetSenderID() : string(), msg ? msg->GetMessgeType() : -1);
}

void ObjectManagers::_removeBuff(ISocket *sock)
{
    map<ISocket *, LoopQueBuff*>::iterator itr = m_socksRcv.find(sock);
    if (itr != m_socksRcv.end())
    {
        delete itr->second;
        m_socksRcv.erase(itr);
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
            for (const ObjectDsc &s : getMessageSubcribes(msg))
            {
                if (IObjectManager *mgr = GetManagerByType(s.first))
                    mgr->PushManagerMessage(msg->Clone(s.second, s.first));
            }
            if (IObjectManager *mgr = GetManagerByType(msg->GetReceiverType()))
                mgr->PushManagerMessage(msg);
            else
                delete msg;
        }
    }
    GetLog().ProcessLog();
}

void ObjectManagers::_prcsReleaseMessages(IObjectManager *mgr)
{
    int n = 0;
    while (MessageQue *que = mgr->GetReleaseQue(n++))
    {
        while (!que->IsEmpty())
        {
            IMessage *msg = que->Pop();
            if (!msg)
                continue;
            if (msg->IsClone())
                delete msg;
            else if (IObjectManager *m = GetManagerByType(msg->GetSenderType()))
                m->PushReleaseMsg(msg);
        }
    }
}
