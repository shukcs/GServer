#include "ObjectBase.h"
#include "socketBase.h"
#include "Thread.h"
#include "Mutex.h"
#include "ObjectManagers.h"
#include "Utility.h"
#include "Lock.h"
#include "IMessage.h"

using namespace std;

#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif

class ObjectMetuxs {
public:
    ObjectMetuxs() :
        m_mtx(new Mutex), m_mtxMsg(new Mutex){};
public:
    IMutex                  *m_mtx;
    IMutex                  *m_mtxMsg;
};
//////////////////////////////////////////////////////////////////
//BussinessThread
//////////////////////////////////////////////////////////////////
class BussinessThread : public Thread
{
public:
    BussinessThread(int id, IObjectManager *mgr) :Thread(true)
    , m_mgr(mgr), m_id(id)
    {
    }
    int GetId()const
    {
        return m_id;
    }
protected:
    bool RunLoop()
    {
        if (!m_mgr)
            return false;
        bool ret = false;
        if (m_id == 0)
            ret = m_mgr->ProcessBussiness();

        for (const pair<string, IObject*> &itrO : m_mgr->GetThreadObject(m_id))
        {
            if (itrO.second->PrcsBussiness())
                return ret = true;
        }

        return ret;
    }
private:
    IObjectManager  *m_mgr;
    int             m_id;
};
//////////////////////////////////////////////////////////////////
//IObject
//////////////////////////////////////////////////////////////////
IObject::IObject(ISocket *sock, const string &id)
: m_sock(sock), m_id(id), m_bRelease(false)
, m_mtx(NULL), m_mtxMsg(NULL), m_idThread(-1)
{
    SetBuffSize(1024 * 4);
}

IObject::~IObject()
{
    if (m_sock)
    {
        m_sock->SetObject(NULL);
        m_sock->Close();
    }
}

const string &IObject::GetObjectID() const
{
    return m_id;
}

void IObject::SetObjectID(const std::string &id)
{
    m_id = id;
}

ISocket * IObject::GetSocket() const
{
    return m_sock;
}

void IObject::SetSocket(ISocket *s)
{
    if (s)
        s->SetObject(this);

    m_sock = s;
}

int IObject::GetSenLength() const
{
    return 0;
}

int IObject::CopySend(char *, int, unsigned)
{
    return 0;
}

void IObject::SetSended(int)
{
}

void IObject::RemoveRcvMsg(IMessage *msg)
{
    Lock l(m_mtxMsg);
    m_lsMsg.remove(msg);
}

void IObject::AddRelease(IMessage *msg)
{
    Lock l(m_mtxMsg);
    m_lsMsgRelease.push_back(msg);
}

bool IObject::PrcsBussiness()
{
    bool ret = false;
    if (m_buff.IsChanged())
    {
        int n = ProcessReceive(m_buff.GetBuffAddress(), m_buff.Count());
        if(n>0)
        {
            m_mtx->Lock();
            m_buff.Clear(n);
            m_mtx->Unlock();
            ret = true;
        }
    }
    while (m_lsMsg.size())
    {
        IMessage *msg = m_lsMsg.front();
        m_lsMsg.pop_front();
        if (msg->IsValid())
            ProcessMassage(*msg);
        msg->Release();
    }

    while (m_lsMsgRelease.size() > 0)
    {
        delete m_lsMsgRelease.front();
        m_lsMsgRelease.pop_front();
    }
    return ret;
}

int IObject::GetThreadId() const
{
    return m_idThread;
}

void IObject::SetThreadId(int id)
{
    m_idThread = id;
}

void IObject::OnClose(ISocket *s)
{
    if (m_sock == s)
        OnConnected(false);
}

void IObject::Release()
{
    if (IObjectManager *m = GetManager())
        m->RemoveObject(this);

    ObjectManagers::Instance().Destroy(this);
    m_bRelease = true;
}

bool IObject::IsRealse()
{
    return m_bRelease;
}

bool IObject::RcvMassage(IMessage *msg)
{
    Lock l(m_mtxMsg);
    m_lsMsg.push_back(msg);
    return true;
}

void IObject::SetBuffSize(uint16_t sz)
{
    if (sz < 32)
        sz = 32;

    m_buff.InitBuff(sz);
}

IObjectManager *IObject::GetManager() const
{
    return GetManagerByType(GetObjectType());
}

bool IObject::SendMsg(IMessage *msg)
{
    return ObjectManagers::Instance().SendMsg(msg);
}

IObjectManager *IObject::GetManagerByType(int tp)
{
    return ObjectManagers::Instance().GetManagerByType(tp);
}

bool IObject::Receive(const void *buf, int len)
{
    bool ret = false;
    if (m_mtx && len>0)
    {
        Lock l(m_mtx);
        ret = m_buff.Add(buf, len);
    }
    return ret;
}
//////////////////////////////////////////////////////////////////
//IObjectManager
//////////////////////////////////////////////////////////////////
IObjectManager::IObjectManager(uint16_t nThread):m_mtx(new Mutex)
{
    InitThread(nThread);
}

IObjectManager::~IObjectManager()
{
    for (const pair<int, ThreadObjects> &itr : m_mapThreadObject)
    {
        for (const pair<string, IObject*> &itrO : itr.second)
        {
            itrO.second->Release();
        }
    }
    for (IMessage *msg : m_lsMsg)
    {
        msg->Release();
    }
    for (BussinessThread *t : m_lsThread)
    {
        t->SetRunning(false);
    }
    Utility::Sleep(100);
}

bool IObjectManager::Receive(ISocket *s, const BaseBuff &buff)
{
    void *buf = buff.GetBuffAddress();
    int len = buff.Count();
    if (!s || !buf || len <= 0)
        return false;

    IObject *o = PrcsReceiveByMrg(s, (char*)buf, len);
    if (o)
    {
        AddObject(o);
        o->SetSocket(s);
    }

    if (o && len < buff.Count())
        o->Receive((char*)buf + len, buff.Count() - len);

    return o != NULL;
}

void IObjectManager::RemoveRcvMsg(IMessage *msg)
{
    m_lsMsg.remove(msg);
}

void IObjectManager::AddRelease(IMessage *msg)
{
    m_mtx->Lock(); //这个来自不同线程，需要加锁
    m_lsMsgRelease.push_back(msg);
    m_mtx->Unlock();
}

void IObjectManager::PrcsReleaseMsg()
{
    while (m_lsMsgRelease.size() > 0)
    {
        delete *m_lsMsgRelease.begin();
        m_lsMsgRelease.pop_front();
    }
}

bool IObjectManager::PrcsRemainMsg(const IMessage &)
{
    return false;
}

bool IObjectManager::HasIndependThread() const
{
    return m_lsThread.size() > 0;
}

bool IObjectManager::ProcessBussiness()
{
    bool ret = false;
    while (m_lsMsg.size())
    {
        IMessage *msg = m_lsMsg.front();
        m_mtx->Lock();
        m_lsMsg.pop_front();
        m_mtx->Unlock();
        if (msg->IsValid())
        {
            ProcessMassage(*msg);
            msg->Release();
            ret = true;
        }
    }

    if(!HasIndependThread())
    {
        for (const pair<int, ThreadObjects> &itr : m_mapThreadObject)
        {
            for (const pair<string, IObject*> &itrO : itr.second)
            {
                itrO.second->PrcsBussiness();
                ret = true;
            }
        }
    }
    PrcsReleaseMsg();
    return ret;
}

void IObjectManager::RemoveObject(IObject *o)
{
    if (!o)
        return;

    ObjectsMap::iterator itr = m_mapThreadObject.find(o->GetThreadId());
    if (itr != m_mapThreadObject.end())
    {
        m_mtx->Lock();
        itr->second.erase(o->GetObjectID());
        m_mtx->Unlock();
    }
}

const IObjectManager::ThreadObjects &IObjectManager::GetThreadObject(int t) const
{
    static ThreadObjects sLsEmpty;
    ObjectsMap::const_iterator itr = m_mapThreadObject.find(t);
    if (itr != m_mapThreadObject.end())
        return itr->second;

    return sLsEmpty;
}

bool IObjectManager::Exist(IObject *obj) const
{
    if (!obj || obj->GetThreadId()>=0)
        return false;

    ObjectsMap::const_iterator itr = m_mapThreadObject.find(obj->GetThreadId());
    if (itr != m_mapThreadObject.end())
    {
        ThreadObjects::const_iterator itrO = itr->second.find(obj->GetObjectID());
        if (itrO != itr->second.end())
            return true;
    }
    return false;
}

void IObjectManager::ProcessMassage(const IMessage &msg)
{
    if (PrcsRemainMsg(msg))
        return;

    for (const pair<int, ThreadObjects> &itr : m_mapThreadObject)
    {
        for (const pair<string, IObject*> &o : itr.second)
        {
            o.second->ProcessMassage(msg);
        }
    }
}

bool IObjectManager::AddObject(IObject *obj)
{
    if (!obj || Exist(obj))
        return false;

    int n = GetPropertyThread();
    std::map<int, ObjectMetuxs*>::iterator itr = m_threadMutexts.find(n);
    if(itr!=m_threadMutexts.end())
    {
        obj->m_mtx = itr->second->m_mtx;
        obj->m_mtxMsg = itr->second->m_mtxMsg;
    }

    m_mtx->Lock();
    m_mapThreadObject[n][obj->GetObjectID()] = obj;
    obj->SetThreadId(n);
    m_mtx->Unlock();

    return true;
}

IObject *IObjectManager::GetObjectByID(const std::string &id) const
{
    for (const pair<int, ThreadObjects> &itr : m_mapThreadObject)
    {
        ThreadObjects::const_iterator itrObj = itr.second.find(id);
        if(itrObj == itr.second.end())
            continue;

        return itrObj->second;
    }
    return NULL;
}

void IObjectManager::ReceiveMessage(IMessage *msg)
{
    if (IObject *obj = GetObjectByID(msg->GetReceiverID()))
        obj->RcvMassage(msg);
    else
        AddMessage(msg);
}

bool IObjectManager::SendMsg(IMessage *msg)
{
    return IObject::SendMsg(msg);
}

void IObjectManager::InitThread(uint16_t nThread /*= 1*/)
{
    if (m_threadMutexts.find(0) == m_threadMutexts.end())
        m_threadMutexts[0] = new ObjectMetuxs();
    if (nThread > 255 || nThread < 1 || m_lsThread.size()>0)
        return;

    for (int i=0; i<nThread; ++i)
    {
        if(BussinessThread *t = new BussinessThread(i, this))
            m_lsThread.push_back(t);

        if (m_threadMutexts.find(i) == m_threadMutexts.end())
            m_threadMutexts[i] = new ObjectMetuxs();
    }
}

int IObjectManager::GetPropertyThread() const
{
    int nCount = -1;
    int ret = 0;
    for (BussinessThread *t : m_lsThread)
    {
        ObjectsMap::const_iterator itr = m_mapThreadObject.find(t->GetId());
        if (itr == m_mapThreadObject.end())
            return t->GetId();

        if (nCount<0 || nCount>(int)itr->second.size())
        {
            nCount = itr->second.size();
            ret = itr->first;
        }
    }

    return ret;
}

void IObjectManager::AddMessage(IMessage *msg)
{
    m_mtx->Lock();
    m_lsMsg.push_back(msg);
    m_mtx->Unlock();
}