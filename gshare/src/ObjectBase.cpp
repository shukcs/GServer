#include "ObjectBase.h"
#include "socketBase.h"
#include "Thread.h"
#include "ObjectManagers.h"
#include "BussinessThread.h"
#include "Utility.h"
#include "Lock.h"
#include "IMessage.h"
#include "ILog.h"
#include <stdarg.h>

using namespace std;
#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif
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
ILink::ILink() : m_bLogined(false), m_sock(NULL), m_recv(NULL), m_szRcv(0), m_pos(0)
, m_mtxS(NULL), m_thread(NULL), m_Stat(Stat_Link)
{
}

ILink::~ILink()
{
    Release();
    delete m_recv;
}

void ILink::SetBuffSize(uint16_t sz)
{
    if (sz < 256)
        sz = 256;

    m_szRcv = sz;
    m_pos = 0;
    if (!m_recv)
        delete m_recv;

    m_recv = new char[sz];
}

void ILink::OnLogined(bool suc, ISocket *s)
{
    if (!s)
        s = m_sock;

    if (s)
    {
        m_bLogined = suc;
        IObject *o = GetParObject();
        if (s && o)
            o->GetManager()->Log(0, IObjectManager::GetObjectFlagID(o), 0, "[%s:%d]%s", s->GetHost().c_str(), s->GetPort(), suc ? "logined" : "login fail");
        if (!suc)
            CloseLink();
    }
}

int ILink::GetSendRemain() const
{
    Lock l(m_mtxS);
    if (m_sock && m_sock->IsNoWriteData())
        return m_sock->GetSendRemain();

    return -1;
}

void ILink::OnSockClose()
{
    OnConnected(false);
    _cleanRcvPacks(true);

    Lock l(m_mtxS);
    m_sock = NULL;
    m_Stat |= Stat_Close;
}

bool ILink::IsConnect() const
{
    return m_bLogined;
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
            o->GetManager()->Log(0, IObjectManager::GetObjectFlagID(o), 0, "disconnect");
    }
}

void ILink::SetSocket(ISocket *s)
{
    Lock l(m_mtxS);
    if (m_sock != s)
    {
        if (s)
            m_Stat = Stat_Link;
        m_sock = s;
        if (s)
            s->SetHandleLink(this);
        OnConnected(s != NULL);
    }
}

bool ILink::IsLinked() const
{
    return m_Stat == Stat_Link;
}

void ILink::SendData(char *buf, int len)
{
    Lock l(m_mtxS);
    if (buf && len>0 && m_sock)
        m_sock->Send(len, buf);
}

int ILink::ParsePack(const char *buf, int len)
{
    auto pf = GetParseRcv();
    if (!pf)
        return len;

    int used = 0;
    uint32_t tmp = len;
    while (tmp > 0)
    {
        auto pack = pf(buf, tmp);
        used += tmp;
        tmp = len - used;
        if (!pack)
            break;

        AddProcessRcv(pack, false);
    }
    return used;
}

ParsePackFuc ILink::GetParseRcv()const
{
    auto mgr = m_thread ? m_thread->GetManager() : NULL;
    if (mgr)
        return mgr->m_pfParsePack;
    return NULL;
}

RelaesePackFuc ILink::GetRlsPacket() const
{
    auto mgr = m_thread ? m_thread->GetManager() : NULL;
    if (mgr)
        return mgr->m_pfRlsPack;
    return NULL;
}

void ILink::AddProcessRcv(void *pack, bool bUsed)
{
    Lock l(m_mtxS);
    if (bUsed)
        m_packetsUsed.push(pack);
    else
        m_packetsRcv.push(pack);
}

void ILink::ProcessRcvPacks()
{
    while (auto pack = PopRcv(false))
    {
        ProcessRcvPack(pack);
        AddProcessRcv(pack);
    }
}

void *ILink::PopRcv(bool bUsed)
{
    void *ret = NULL;
    Lock l(m_mtxS);
    if (bUsed)
        Utility::Pop(m_packetsUsed, ret);
    else
        Utility::Pop(m_packetsRcv, ret);

    return ret;
}

void ILink::SetSocketBuffSize(uint16_t sz)
{
    if (m_sock)
        m_sock->ResizeBuff(sz);
}

ISocket *ILink::GetSocket() const
{
    return m_sock;
}

void ILink::CloseLink()
{
    Lock l(m_mtxS);
    if (m_sock)
    {
        m_sock->Close();
        m_sock = NULL;
    }
    m_Stat = Stat_Close;
}

bool ILink::Receive(const void *buf, int len)
{
    if (!_adjustBuff((const char*)buf, len))
        return false;

    int tmp = ParsePack(m_recv, m_pos);
    if (tmp>0 && tmp<m_pos)
        memcpy(m_recv, m_recv+tmp, m_pos-tmp);
    m_pos -= (tmp<m_pos)? tmp:m_pos;

    return true;
}

void ILink::SetMutex(std::mutex *m)
{
    m_mtxS = m;
}

void ILink::SetThread(BussinessThread *t)
{
    m_thread = t;
    if (m_thread == NULL)
    {
        m_mtxS = NULL;
        CloseLink();
    }
}

BussinessThread *ILink::GetThread() const
{
    return m_thread;
}

void ILink::processSocket(ISocket &s, BussinessThread &t)
{
    if (NULL == m_sock)
    {
        SetSocket(&s);
        FreshLogin(Utility::msTimeTick());
        if (!m_thread)
            t.AddOneLink(this);
    }
    else if (m_sock != &s)
    {
        s.Close(false);
    }
}

void ILink::Release()
{
    CloseLink();
    m_Stat |= Stat_Release;
}

bool ILink::IsRealse()
{
    return Stat_CloseAndRealese == (m_Stat&Stat_CloseAndRealese);
}

bool ILink::IsLinkThread() const
{
    return m_thread && m_thread->GetThreadId() == Utility::ThreadID();
}

bool ILink::_adjustBuff(const char *buf, int len)
{
    if (!buf || len <= 0 || m_szRcv < 256)
        return false;

    int tmp = m_pos + len - m_szRcv;
    if (tmp <= 0)
    {
        memcpy(m_recv + m_pos, buf, len);
        m_pos += len;
    }
    else if (len >= m_szRcv)
    {
        memcpy(m_recv, (char*)buf + len - m_szRcv, m_szRcv);
        m_pos = m_szRcv;
    }
    else
    {
        if (tmp < m_pos)
            memcpy(m_recv, m_recv + tmp, m_pos - tmp);
        m_pos = m_szRcv - len;
        memcpy(m_recv + m_pos, buf, len);
        m_pos = m_szRcv;
    }
    return true;
}

void ILink::_cleanRcvPacks(bool bUnuse)
{
    auto pf = GetRlsPacket();
    while (auto pack = PopRcv(false))
    {
        pf(pack);
    }
    if (bUnuse)
    {
        while (auto pack = PopRcv())
        {
            pf(pack);
        }
    }
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
    return IObjectManager::SendMsg(msg);
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
///////////////////////////////////////////////////////////////////////////
//IObjectManager
///////////////////////////////////////////////////////////////////////////
IObjectManager::IObjectManager() : m_log(NULL), m_mtxLogin(new std::mutex)
, m_mtxMsgs(new std::mutex), m_pfSrlzPack(NULL), m_pfParsePack(NULL)
, m_pfRlsPack(NULL)
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

void IObjectManager::PushReleaseMsg(IMessage *msg)
{
    BussinessThread *t = msg ? GetThread(msg->CreateThreadID()) : NULL;
    if (t==NULL || !t->PushRelaseMsg(msg))
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
        PrcsSubcribes();
        ProcessMessage();
        ProcessEvents();

        if (IsReceiveData())
            ret = ProcessLogins(s);
    }
    return ret;
}

bool IObjectManager::Exist(IObject *obj) const
{
    if (!obj)
        return false;

    return m_objects.find(obj->GetObjectID()) != m_objects.end();
}

void IObjectManager::InitPackPrcs(SerierlizePackFuc srlz, ParsePackFuc prs, RelaesePackFuc rls)
{
    m_pfSrlzPack = srlz;
    m_pfParsePack = prs;
    m_pfRlsPack = rls;
}


void IObjectManager::InitPrcsLogin(const PrcsLoginHandle &hd)
{
    m_pfPrcsLogin = hd;
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
    IMessage *msg = NULL;
    Lock l(m_mtxMsgs);
    while (Utility::Pop(m_messages, msg))
    {
        if (!msg)
            continue;

        if (msg->GetMessgeType() == ObjectSignal::S_Release)
        {
            auto itr = m_objects.find(msg->GetSenderID());
            if (itr != m_objects.end())
            {
                auto obj = itr->second;
                if (!obj->GetLink() || obj->GetLink()->IsRealse())
                {
                    delete itr->second;
                    m_objects.erase(itr);
                }
            }
        }
        else if (msg->IsValid())
        {
            SubcribesProcess(msg);
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

        if (!ObjectManagers::RlsMessage(msg))
            delete msg;
    }
}

bool IObjectManager::ProcessLogins(BussinessThread *t)
{
    bool ret = false;
    if (!t || NULL==m_pfPrcsLogin)
        return ret;

    Lock l(m_mtxLogin);
    for (auto itr = m_loginSockets.begin(); itr!= m_loginSockets.end(); ++itr)
    {
        ISocket *s = itr->first;
        if (s->GetHandleLink())
            continue;

        IObject *o = m_pfPrcsLogin(s, itr->second);
        if (o)
        {
            AddObject(o);
            if (auto link = o->GetLink())
                link->processSocket(*s, *GetPropertyThread());
            ret = true;
        }
    }
    return ret;
}

bool IObjectManager::IsHasReuest(const char *, int)const
{
    return false;
}

void IObjectManager::ProcessEvents()
{
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


IObject *IObjectManager::GetFirstObject() const
{
    auto itrObj = m_objects.begin();
    if (itrObj != m_objects.end())
        return itrObj->second;

    return NULL;
}

void IObjectManager::PushManagerMessage(IMessage *msg)
{
    if (!msg)
        return;

    Lock l(m_mtxMsgs);
    m_messages.push(msg);
}

bool IObjectManager::SendMsg(IMessage *msg)
{
    if (ObjectManagers::SndMessage(msg))
        return true;

    return false;
}


bool IObjectManager::SendData2Link(const std::string &id, void *data)
{
    if (!m_pfSrlzPack || !m_pfRlsPack)
        return false;
    if (auto t = CurThread())
        return t->Send2Link(id, data);

    return false;
}

string IObjectManager::GetObjectFlagID(IObject *o)
{
    string ret;
    if (!o)
        return ret;

    switch (o->GetObjectType())
    {
    case IObject::Plant:
        ret = "UAV:"; break;
    case IObject::GroundStation:
        ret = "GS:"; break;
    case IObject::DBMySql:
        ret = "DB:"; break;
    case IObject::VgFZ:
        ret = "FZ:"; break;
    case IObject::Tracker:
        ret = "Tracker:"; break;
    case IObject::GVMgr:
        ret = "GV:"; break;
    case IObject::GXLink:
        ret = "GXClient:"; break;
    default:
        return ret;
    }
    return ret + o->GetObjectID();
}

void IObjectManager::InitThread(uint16_t nThread, uint16_t bufSz)
{
    if (nThread < 1)
        nThread = 1;

    if (nThread > 255 || m_lsThread.size()>0)
        return;

    for (int i=0; i<nThread; ++i)
    {
        BussinessThread *t = new BussinessThread(*this);
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
    if (s)
    {
        Lock l(m_mtxLogin);
        auto itr = m_loginSockets.find(s);
        if (itr != m_loginSockets.end())
        {
            m_pfRlsPack(itr->second);
            m_loginSockets.erase(itr);
        }
    }
}

int IObjectManager::AddLoginData(ISocket *s, const char *buf, uint32_t len)
{
    if (!s || !m_pfParsePack || !m_pfRlsPack)
        return -1;

    Lock l(m_mtxLogin);
    auto itr = m_loginSockets.find(s);
    if (len <= 0)
    {
        s->SetLogin(NULL);
        if (itr != m_loginSockets.end())
        {
            m_pfRlsPack(itr->second);
            m_loginSockets.erase(itr);
        }
        return 0;
    }

    if (auto pack = m_pfParsePack(buf, len))
    {
        if (itr != m_loginSockets.end())
            m_pfRlsPack(itr->second);
        m_loginSockets[s] = pack;
    }

    return len;
}

IObjectManager *IObjectManager::MangerOfType(int type)
{
    return ObjectManagers::Instance().GetManagerByType(type);
}

void IObjectManager::PrcsSubcribes()
{
    SubcribeStruct *sub = NULL;
    Lock l(m_mtxMsgs);
    while (Utility::Pop(m_subcribeQue, sub))
    {
        if (sub->m_bSubcribe)
        {
            StringList &ls = m_subcribes[sub->m_sender][sub->m_msgType];
            if (!Utility::IsContainsInList(ls, sub->m_subcribeId))
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

void IObjectManager::SubcribesProcess(IMessage *msg)
{
    const StringList &sbs = getMessageSubcribes(msg);
    for (const string id : sbs)
    {
        if (IObject *obj = GetObjectByID(id))
            obj->ProcessMessage(msg);
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
    if (!m_mtxMsgs || dsub.empty() || sender.empty() || tpMsg < 0)
        return;

    if (SubcribeStruct *sub = new SubcribeStruct(dsub, sender, tpMsg, true))
    {
        Lock l(m_mtxMsgs);
        m_subcribeQue.push(sub);
    }
}

void IObjectManager::Unsubcribe(const string &dsub, const std::string &sender, int tpMsg)
{
    if (!m_mtxMsgs || dsub.empty() || sender.empty() || tpMsg < 0)
        return;

    if (SubcribeStruct *sub = new SubcribeStruct(dsub, sender, tpMsg, false))
    {
        Lock l(m_mtxMsgs);
        m_subcribeQue.push(sub);
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

        int tmp = t->LinkCount();
        if (minLink == -1 || tmp < minLink)
        {
            minLink = tmp;
            ret = t;
        }     
    }

    return ret;
}

BussinessThread *IObjectManager::CurThread() const
{
    auto id = Utility::ThreadID();
    for (auto itr: m_lsThread)
    {
        if (itr->GetThreadId() == id)
            return itr;
    }
    return NULL;
}
