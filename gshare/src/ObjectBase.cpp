#include "ObjectBase.h"
#include "socketBase.h"
#include "Thread.h"
#include "Mutex.h"
#include "ObjectManagers.h"
#include "Utility.h"
#include "Lock.h"
#include "IMessage.h"
#include "ILog.h"
#include "LoopQueue.h"
#include <stdarg.h>

using namespace std;

#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif

class ObjectMetuxs {
public:
    ObjectMetuxs() :
        m_mtx(new Mutex), m_mtxMsg(new Mutex){};
    ~ObjectMetuxs()
    {
        delete m_mtx;
        delete m_mtxMsg;
    }
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
    , m_mgr(mgr), m_id(id), m_szBuff(0), m_buff(NULL)
    {
    }
    ~BussinessThread() 
    {
        delete m_buff;
    }
    char *GetBuff()const 
    {
        return m_buff;
    }
    int GetBuffLenth()const
    {
        return m_szBuff;
    }
    void InitialBuff(uint16_t sz)
    {
        if (sz > m_szBuff)
        {
            if(m_buff = new char[sz])
                m_szBuff = sz;
        }
    }
protected:
    bool RunLoop()
    {
        if (!m_mgr)
            return false;

        m_mgr->ProcessBussiness(this);
        return m_mgr->PrcsObjectsOfThread(*this);
    }
private:
    IObjectManager  *m_mgr;
    int             m_id;
    int             m_szBuff;
    char            *m_buff;
};
/////////////////////////////////////////////////////////////////////////////////////////
//IObject
/////////////////////////////////////////////////////////////////////////////////////////
IObject::IObject(ISocket *sock, const string &id): m_tmLastInfo(Utility::msTimeTick())
, m_sock(sock), m_id(id), m_bRelease(false), m_stInit(Uninitial), m_buff(NULL), m_mtxMsg(NULL)
, m_thread(NULL)
{
}

IObject::~IObject()
{
    if (m_sock)
    {
        m_sock->SetObject(NULL);
        m_sock->Close();
    }
}

void IObject::_prcsMessage()
{
    while (m_lsMsg.size() > 0)
    {
        m_mtxMsg->Lock();
        IMessage *msg = m_lsMsg.front();
        m_lsMsg.pop_front();
        m_mtxMsg->Unlock();
        if (msg->IsValid())
            ProcessMessage(msg);
        msg->Release();
    }

    while (m_lsMsgRelease.size() > 0)
    {
        m_mtxMsg->Lock();
        IMessage *msg = m_lsMsgRelease.front();
        m_lsMsgRelease.pop_front();
        m_mtxMsg->Unlock();
        delete msg;
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
    if (m_sock)
        return;

    if (s)
    {
        s->SetObject(this);
        m_tmLastInfo = Utility::msTimeTick();
    }

    m_sock = s;
    OnConnected(s != NULL);
}

void IObject::PushReleaseMsg(IMessage *msg)
{
    Lock l(m_mtxMsg);
    m_lsMsgRelease.push_back(msg);
}

bool IObject::PrcsBussiness(uint64_t ms)
{
    bool ret = false;
    _prcsMessage();
    if (InitialFail == m_stInit)
    {
        Release();
        return ret;
    }
    if (Initialed != m_stInit)
    {
        InitObject();
        return ret;
    }

    if (m_buff && m_buff->Count()>8 && m_thread)
    {
        void *buff = getThreadBuff();
        int len = m_buff->CopyData(buff, getThreadBuffLen());
        if (len > 0)
        {
            len = ProcessReceive(buff, len);
            if (len > 0)
            {
                m_buff->Clear(len);
                ret = true;
            }
        }
    }
    CheckTimer(ms);
    return ret;
}

BussinessThread *IObject::GetThread() const
{
    return m_thread;
}

void IObject::SetThread(BussinessThread *t)
{
    m_thread = t;
}

void IObject::OnSockClose(ISocket *s)
{
    if (m_sock == s)
    {
        m_sock = NULL;
        OnConnected(false);
    }
}

void IObject::Subcribe(const std::string &sender, int msg)
{
    ObjectManagers::Instance().Subcribe(this, sender, msg);
}

void IObject::Unsubcribe(const std::string &sender, int msg)
{
    ObjectManagers::Instance().Unsubcribe(this, sender, msg);
}

void IObject::Release()
{
    m_bRelease = true;
}

bool IObject::IsRealse()
{
    return m_bRelease;
}

bool IObject::PushMessage(IMessage *msg)
{
    Lock l(m_mtxMsg);
    m_lsMsg.push_back(msg);
    return true;
}

void IObject::SetBuffSize(uint16_t sz)
{
    if (sz < 32)
        sz = 32;
    if (!m_buff)
        m_buff = new LoopQueBuff(sz);
    else
        m_buff->ReSize(sz);
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
    if (buf && len > 0)
        ret = m_buff->Push(buf, len) > 0;

    if (ret)
        m_tmLastInfo = Utility::msTimeTick();
    return ret;
}

void IObject::CheckTimer(uint64_t ms)
{
    if (ms - m_tmLastInfo > 3600000)
        Release();
    else if (m_sock && ms - m_tmLastInfo > 30000)//超时关闭
        m_sock->Close();
}

void *IObject::getThreadBuff()const
{
    if (m_thread)
        return m_thread->GetBuff();

    return NULL;
}

int IObject::getThreadBuffLen() const
{
    if (m_thread)
        return m_thread->GetBuffLenth();
    return 0;
}

void IObject::ClearRead()
{
    if (m_buff)
        m_buff->Clear(m_buff->Count());
}

//////////////////////////////////////////////////////////////////
//IObjectManager
//////////////////////////////////////////////////////////////////
IObjectManager::IObjectManager() :m_mtx(new Mutex)
, m_log(NULL)
{
}

IObjectManager::~IObjectManager()
{
    for (const pair<BussinessThread*, ThreadObjects> &itr : m_mapThreadObject)
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
    delete m_mtx;
    for (const pair<BussinessThread*, ObjectMetuxs*> &t : m_threadMutexts)
    {
        delete t.second;
    }
    ObjectManagers::Instance().RemoveManager(this);
}

void IObjectManager::LoadConfig()
{
}

bool IObjectManager::Receive(ISocket *s, int len, const char *buf)
{
    if (!s || !buf || len <= 0)
        return false;

    IObject *o = PrcsNotObjectReceive(s, buf, len);
    if (o && !o->GetSocket())
    {
        AddObject(o);
        o->SetSocket(s);
    }

    if (o)
        o->Receive((char*)buf, len);

    return o != NULL;
}

void IObjectManager::PushReleaseMsg(IMessage *msg)
{
    Lock l(m_mtx); //这个来自不同线程，需要加锁
    m_lsMsgRelease.push_back(msg);
}

void IObjectManager::PrcsReleaseMsg()
{
    while (m_lsMsgRelease.size() > 0)
    {
        m_mtx->Lock();
        IMessage *ms = m_lsMsgRelease.front();
        m_lsMsgRelease.pop_front();
        m_mtx->Unlock();
        delete ms;
    }
}

void IObjectManager::removeObject(ThreadObjects &objs, const std::string &id)
{
    ThreadObjects::iterator itr = objs.find(id);
    if (itr != objs.end())
    {
        IObject *o = itr->second;
        m_mtx->Lock();
        objs.erase(itr);
        m_mtx->Unlock();
        ObjectManagers::Instance().Destroy(o);
    }
}

bool IObjectManager::PrcsPublicMsg(const IMessage &)
{
    return false;
}

bool IObjectManager::ProcessBussiness(BussinessThread *)
{
    bool ret = false;
    if (!InitManager())
        return false;

    if (m_lsMsg.size())
    {
        m_mtx->Lock();
        while (m_lsMsg.size())
        {
            IMessage *msg = m_lsMsg.front();
            m_lsMsg.pop_front();    //队列方式, 不用枷锁
            if (msg->IsValid())
            {
                ProcessMessage(msg);
                msg->Release();
                ret = true;
            }
        }
        m_mtx->Unlock();
    }
   
    PrcsReleaseMsg();
    return ret;
}

bool IObjectManager::PrcsObjectsOfThread(BussinessThread &t)
{
    ObjectsMap::iterator itrObjs = m_mapThreadObject.find(&t);
    if (itrObjs == m_mapThreadObject.end())
        return false;

    bool ret = false;
    ThreadObjects::iterator itr = itrObjs->second.begin();
    uint64_t ms = Utility::msTimeTick();
    while (itr != itrObjs->second.end())
    {
        if (itr->second->PrcsBussiness(ms))
            ret = true;

        if (itr->second->IsRealse())
        {
            ThreadObjects::iterator tmp = itr++;
            removeObject(itrObjs->second, tmp->first);
            continue;
        }
        itr++;
    }
    return ret;
}

bool IObjectManager::Exist(IObject *obj) const
{
    if (!obj || obj->GetThread()==NULL)
        return false;

    ObjectsMap::const_iterator itr = m_mapThreadObject.find(obj->GetThread());
    if (itr != m_mapThreadObject.end())
    {
        ThreadObjects::const_iterator itrO = itr->second.find(obj->GetObjectID());
        if (itrO != itr->second.end())
            return true;
    }
    return false;
}

void IObjectManager::SetLog(ILog *l)
{
    m_log = l;
}

void IObjectManager::Log(int err, const std::string &obj, int evT, const char *fmt, ...)
{
    char slask[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(slask, 1023, fmt, ap);
    va_end(ap);
    ILog *log = m_log ? m_log : &ObjectManagers::GetLog();
    log->Log(slask, obj, evT, err);
}

void IObjectManager::ProcessMessage(IMessage *msg)
{
    if (PrcsPublicMsg(*msg))
        return;

    for (const pair<BussinessThread*, ThreadObjects> &itr : m_mapThreadObject)
    {
        for (const pair<string, IObject*> &o : itr.second)
        {
            o.second->ProcessMessage(msg);
        }
    }
}

bool IObjectManager::AddObject(IObject *obj)
{
    if (!obj || Exist(obj))
        return false;

    BussinessThread *t = GetPropertyThread();
    ThreadMetuxsMap::iterator itr = m_threadMutexts.find(t);
    if(itr!=m_threadMutexts.end())
        obj->m_mtxMsg = itr->second->m_mtxMsg;

    m_mtx->Lock();
    m_mapThreadObject[t][obj->GetObjectID()] = obj;
    m_mtx->Unlock();
    obj->SetThread(t);

    return true;
}

IObject *IObjectManager::GetObjectByID(const std::string &id) const
{
    for (const pair<BussinessThread*, ThreadObjects> &itr : m_mapThreadObject)
    {
        ThreadObjects::const_iterator itrObj = itr.second.find(id);
        if(itrObj == itr.second.end())
            continue;

        return itrObj->second;
    }
    return NULL;
}

void IObjectManager::PushMessage(IMessage *msg)
{
    if (!msg)
        return;

    const string id = msg->GetReceiverID();
    IObject *obj = id.empty() ? NULL : GetObjectByID(id);
    if (obj)
        obj->PushMessage(msg);
    else 
        AddMessage(msg);
}

bool IObjectManager::SendMsg(IMessage *msg)
{
    return IObject::SendMsg(msg);
}

void IObjectManager::InitThread(uint16_t nThread, uint16_t bufSz)
{
    if (nThread < 1)
        nThread = 1;

    if (nThread > 255 || m_lsThread.size()>0)
        return;

    for (int i=0; i<nThread; ++i)
    {
        BussinessThread *t = new BussinessThread(i, this);
        if(t)
        {
            t->InitialBuff(bufSz);
            m_lsThread.push_back(t);
            if (m_threadMutexts.find(t) == m_threadMutexts.end())
                m_threadMutexts[t] = new ObjectMetuxs();
        }
    }
}

BussinessThread *IObjectManager::GetPropertyThread() const
{
    int nCount = -1;
    BussinessThread *ret = 0;
    for (BussinessThread *t : m_lsThread)
    {
        ObjectsMap::const_iterator itr = m_mapThreadObject.find(t);
        if (itr == m_mapThreadObject.end())
            return m_lsThread.back();

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