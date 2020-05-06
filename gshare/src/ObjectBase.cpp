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
    BussinessThread(IObjectManager *mgr) :Thread(true)
    , m_mgr(mgr), m_szBuff(0), m_buff(NULL)
    {
    }
    ~BussinessThread() 
    {
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

    void RemoveObject(const std::string &id)
    {
        auto itr = m_links.find(id);
        if (itr != m_links.end())
            m_links.erase(itr);
    }
protected:
    bool RunLoop()
    {
        if (!m_mgr)
            return false;
        bool ret = false;
        while (!m_lsMsgRelease.IsEmpty())
        {
            delete m_lsMsgRelease.Pop();
        }
        ProcessAddLinks();
        m_mgr->ProcessBussiness(this);
        uint64_t ms = Utility::msTimeTick();
        auto itr = m_links.begin();
        for (; itr != m_links.end(); ++itr)
        {
            ILink *l = itr->second;
            if (l->PrcsBussiness(ms, *this))
                ret = true;
            if (l->IsRealse())
            {
                m_lsMsgSend.Push(new ObjectEvent(itr->first, m_mgr->GetObjectType(), ObjectEvent::E_Release));
                auto tmp = itr++;
                m_links.erase(tmp);
                if (itr==m_links.end())
                    break;
            }
        }
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
                m_links[id] = l;
        }
    }
private:
    friend class ILink;
    friend class IObject;
    friend class IObjectManager;
    IObjectManager  *m_mgr;
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
ILink::ILink() : m_tmLastInfo(Utility::msTimeTick())
, m_sock(NULL), m_recv(NULL), m_bLogined(false)
, m_bChanged(false), m_bRelease(false)
{
}

ILink::~ILink()
{
    if (m_sock)
    {
        m_sock->SetObject(NULL);
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

void ILink::OnLogined(bool suc)
{
    m_bLogined = suc;
    if (!suc && m_sock)
        m_sock->Close();

    if (IObject *o = GetParObject())
        o->GetManager()->Log(0, o->GetObjectID(), 0, "[%s:%d]%s", m_sock->GetHost().c_str(), m_sock->GetPort(), suc ? "logined" : "login fail");
}

void ILink::OnSockClose(ISocket *s)
{
    if (m_sock == s)
    {
        m_sock = NULL;
        OnConnected(false);
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

void ILink::CheckTimer(uint64_t ms)
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
    if (buf && len && m_sock)
        return m_sock->Send(len, buf);

    return 0;
}

void ILink::SetSocket(ISocket *s)
{
    if (m_sock)
        return;

    m_sock = s;
    if (s)
        m_tmLastInfo = Utility::msTimeTick();
    OnConnected(s != NULL);
    if (s)
        s->SetObject(this);
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

void ILink::ClearRecv(int n)
{
    m_bChanged = false;
    if (m_recv)
        m_recv->Clear(n < 0 ? m_recv->Count() : n);
}

bool ILink::PrcsBussiness(uint64_t ms, BussinessThread &t)
{
    CheckTimer(ms);
    if (IsChanged())
    {
        int len = CopyData(t.m_buff, t.m_szBuff);
        len = ProcessReceive(t.m_buff, len);
        ClearRecv(len);
        return true;
    }
    return false;
}

void ILink::Release()
{
    m_bRelease = true;
}

bool ILink::IsRealse()
{
    return m_bRelease;
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

char *IObject::GetThreadBuff() const
{
    if (IObjectManager *m = GetManager())
        return m->GetThread(Utility::ThreadID())->m_buff;

    return NULL;
}

int IObject::GetThreadBuffLength() const
{
    if (IObjectManager *m = GetManager())
        return m->GetThread(Utility::ThreadID())->m_szBuff;

    return 0;
}

void IObject::Subcribe(const std::string &sender, int msg)
{
    GetManager()->Subcribe(this, sender, msg);
}

void IObject::Unsubcribe(const std::string &sender, int msg)
{
    GetManager()->Unsubcribe(this, sender, msg);
}

bool IObject::SendMsg(IMessage *msg)
{
    if (IObjectManager *mgr = GetManager())
        return mgr->SendMsg(msg);

    return false;
}

ILink *IObject::GetHandle()
{
    return NULL;
}

bool IObject::IsInitaled() const
{
    return m_stInit == Initialed;
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
    if (m_lsThread.empty() || s==m_lsThread.front())
    {
        InitManager();
        ret = ProcessLogins(s);
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
            auto itr = m_objects.find(msg->GetReceiverID());
            if (itr != m_objects.end())
            { 
                auto o = itr->second;
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
            if (!PrcsPublicMsg(*msg))
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
    if (!t || m_loginSockets.IsEmpty())
        return false;

    while (!m_loginSockets.IsEmpty())
    {
        ISocket *s = m_loginSockets.Pop();
        int len = s->CopyData(t->m_buff, t->m_szBuff);

        IObject *o = PrcsNotObjectReceive(s, t->m_buff, len);
        if (!o)
            continue;

        AddObject(o);
        if (ILink *h = o->GetHandle())
        {
            BussinessThread *t = GetPropertyThread();
            h->SetSocket(s);
            t->m_linksAdd.Push(h);
        }

        return o != NULL;
    }
    return true;
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

bool IObjectManager::ParseRequest(ISocket *s, const char *buf, int len)
{
    if (s && IsHasReuest(buf, len))
    {
        m_mtx->Lock();
        m_loginSockets.Push(s);
        m_mtx->Unlock();
        return true;
    }
    return false;
}

bool IObjectManager::IsHasReuest(const char *buf, int len)const
{
    return false;
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

void IObjectManager::Subcribe(IObject *o, const std::string &sender, int tpMsg)
{
    if (!o || o->GetObjectID().empty() || sender.empty() || tpMsg < 0)
        return;
    if (SubcribeStruct *sub = new SubcribeStruct(o->GetObjectID(), sender, tpMsg, true))
    {
        m_mtx->Lock();
        m_subcribeQue.Push(sub);
        m_mtx->Unlock();
    }
}

void IObjectManager::Unsubcribe(IObject *o, const std::string &sender, int tpMsg)
{
    if (!o || o->GetObjectID().empty() || sender.empty() || tpMsg < 0)
        return;

    if (SubcribeStruct *sub = new SubcribeStruct(o->GetObjectID(), sender, tpMsg, false))
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
