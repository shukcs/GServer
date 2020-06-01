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

typedef std::map<std::string, ILink*> MapLinks;
typedef LoopQueue<ILink*> LinksQue;
//////////////////////////////////////////////////////////////////
//BussinessThread
//////////////////////////////////////////////////////////////////
class BussinessThread : public Thread
{
public:
    BussinessThread(IObjectManager *mgr) :Thread(true), m_mgr(mgr)
    , m_mtx(NULL), m_szBuff(0), m_buff(NULL)
    {
    }
    ~BussinessThread() 
    {
        delete m_mtx;
        delete m_buff;
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
    IMutex *GetMutex()
    {
        if (!m_mtx)
            m_mtx = new Mutex();
        return m_mtx;
    }
protected:
    void ReleasePrcsdMsg()
    {
        while (!m_lsMsgRelease.IsEmpty())
        {
            delete m_lsMsgRelease.Pop();
        }
    }
    bool CheckLinks()
    {
        bool ret = false;
        uint64_t ms = Utility::msTimeTick();
        auto itr = m_links.begin();
        for (; itr != m_links.end(); ++itr)
        {
            ILink *l = itr->second;
            if (l->PrcsBussiness(ms, *this))
                ret = true;

            if (l->IsRealse())
            {
                auto obj = l->GetParObject();
                if (obj && obj->IsAllowRelease())
                    m_lsMsgSend.Push(new ObjectEvent(obj, obj->GetObjectType()));
                auto tmp = itr++;
                m_links.erase(tmp);
                if (itr == m_links.end())
                    break;
            }
        }
        return ret;
    }
    bool RunLoop()
    {
        if (!m_mgr)
            return false;
        ReleasePrcsdMsg();
        ProcessAddLinks();
        bool ret = m_mgr->ProcessBussiness(this);
        if (m_mgr->IsReceiveData() && CheckLinks())
            return true;

        return ret;
    }
    void ProcessAddLinks()
    {
        while (!m_linksAdd.IsEmpty())
        {
            ILink *l = m_linksAdd.Pop();
            IObject *o = l ? l->GetParObject() : NULL;
            auto id = o ? o->GetObjectID() : string();
            if (!id.empty() && m_links.find(id)==m_links.end())
            {
                l->SetMutex(GetMutex());
                m_links[id] = l;
            }
        }
    }
private:
    friend class ILink;
    friend class IObject;
    friend class IObjectManager;
    IObjectManager  *m_mgr;
    IMutex          *m_mtx;
    int             m_szBuff;
    char            *m_buff;
    MapLinks        m_links;
    LinksQue        m_linksAdd;
    MessageQue      m_lsMsgSend;        //发送消息队列
    MessageQue      m_lsMsgRelease;     //释放等待队列
};
////////////////////////////////////////////////////////////////////////
//SubcribeStruct
////////////////////////////////////////////////////////////////////////
class SubcribeStruct
{
public:
    SubcribeStruct(const string &subcr, const string &sender, int msgType, bool bSubcribe)
        : m_sender(sender), m_msgType(msgType), m_subcribeId(subcr)
        , m_bSubcribe(bSubcribe)
    {
    }
public:
    string  m_sender;
    int     m_msgType;
    string  m_subcribeId;
    bool    m_bSubcribe;
};
///////////////////////////////////////////////////////////////////////////////////////
//SocketHandle
///////////////////////////////////////////////////////////////////////////////////////
ILink::ILink() : m_tmLastInfo(Utility::msTimeTick()), m_sock(NULL)
, m_recv(NULL), m_mtx(NULL), m_thread(NULL), m_bLogined(false)
, m_bChanged(false), m_bRelease(false)
{
}

ILink::~ILink()
{
    if (m_sock)
    {
        m_sock->SetHandleLink(NULL);
        m_sock->Close();
    }
    delete m_recv;
}

void ILink::SetBuffSize(uint16_t sz)
{
    if (sz < 32)
        sz = 32;
    if (!m_recv)
        m_recv = new LoopQueBuff(sz);
    else
        m_recv->ReSize(sz);
}

void ILink::OnLogined(bool suc, ISocket *s)
{
    if (!s)
        s = m_sock;

    m_bLogined = suc;
    if (!suc && s)
        s->Close();

    IObject *o = GetParObject();
    if (s && o)
        o->GetManager()->Log(0, o->GetObjectID(), 0, "[%s:%d]%s", s->GetHost().c_str(), s->GetPort(), suc ? "logined" : "login fail");
}

bool ILink::CanSend() const
{
    Lock l(m_mtx);
    return m_sock && m_sock->IsNoWriteData();
}

void ILink::OnSockClose(ISocket *s)
{
    if (m_sock == s && m_mtx)
    {
        m_mtx->Lock();
        SetSocket(NULL);
        m_mtx->Unlock();
    }
}

bool ILink::ChangeLogind(bool b)
{
    if (m_sock && b)
        m_bLogined = true;
    else if (!m_sock && !b && m_bLogined)
        m_bLogined = false;
    else
        return false;

    return true;
}

void ILink::CheckTimer(uint64_t)
{
    IObject *o = GetParObject();
    if (o)
    {
        if (IObject::Uninitial == o->m_stInit)
            o->InitObject();

        if (ChangeLogind(false))
            o->GetManager()->Log(0, o->GetObjectID(), 0, "disconnect");
    }
}

int ILink::Send(const char *buf, int len)
{
    Lock l(m_mtx);
    if (buf && len && m_sock)
        return m_sock->Send(len, buf);

    return 0;
}

void ILink::SetSocket(ISocket *s)
{
    if (m_sock != s)
    {
        m_sock = s;
        OnConnected(s != NULL);
        if (s)
        {
            s->SetHandleLink(this);
            m_tmLastInfo = Utility::msTimeTick();
        }
    }
}

ISocket *ILink::GetSocket() const
{
    return m_sock;
}

bool ILink::Receive(const void *buf, int len)
{
    bool ret = false;
    if (buf && len > 0)
        ret = m_recv->Push(buf, len);

    if (ret)
    {
        m_tmLastInfo = Utility::msTimeTick();
        m_bChanged = true;
    }
    return ret;
}

bool ILink::IsChanged() const
{
    return m_sock && m_recv && m_bChanged;
}

int ILink::CopyData(void *data, int len) const
{
    return m_recv ? m_recv->CopyData(data, len):0;
}

void ILink::SetMutex(IMutex *m)
{
    m_mtx = m;
}

void ILink::SetThread(BussinessThread *t)
{
    m_thread = t;
}

void ILink::ClearRecv(int n)
{
    m_bChanged = false;
    if (m_recv)
        m_recv->Clear(n < 0 ? m_recv->Count() : n);
}

bool ILink::PrcsBussiness(uint64_t ms, BussinessThread &t)
{
    bool ret = false;
    if (IsChanged())
    {
        int len = CopyData(t.m_buff, t.m_szBuff);
        len = ProcessReceive(t.m_buff, len);
        ClearRecv(len);
        ret = true;
    }
    CheckTimer(ms);
    return ret;
}

void ILink::Release()
{
    m_bRelease = true;
}

bool ILink::IsRealse()
{
    return m_bRelease;
}

char *ILink::GetThreadBuff() const
{
    if (m_thread)
        return m_thread->m_buff;

    return NULL;
}

int ILink::GetThreadBuffLength() const
{
    if (m_thread)
        return m_thread->m_szBuff;

    return 0;
}
/////////////////////////////////////////////////////////////////////////////////////////
//IObject
/////////////////////////////////////////////////////////////////////////////////////////
IObject::IObject(const string &id):m_id(id)
, m_stInit(Uninitial)
{
}

IObject::~IObject()
{
}

void IObject::Subcribe(const string &sender, int tpMsg)
{
    if (IObjectManager *mgr = GetManager())
        mgr->Subcribe(m_id, sender, tpMsg);
}

void IObject::Unsubcribe(const string &sender, int tpMsg)
{
    if (IObjectManager *mgr = GetManager())
        mgr->Unsubcribe(m_id, sender, tpMsg);
}

const string &IObject::GetObjectID() const
{
    return m_id;
}

void IObject::SetObjectID(const std::string &id)
{
    m_id = id;
}

void IObject::PushReleaseMsg(IMessage *msg)
{
    if (IObjectManager *m = GetManager())
        m->PushReleaseMsg(msg);
}

bool IObject::SendMsg(IMessage *msg)
{
    if (IObjectManager *mgr = GetManager())
        return mgr->SendMsg(msg);

    return false;
}

ILink *IObject::GetLink()
{
    return NULL;
}

bool IObject::IsInitaled() const
{
    return m_stInit == Initialed;
}

bool IObject::IsAllowRelease() const
{
    return true;
}

IObjectManager *IObject::GetManager() const
{
    return GetManagerByType(GetObjectType());
}

IObjectManager *IObject::GetManagerByType(int tp)
{
    return ObjectManagers::Instance().GetManagerByType(tp);
}
//////////////////////////////////////////////////////////////////
//IObjectManager
//////////////////////////////////////////////////////////////////
IObjectManager::IObjectManager() : m_log(NULL), m_mtx(NULL)
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
}

void IObjectManager::LoadConfig()
{
}

void IObjectManager::PushReleaseMsg(IMessage *msg)
{
    BussinessThread *t = msg ? GetThread(msg->CreateThreadID()) : NULL;
    if (t)
        t->m_lsMsgRelease.Push(msg);
    else
        delete msg;
}

bool IObjectManager::PrcsPublicMsg(const IMessage &)
{
    return false;
}

void IObjectManager::ToCurrntLog(int err, const std::string &obj, int evT, const std::string &dscb)
{
    ObjectManagers::GetLog().Log(dscb, obj, evT, err);
}

bool IObjectManager::ProcessBussiness(BussinessThread *s)
{
    bool ret = false;
    if (s && !m_lsThread.empty() && s==m_lsThread.front())
    {
        if (!m_mtx)
            m_mtx = s->GetMutex();

        if (IsReceiveData())
        {
            m_mtx->Lock();
            ret = ProcessLogins(s);
            m_mtx->Unlock();
        }

        PrcsSubcribes();
        ProcessMessage();
    }
    return ret;
}

bool IObjectManager::Exist(IObject *obj) const
{
    if (!obj)
        return false;

    return m_objects.find(obj->GetObjectID()) != m_objects.end();
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

void IObjectManager::ProcessMessage()
{
    while (!m_messages.IsEmpty())
    {
        IMessage *msg = m_messages.Pop();
        if (!msg)
            continue;

        if (msg->GetMessgeType() == ObjectEvent::E_Release)
        {
            auto itr = m_objects.find(msg->GetSenderID());
            auto o = itr!=m_objects.end() ? itr->second : NULL;
            if (o)
            { 
                m_objects.erase(itr);
                delete o;
            }
        }
        else if (msg->IsValid())
        {
            const StringList &sbs = getMessageSubcribes(msg);
            for (const string id : sbs)
            {
                if (IObject *obj = GetObjectByID(id))
                    obj->ProcessMessage(msg);
            }
            const string &rcv = msg->GetReceiverID();
            if (IObject *obj = GetObjectByID(rcv))
            {
                obj->ProcessMessage(msg);
                continue;
            }
            if (!PrcsPublicMsg(*msg) && rcv.empty())
            {
                for (const pair<string, IObject*> &o : m_objects)
                {
                    o.second->ProcessMessage(msg);
                }
            }
        }
        m_lsMsgRecycle.Push(msg);
    }
}

bool IObjectManager::ProcessLogins(BussinessThread *t)
{
    if (!t || m_loginSockets.empty())
        return false;

    while (!m_loginSockets.empty())
    {
        ISocket *s = m_loginSockets.front();
        m_loginSockets.pop_front();
        if (!s)
            continue;

        int len = s->CopyData(t->m_buff, t->m_szBuff);
        IObject *o = PrcsNotObjectReceive(s, t->m_buff, len);
        if (!o)
            continue;

        s->ClearBuff();
        AddObject(o);
        if (ILink *h = o->GetLink())
        {
            BussinessThread *tmp = GetPropertyThread();
            h->SetSocket(s);
            h->SetThread(tmp);
            t->m_linksAdd.Push(h);
        }

        return o != NULL;
    }
    return true;
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

bool IObjectManager::ParseRequest(ISocket *s, const char *buf, int len)
{
    if (s && IsHasReuest(buf, len) && m_mtx)
    {
        m_mtx->Lock();
        m_loginSockets.push_back(s);
        m_mtx->Unlock();
        return true;
    }
    return false;
}

bool IObjectManager::IsHasReuest(const char *, int)const
{
    return false;
}

bool IObjectManager::IsReceiveData() const
{
    return true;
}

bool IObjectManager::AddObject(IObject *obj)
{
    if (!obj || Exist(obj))
        return false;

    m_objects[obj->GetObjectID()] = obj;
    return true;
}

IObject *IObjectManager::GetObjectByID(const std::string &id) const
{
    auto itrObj = m_objects.find(id);
    if (itrObj != m_objects.end())
        return itrObj->second;

    return NULL;
}

void IObjectManager::PushManagerMessage(IMessage *msg)
{
    if (!msg)
        return;

    m_messages.Push(msg);
}

IMessage *IObjectManager::PopRecycleMessage()
{
    if (m_lsMsgRecycle.IsEmpty())
        return NULL;

    return m_lsMsgRecycle.Pop();
}

bool IObjectManager::SendMsg(IMessage *msg)
{
    if (auto t = GetThread(msg?msg->CreateThreadID(): -1))
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

BussinessThread *IObjectManager::GetThread(int id) const
{
    for (auto itr : m_lsThread)
    {
        if (itr->GetThreadId() == (uint32_t)id)
            return itr;
    }
    return NULL;
}

void IObjectManager::OnSocketClose(ISocket *s)
{
    if (!IsReceiveData())
        return;

    if (m_mtx && s)
    {
        m_mtx->Lock();
        m_loginSockets.remove(s);
        m_mtx->Unlock();
    }
}

IObjectManager *IObjectManager::MangerOfType(int type)
{
    return ObjectManagers::Instance().GetManagerByType(type);
}

void IObjectManager::PrcsSubcribes()
{
    while (!m_subcribeQue.IsEmpty())
    {
        SubcribeStruct *sub = m_subcribeQue.Pop();
        if (sub->m_bSubcribe)
        {
            StringList &ls = m_subcribes[sub->m_sender][sub->m_msgType];
            if (!IObject::IsContainsInList(ls, sub->m_subcribeId))
                ls.push_back(sub->m_subcribeId);
        }
        else
        {
            MessageSubcribes::iterator itr = m_subcribes.find(sub->m_sender);
            if (itr != m_subcribes.end())
            {
                auto it = itr->second.find(sub->m_msgType);
                if (it != itr->second.end())
                {
                    it->second.remove(sub->m_subcribeId);
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

const StringList &IObjectManager::getMessageSubcribes(IMessage *msg)
{
    static StringList sEpty;
    int tpMsg = msg ? msg->GetMessgeType() : -1;
    string sender = msg ? msg->GetSenderID() : string();
    if (!sender.empty() || tpMsg >= 0)
    {
        MessageSubcribes::iterator itr = m_subcribes.find(sender);
        if (itr != m_subcribes.end())
        {
            auto it = itr->second.find(tpMsg);
            if (it != itr->second.end())
                return it->second;
        }
    }
    return sEpty;
}

void IObjectManager::Subcribe(const string &dsub, const std::string &sender, int tpMsg)
{
    if (!m_mtx || dsub.empty() || sender.empty() || tpMsg < 0)
        return;

    if (SubcribeStruct *sub = new SubcribeStruct(dsub, sender, tpMsg, true))
    {
        m_mtx->Lock();
        m_subcribeQue.Push(sub);
        m_mtx->Unlock();
    }
}

void IObjectManager::Unsubcribe(const string &dsub, const std::string &sender, int tpMsg)
{
    if (!m_mtx || dsub.empty() || sender.empty() || tpMsg < 0)
        return;

    if (SubcribeStruct *sub = new SubcribeStruct(dsub, sender, tpMsg, false))
    {
        m_mtx->Lock();
        m_subcribeQue.Push(sub);
        m_mtx->Unlock();
    }
}

BussinessThread *IObjectManager::GetPropertyThread() const
{
    if (m_lsThread.empty())
        return NULL;

    BussinessThread *ret = m_lsThread.front();
    int minLink = -1;
    for (BussinessThread *t : m_lsThread)
    {
        if (ret==t)
            continue;

        int tmp = t->m_links.size() + t->m_linksAdd.ElementCount();
        if (minLink == -1 || tmp < minLink)
        {
            minLink = tmp;
            ret = t;
        }     
    }

    return ret;
}
