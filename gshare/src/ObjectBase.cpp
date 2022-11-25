#include "ObjectBase.h"
#include "socketBase.h"
#include "Thread.h"
#include "ObjectManagers.h"
#include "BussinessThread.h"
#include "Utility.h"
#include "Lock.h"
#include "IMessage.h"
#include "ILog.h"
#include "Varient.h"
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
ILink::ILink() : m_sock(NULL), m_recv(NULL), m_szRcv(0), m_pos(0), m_mtxS(NULL)
, m_stat(Stat_Link), m_thread(NULL), m_bLogined(false)
{
}

ILink::~ILink()
{
    CloseLink();
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

int ILink::GetSendRemain() const
{
    Lock l(m_mtxS);
    if (m_sock && m_sock->IsNoWriteData())
        return m_sock->GetSendRemain();

    return -1;
}

void ILink::OnSockClose(ISocket *s)
{
	s->SetHandleLink(NULL);
    if (m_sock != s)
        return;

	_cleanRcvPacks(true);
	_clearSocket();
    if (m_thread)
        m_thread->AddWaiPrcs(GetParObject()->GetObjectID());
}

void ILink::SetSocket(ISocket *s)
{
    Lock l(m_mtxS);
    if (s && !m_sock)
	{
		m_stat = Stat_Link;
        m_sock = s;
        s->SetHandleLink(this);
        OnConnected(true);
    }
}

bool ILink::IsRemoveThread() const
{
	return m_stat == ILink::Stat_Remove;
}

bool ILink::IsLinked() const
{
    return m_stat == Stat_Link;
}

void ILink::SendData(char *buf, int len)
{
    Lock l(m_mtxS);
    if (buf && len>0 && m_sock)
        m_sock->Send(len, buf);
}

int ILink::ParsePack(const char *buf, int len)
{
    auto pf = GetParsePackFunc();
    if (!pf)
        return len;

    int used = 0;
    uint32_t tmp = len;
    while (tmp > 0)
    {
        auto pack = pf(buf+used, tmp);
        used += tmp;
        tmp = len - used;
        if (!pack)
            break;

        AddProcessRcv(pack, false);
        if (m_thread)
            m_thread->AddWaiPrcs(GetParObject()->GetObjectID());
    }
    return used;
}

ParsePackFuc ILink::GetParsePackFunc()const
{
    auto mgr = m_thread ? m_thread->GetManager() : NULL;
    if (mgr)
        return mgr->m_pfParsePack;
    return NULL;
}

RelaesePackFuc ILink::GetReleasePackFunc() const
{
    auto mgr = m_thread ? m_thread->GetManager() : NULL;
    if (mgr)
        return mgr->m_pfRlsPack;
    return NULL;
}

SerierlizePackFuc ILink::GetSerierlizePackFuc() const
{
    auto mgr = GetParObject()->GetManager();
    if (mgr)
        return mgr->m_pfSrlzPack;

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

void ILink::ProcessRcvPacks(int64_t ms)
{
    while (auto pack = PopRcv(false))
    {
        if (ProcessRcvPack(pack))
            RefreshRcv(ms);

        AddProcessRcv(pack);
    }
}

void *ILink::PopRcv(bool bUsed)
{
    void *ret = NULL;
    Lock l(m_mtxS);
    if (bUsed)
        Utility::PullQue(m_packetsUsed, ret);
    else
        Utility::PullQue(m_packetsRcv, ret);

    return ret;
}

void ILink::_disconnect()
{
	if (!m_bLogined)
		return;

	m_bLogined = false;
	auto obj = GetParObject();
	obj->GetManager()->Log(0, IObjectManager::GetObjectFlagID(obj)
		, 0, "(User: %s) disconnect!", obj->GetObjectID().c_str());
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
        m_sock->Close(false);
}

bool ILink::OnReceive(const ISocket &s, const void *buf, int len)
{
	_cleanRcvPacks();
    if (&s != m_sock)
        return false;

    if (!_adjustBuff((const char*)buf, len))
        return false;

    int tmp = ParsePack(m_recv, m_pos);
    if (tmp>0 && tmp<m_pos)
        memcpy(m_recv, m_recv+tmp, m_pos-tmp);
    m_pos -= (tmp<m_pos) ? tmp : m_pos;
    return true;
}

void ILink::SetMutex(std::mutex *m)
{
    m_mtxS = m;
}

void ILink::SetThread(BussinessThread *t)
{
	if (NULL == t)
		m_stat = ILink::Stat_Remove;

    m_thread = t;
    if (m_thread == NULL)
        m_mtxS = NULL;
}

BussinessThread *ILink::GetThread() const
{
    return m_thread;
}

void ILink::processSocket(ISocket &s, BussinessThread &t)
{
	if (!m_thread)
		t.AddOneLink(this);

	SetSocket(&s);
	RefreshRcv(Utility::msTimeTick());
}

bool ILink::IsConnect() const
{
	return m_bLogined;
}

void ILink::SetLogined(bool suc, ISocket *s)
{
	if (!s)
		s = GetSocket();

	if (s)
	{
		m_bLogined = suc;
		auto obj = GetParObject();
		obj->GetManager()->Log(0, IObjectManager::GetObjectFlagID(obj), 0, "[%s:%d](User: %s) %s!"
			, s->GetHost().c_str(), s->GetPort(), obj->GetObjectID().c_str(), suc ? "logined" : "login fail");

		if (!suc)
			CloseLink();
	}
}

bool ILink::SendData2Link(void *data, ISocket *s)
{
    if (s)
    {
        char buf[512];
        auto pf = GetSerierlizePackFuc();
        auto len = pf(data, buf, sizeof(buf));
		if (len > 0)
		{
			s->SetHandleLink(this);
			s->Send(len, buf);
		}

        return len > 0;
    }

    if (!m_thread || !IsLinked())
        return false;

    return m_thread->Send2Link(GetParObject()->GetObjectID(), data);
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
    auto pf = GetReleasePackFunc();
    while (auto pack = PopRcv(false))
    {
        pf(pack);
    }
	if (!bUnuse)
		return;

	while (auto pack = PopRcv())
	{
		pf(pack);
	}
}

void ILink::_clearSocket()
{
	Lock l(m_mtxS);
	m_stat = Stat_Close;
	m_sock = NULL;
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

void IObject::CheckTimer(uint64_t)
{
}

Timer *IObject::StartTimer(uint32_t ms, bool repeat)
{
    if (auto t = GetManager()->GetMainThread())
        return t->AddTimer(GetObjectID(), ms, repeat);

    return NULL;
}

void IObject::KillTimer(Timer *timer)
{
    GetManager()->GetMainThread()->KillTimer(timer);
}

void IObject::ReleaseObject()
{
	if (auto lk = GetLink())
		lk->CloseLink();

	if (!IsAllowRelease())
		return;

	m_stInit = ReleaseLater;
	GetManager()->ObjectRelease(*this);
}

bool IObject::IsRelease() const
{
    return ReleaseLater == m_stInit;
}

void IObject::InitObject()
{
	m_stInit = Initialed;
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
    return m_stInit > Uninitial;
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
//////////////////////////////////////////////////////////////////////////////////////////
//IObjectManager
//////////////////////////////////////////////////////////////////////////////////////////
IObjectManager::IObjectManager() : m_thread(NULL), m_log(NULL), m_mtxLogin(new std::mutex)
, m_mtxMsgs(new std::mutex), m_pfSrlzPack(NULL), m_pfParsePack(NULL), m_pfRlsPack(NULL)
, m_pfPrcsLogin(NULL)
{
}

IObjectManager::~IObjectManager()
{
    delete m_thread;
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

void IObjectManager::ObjectRelease(const IObject &obj)
{
	if (NULL == GetThread(Utility::ThreadID()))
		return;

	if (auto evt = new ObjectSignal(&obj, GetObjectType()))
		PushManagerMessage(evt);
}

bool IObjectManager::PrcsPublicMsg(const IMessage &)
{
    return false;
}

void IObjectManager::ToCurrntLog(int err, const std::string &obj, int evT, const std::string &dscb)
{
    ObjectManagers::GetLog().Log(dscb, obj, evT, err);
}

bool IObjectManager::ProcessBussiness()
{
    PrcsSubcribes();
    ProcessMessage();
    ProcessEvents();

    if (IsReceiveData())
        return ProcessLogins();

    return false;
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


void IObjectManager::InitPrcsLogin(PrcsLoginHandle hd)
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
	const IMessage *lastUnprcs = NULL;
    while (auto msg =_popMessage())
    {
        if (!msg)
            continue;
		if (lastUnprcs == msg)///防止死循环
		{
			PushManagerMessage(msg);
			break;
		}
		if (msg->GetMessgeType() == ObjectSignal::S_Release)
		{
			if (!_prcsRelease(*(ObjectSignal*)msg))
			{
				PushManagerMessage(msg);
				if (NULL == lastUnprcs)
					lastUnprcs = msg;
				continue;
			}
		}
		else if (msg->IsValid())
		{
			_prcsMessage(*msg);
		}

		if (!ObjectManagers::RlsMessage(msg))
            delete msg;
    }
}

bool IObjectManager::ProcessLogins()
{
    bool ret = false;
    if (NULL == m_pfPrcsLogin)
        return ret;

    Lock l(m_mtxLogin);
    for (auto itr = m_loginSockets.begin(); itr!= m_loginSockets.end(); ++itr)
    {
        ISocket *s = itr->first;
        if (s->GetHandleLink())
            continue;

        IObject *o = (this->*m_pfPrcsLogin)(s, itr->second);
        if (o)
        {
			auto link = o->GetLink();
            link->processSocket(*s, *GetPropertyThread());
            AddObject(o);
			if (!o->IsInitaled())
				o->InitObject();
            ret = true;
        }
    }
    return ret;
}

bool IObjectManager::OnTimerTrigger(const std::string &id, int64_t ms)
{
    auto itr = m_objects.find(id);
    if (itr == m_objects.end())
        return false;

    itr->second->CheckTimer(ms);
    return true;
}

IMessage *IObjectManager::_popMessage()
{
	IMessage *msg = NULL;
	Lock l(m_mtxMsgs);
	Utility::PullQue(m_messages, msg);
	return msg;
}

bool IObjectManager::_prcsRelease(const ObjectSignal &evt)
{
	auto itr = m_objects.find(evt.GetSenderID());
	if (itr != m_objects.end())
	{
		auto link = itr->second->GetLink();
		if (!link || link->IsRemoveThread())
		{
			delete itr->second;
			m_objects.erase(itr);
			return true;
		}
		return false;
	}
	return true;
}

void IObjectManager::_prcsMessage(const IMessage &msg)
{
	SubcribesProcess(&msg);
	const string &rcv = msg.GetReceiverID();
	if (IObject *obj = GetObjectByID(rcv))
	{
		obj->ProcessMessage(&msg);
		return;
	}

	if (!PrcsPublicMsg(msg) && rcv.empty())
	{
		for (const pair<string, IObject*> &o : m_objects)
		{
			o.second->ProcessMessage(&msg);
		}
	}
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

string IObjectManager::GetObjectFlagID(const IObject *o)
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
    m_thread = new BussinessThread(*this);
    for (int i=1; i<nThread; ++i)
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
    if (m_thread && m_thread->GetThreadId())
        return m_thread;

    for (auto itr : m_lsThread)
    {
        if (itr->GetThreadId() == (uint32_t)id)
            return itr;
    }
    return NULL;
}

BussinessThread *IObjectManager::GetMainThread() const
{
    return m_thread;
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
    while (Utility::PullQue(m_subcribeQue, sub))
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

void IObjectManager::SubcribesProcess(const IMessage *msg)
{
    const StringList &sbs = getMessageSubcribes(msg);
    for (const string id : sbs)
    {
        if (IObject *obj = GetObjectByID(id))
            obj->ProcessMessage(msg);
    }
}

const StringList &IObjectManager::getMessageSubcribes(const IMessage *msg)
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

    BussinessThread *ret = m_thread;
    int minLink = -1;
    for (BussinessThread *t : m_lsThread)
    {
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
