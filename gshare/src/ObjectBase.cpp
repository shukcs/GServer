#include "ObjectBase.h"
#include "socketBase.h"
#include "Thread.h"
#include "Mutex.h"
#include "ObjectManagers.h"
#include "Utility.h"
#include "Lock.h"

using namespace std;

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

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

        for (IObject *o : m_mgr->GetThreadObject(m_id))
        {
            if (o->PrcsBussiness())
                return ret = true;
        }

        return ret;
    }
private:
    IObjectManager  *m_mgr;
    int             m_id;
};

////////////////////////////////////////////////////////////////////////////////////////
//IMessage
///////////////////////////////////////////////////////////////////////////////////////
IMessage::IMessage(IObject *sender, const std::string &id, int rcv, int tpMsg)
: m_idRcv(id), m_tpRcv(rcv), m_tpMsg(tpMsg), m_tpSender(IObject::UnKnow), m_sender(sender)
, m_bRelease(false)
{
    if (sender)
        m_tpSender = m_sender->GetObjectType();
}

IMessage::IMessage(IObjectManager *sender, const std::string &id, int rcv, int tpMsg)
: m_idRcv(id), m_tpRcv(rcv), m_tpMsg(tpMsg), m_tpSender(IObject::UnKnow), m_sender(NULL)
, m_bRelease(false)
{
    if (sender)
        m_tpSender = sender->GetObjectType();
}

int IMessage::GetReceiverType() const
{
    return m_tpRcv;
}

int IMessage::GetMessgeType() const
{
    return m_tpMsg;
}

const std::string &IMessage::GetReceiverID() const
{
    return m_idRcv;
}

int IMessage::GetSenderType() const
{
    return  m_sender ? m_sender->GetObjectType() : IObject::UnKnow;
}

const std::string &IMessage::GetSenderID() const
{
    static const string def;
    return m_sender ? m_sender->GetObjectID() : def;
}

IObject *IMessage::GetSender() const
{
    return m_sender;
}

bool IMessage::IsValid() const
{
    return (m_sender||m_tpSender>IObject::UnKnow) && m_tpRcv>IObject::UnKnow && !m_bRelease;
}

void IMessage::Release()
{
    IObjectManager *om = ObjectManagers::Instance().GetManagerByType(m_tpRcv);
    if (!om)
    {
        delete this;
        return;
    }
    IObject *obj = m_idRcv.size() > 0 ? om->GetObjectByID(m_idRcv) : NULL;
    if (obj)
        obj->RemoveRcvMsg(this);
    else
        om->RemoveRcvMsg(this);

    if (m_sender)
        m_sender->AddRelease(this);
    else if (IObjectManager *m = ObjectManagers::Instance().GetManagerByType(m_tpSender))
        m->AddRelease(this);
}
//////////////////////////////////////////////////////////////////
//IObject
//////////////////////////////////////////////////////////////////
IObject::IObject(ISocket *sock, const string &id)
    : m_sock(sock), m_id(id), m_bRelease(false)
    , m_mtx(new Mutex), m_mtxMsg(new Mutex)
    , m_idThread(-1)
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

int IObject::CopySend(char *buf, int sz, unsigned form)
{
    return 0;
}

void IObject::SetSended(int)
{
}

void IObject::RemoveRcvMsg(IMessage *msg)
{
    Lock l(*m_mtxMsg);
    m_lsMsg.remove(msg);
}

void IObject::AddRelease(IMessage *msg)
{
    Lock l(*m_mtxMsg);
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
        m_mtxMsg->Lock();
        m_lsMsg.pop_front();
        m_mtxMsg->Unlock();
        if (msg->IsValid())
        {
            ProcessMassage(*msg);
            msg->Release();
        }
    }

    if (m_lsMsgRelease.size() > 0)
    {
        for (IMessage *msg : m_lsMsgRelease)
        {
            delete msg;
        }
        m_mtxMsg->Lock();
        m_lsMsgRelease.clear();
        m_mtxMsg->Unlock();
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
    Lock l(*m_mtxMsg);
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
    return ObjectManagers::Instance().GetManagerByType(GetObjectType());
}

bool IObject::SendMsg(IMessage *msg)
{
    return ObjectManagers::Instance().SendMsg(msg);
}

bool IObject::Receive(const void *buf, int len)
{
    bool ret = false;
    if (m_mtx && len>0)
    {
        Lock l(*m_mtx);
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
    for (const pair<int, std::list<IObject*> > &itr : m_mapThreadObject)
    {
        for (IObject *o : itr.second)
        {
            o->Release();
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

    IObject *o = ProcessReceive(s, (char*)buf, len);
    if (o)
        o->SetSocket(s);

    if (o && len < buff.Count())
        o->Receive((char*)buf + len, buff.Count() - len);

    return o != NULL;
}

void IObjectManager::RemoveRcvMsg(IMessage *msg)
{
    m_mtx->Lock();
    m_lsMsg.remove(msg);
    m_mtx->Unlock();
}

void IObjectManager::AddRelease(IMessage *msg)
{
    m_mtx->Lock();
    m_lsMsgRelease.push_back(msg);
    m_mtx->Unlock();
}

void IObjectManager::PrcsRelease()
{
    if (m_lsMsgRelease.size() < 1)
        return;

    m_mtx->Lock();
    for (IMessage *itr : m_lsMsgRelease)
    {
        delete itr;
    }
    m_lsMsgRelease.clear();
    m_mtx->Unlock();
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
        for (const pair<int, std::list<IObject*>> &itr : m_mapThreadObject)
        {
            for (IObject *o : itr.second)
            {
                o->PrcsBussiness();
                ret = true;
            }
        }
    }
    PrcsRelease();
    return ret;
}

void IObjectManager::RemoveObject(IObject *o)
{
    if (!o)
        return;

    ObjectMap::iterator itr = m_mapThreadObject.find(o->GetThreadId());
    if (itr != m_mapThreadObject.end())
    {
        m_mtx->Lock();
        itr->second.remove(o);
        m_mtx->Unlock();
    }
}

const list<IObject*> &IObjectManager::GetThreadObject(int t) const
{
    static list<IObject*> sLsEmpty;
    ObjectMap::const_iterator itr = m_mapThreadObject.find(t);
    if (itr != m_mapThreadObject.end())
        return itr->second;

    return sLsEmpty;
}

void IObjectManager::ProcessMassage(const IMessage &msg)
{
    if (PrcsRemainMsg(msg))
        return;

    for (const pair<int, std::list<IObject*>> &itr : m_mapThreadObject)
    {
        for (IObject *o : itr.second)
        {
            o->ProcessMassage(msg);
        }
    }
}

bool IObjectManager::AddObject(IObject *obj)
{
    if (!obj)
        return false;

    int n = GetPropertyThread();
    m_mtx->Lock();
    m_mapThreadObject[n].push_back(obj);
    obj->SetThreadId(n);
    m_mtx->Unlock();

    return true;
}

IObject *IObjectManager::GetObjectByID(const std::string &id) const
{
    for (const pair<int, std::list<IObject*>> &itr : m_mapThreadObject)
    {
        for (IObject *ret : itr.second)
        {
            if(id == ret->GetObjectID())
                return ret;
        }
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
    return ObjectManagers::Instance().SendMsg(msg);
}

void IObjectManager::InitThread(uint16_t nThread /*= 1*/)
{
    if (nThread > 255 || nThread < 1 || m_lsThread.size()>0)
        return;

    for (int i=0; i<nThread; ++i)
    {
        if(BussinessThread *t = new BussinessThread(i, this))
            m_lsThread.push_back(t);
    }
}

int IObjectManager::GetPropertyThread() const
{
    int nCount = -1;
    int ret = -1;
    for (BussinessThread *t : m_lsThread)
    {
        ObjectMap::const_iterator itr = m_mapThreadObject.find(t->GetId());
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

#ifdef SOCKETS_NAMESPACE
}
#endif