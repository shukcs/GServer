#include "ObjectBase.h"
#include "socketBase.h"
#include "Thread.h"
#include "Mutex.h"
#include "ObjectManagers.h"
#include "Utility.h"
#include "Lock.h"
#include "IMessage.h"
#include "ILog.h"
#include <stdarg.h>

using namespace std;
#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif

typedef std::map<std::string, IObject*> ThreadObjects;
//////////////////////////////////////////////////////////////////
//BussinessThread
//////////////////////////////////////////////////////////////////
class BussinessThread : public Thread
{
public:
    BussinessThread(IObjectManager *mgr) :Thread(true)
    , m_mgr(mgr), m_szBuff(0), m_buff(NULL)
    {
    }
    ~BussinessThread() 
    {
        ThreadObjects::iterator itr = m_objects.begin();
        for (; itr != m_objects.end(); ++itr)
        {
            ObjectManagers::Instance().Destroy(itr->second);
        }
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
            m_buff = new char[sz];
            if(m_buff)
                m_szBuff = sz;
        }
    }
    IObject *GetObject(const string &id)
    {
        if (id.empty())
            return NULL;
        ThreadObjects::const_iterator itr = m_objects.find(id);
        if (itr != m_objects.end())
            return itr->second;

        return NULL;
    }
    void RemoveObject(const std::string &id, IMutex &mtx)
    {
        ThreadObjects::iterator itr = m_objects.find(id);
        if (itr != m_objects.end())
        {
            IObject *o = itr->second;
            mtx.Lock();
            m_objects.erase(itr);
            mtx.Unlock();
            ObjectManagers::Instance().Destroy(o);
        }
    }
protected:
    bool RunLoop()
    {
        if (!m_mgr)
            return false;

        while (!m_lsMsgRecycle.IsEmpty())
        {
            delete m_lsMsgRecycle.Pop();
        }
        while (!m_lsMsg.IsEmpty())
        {
            IMessage *msg = m_lsMsg.Pop();
            if (msg && msg->IsValid())
            {
                if (IObject *obj = GetObject(msg->GetReceiverID()))
                    obj->ProcessMessage(msg);
                else
                    m_mgr->ProcessMessage(msg);

                m_lsMsgRelease.Push(msg);
            }
        }
        m_mgr->ProcessBussiness(this);
        return m_mgr->PrcsObjectsOfThread(*this);
    }
private:
    friend class IObject;
    friend class IObjectManager;
    IObjectManager  *m_mgr;
    int             m_szBuff;
    char            *m_buff;
    ThreadObjects   m_objects;
    MessageQue      m_lsMsg;            //接收消息队列
    MessageQue      m_lsMsgRecycle;     //消息回收队列
    MessageQue      m_lsMsgSend;        //发送消息队列
    MessageQue      m_lsMsgRelease;     //释放等待队列
};
/////////////////////////////////////////////////////////////////////////////////////////
//IObject
/////////////////////////////////////////////////////////////////////////////////////////
IObject::IObject(ISocket *sock, const string &id): m_tmLastInfo(Utility::msTimeTick())
, m_sock(sock), m_id(id), m_bRelease(false), m_stInit(Uninitial), m_buff(NULL)
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
    if (m_thread)
        m_thread->m_lsMsgRecycle.Push(msg);
}

bool IObject::PrcsBussiness(uint64_t ms)
{
    bool ret = false;
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

bool IObject::SendMsg(IMessage *msg)
{
    if (!m_thread || !msg)
        return false;

    return m_thread->m_lsMsgSend.Push(msg);
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
    if (m_thread && msg)
        m_thread->m_lsMsg.Push(msg);
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

IObjectManager *IObject::GetManagerByType(int tp)
{
    return ObjectManagers::Instance().GetManagerByType(tp);
}

bool IObject::Receive(const void *buf, int len)
{
    bool ret = false;
    if (buf && len > 0)
        ret = m_buff->Push(buf, len);

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
    for (BussinessThread *t : m_lsThread)
    {
        t->SetRunning(false);
    }
    Utility::Sleep(100);
    m_lsThread.clear();
    delete m_mtx;
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
    if (IObject *tmp = GetObjectByID(msg->GetSenderID()))
        tmp->PushReleaseMsg(msg);
    else if (!m_lsThread.empty())
        m_lsThread.front()->m_lsMsgRecycle.Push(msg);
}

bool IObjectManager::PrcsPublicMsg(const IMessage &)
{
    return false;
}

void IObjectManager::ToCurrntLog(int err, const std::string &obj, int evT, const std::string &dscb)
{
    ObjectManagers::GetLog().Log(dscb, obj, evT, err);
}

void IObjectManager::ProcessBussiness(BussinessThread *s)
{
    if (!m_lsThread.empty() && s==m_lsThread.front())
        InitManager();
}

bool IObjectManager::PrcsObjectsOfThread(BussinessThread &t)
{
    bool ret = false;
    ThreadObjects::iterator itr = t.m_objects.begin();
    uint64_t ms = Utility::msTimeTick();
    while (itr != t.m_objects.end())
    {
        if (itr->second->PrcsBussiness(ms))
            ret = true;

        if (itr->second->IsRealse())
        {
            ThreadObjects::iterator tmp = itr++;
            t.RemoveObject(tmp->first, *m_mtx);
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

    BussinessThread *t = obj->GetThread();
    if (t)
    {
        ThreadObjects::const_iterator itr = t->m_objects.find(obj->GetObjectID());
        if (itr != t->m_objects.end())
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
    ToCurrntLog(err, obj, evT, slask);
}

void IObjectManager::ProcessMessage(IMessage *msg)
{
    if (PrcsPublicMsg(*msg))
        return;

    for (BussinessThread *itr : m_lsThread)
    {
        for (const pair<string, IObject*> &o : itr->m_objects)
        {
            o.second->ProcessMessage(msg);
        }
    }
}

MessageQue *IObjectManager::GetReleaseQue(int idThread)const
{
    if (idThread >= (int)m_lsThread.size() || idThread<0)
        return NULL;

    auto itr = m_lsThread.begin();
    for (; idThread > 0; --idThread)
    {
        ++itr;
    }
    return &(*itr)->m_lsMsgRelease;
}

MessageQue *IObjectManager::GetSendQue(int idThread)const
{
    if (idThread >= (int)m_lsThread.size() || idThread<0)
        return NULL;

    auto itr = m_lsThread.begin();
    for (; idThread > 0; --idThread)
    {
        ++itr;
    }
    return &(*itr)->m_lsMsgSend;
}

bool IObjectManager::AddObject(IObject *obj)
{
    if (!obj || Exist(obj))
        return false;

    BussinessThread *t = GetPropertyThread();
    m_mtx->Lock();
    t->m_objects[obj->GetObjectID()] = obj;
    m_mtx->Unlock();
    obj->SetThread(t);

    return true;
}

IObject *IObjectManager::GetObjectByID(const std::string &id) const
{
    for (BussinessThread *itr : m_lsThread)
    {
        ThreadObjects::const_iterator itrObj = itr->m_objects.find(id);
        if(itrObj == itr->m_objects.end())
            continue;

        return itrObj->second;
    }
    return NULL;
}

void IObjectManager::PushManagerMessage(IMessage *msg)
{
    if (!msg)
        return;

    const string id = msg->GetReceiverID();
    IObject *obj = id.empty() ? NULL : GetObjectByID(id);
    if (obj)
        obj->PushMessage(msg);
    else if (!m_lsThread.empty())
        m_lsThread.front()->m_lsMsg.Push(msg);
}

bool IObjectManager::SendMsg(IMessage *msg)
{
    if (auto t = CurrentThread())
        return t->m_lsMsgSend.Push(msg);
    return false;
}

void IObjectManager::InitThread(uint16_t nThread, uint16_t bufSz)
{
    if (nThread < 1)
        nThread = 1;

    if (nThread > 255 || m_lsThread.size()>0)
        return;

    for (int i=0; i<nThread; ++i)
    {
        BussinessThread *t = new BussinessThread(this);
        if(t)
        {
            t->InitialBuff(bufSz);
            m_lsThread.push_back(t);
        }
    }
}

BussinessThread *IObjectManager::CurrentThread() const
{
    uint32_t id = Utility::ThreadID();
    for (auto itr : m_lsThread)
    {
        if (itr->GetThreadId() == id)
            return itr;
    }
    return NULL;
}

BussinessThread *IObjectManager::GetPropertyThread() const
{
    if (m_lsThread.empty())
        return NULL;

    int nCount = m_lsThread.front()->m_objects.size()+5;
    BussinessThread *ret = 0;
    for (BussinessThread *t : m_lsThread)
    {
        int tmp = t->m_objects.size();
        if (ret)
        {
            ret = t;
        }
        else if (tmp < nCount)
        {
            nCount = tmp;
            ret = t;
        }
    }

    return ret;
}