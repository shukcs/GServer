#include "BussinessPrcs.h"
#include "ObjectBase.h"
#include "net/socketBase.h"
#include "common/Thread.h"
#include "ObjectManagers.h"
#include "common/Utility.h"
#include "common/Lock.h"
#include "IMessage.h"
#include "ILog.h"
#include "common/Varient.h"
#include <stdarg.h>
#include "common/Thread.h"
#include "common/GLoop.h"

using namespace std;
#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif

enum RcvDataStat {
    Stat_Wait,
    Stat_Used,
    Stat_Release,
};

class SendStruct
{
public:
    int             threadId;
    void            *data;
    bool            bSending;
    std::string     linkID;
public:
    SendStruct(int t, void *dt, const std::string &id)
    : threadId(t), data(dt), bSending(true), linkID(id)
    {}
};
using namespace std;
////////////////////////////////////////////////////////////////////////////////////////
//BussinessPrcs
////////////////////////////////////////////////////////////////////////////////////////
BussinessPrcs::BussinessPrcs(IObjectManager &mgr, bool bEnableTimer) : m_tmCheckLink(0)
, m_mgr(mgr), m_mtx(NULL), m_mtxQue(new std::mutex), m_szBuff(0), m_buff(NULL)
, m_thread(NULL), m_bEnableTimer(bEnableTimer)
{
}

BussinessPrcs::~BussinessPrcs()
{
    delete m_mtx;
    delete m_buff;
    delete m_thread;
}


void BussinessPrcs::SetRunning(bool b /*= true*/)
{
    if (!m_thread && b)
        _bindhread(new Thread());

    if (m_thread)
        m_thread->SetRunning(b);
}

void BussinessPrcs::InitialBuff(uint16_t sz)
{
    if (sz > m_szBuff)
    {
        m_buff = new char[sz];
        if (m_buff)
            m_szBuff = sz;
    }
}

std::mutex * BussinessPrcs::GetMutex()
{
    if (!m_mtx)
        m_mtx = new std::mutex;

    return m_mtx;
}

int BussinessPrcs::LinkCount() const
{
    Lock ll(m_mtxQue);
    return m_linksAdd.size() + m_links.size();
}

bool BussinessPrcs::PushRelaseMsg(IMessage *ms)
{
    Lock l(m_mtxQue);
    m_msgRelease.push(ms);
    return true;
}

char * BussinessPrcs::AllocBuff() const
{
    return m_buff;
}

int BussinessPrcs::m_BuffSize() const
{
    return m_szBuff;
}

bool BussinessPrcs::Send2Link(const std::string &id, void *data)
{
    if (!data || id.empty())
        return false;

    PushSnd(new SendStruct(Utility::ThreadID(), data, id));
    return true;
}

bool BussinessPrcs::AddOneLink(ILink *l)
{
    Lock ll(m_mtxQue);
    m_linksAdd.push(l);
    return true;
}

IObjectManager *BussinessPrcs::GetManager()
{
    return &m_mgr;
}

void BussinessPrcs::AddWaiPrcs(const std::string &idLink)
{
    Lock ll(m_mtxQue);
    m_queAddWaiPrcs.push(idLink);
}

int BussinessPrcs::GetThreadId() const
{
    return m_thread ? m_thread->GetThreadId() : -1;
}

void BussinessPrcs::SetEnableTimer(bool b)
{
    if (m_thread)
        m_thread->SetEnableTimer(b);
}

bool BussinessPrcs::IsEnableTimer() const
{
    return m_thread && m_thread->IsEnableTimer();
}

Timer * BussinessPrcs::AddTimer(const Variant &id, uint32_t tmTriggle, bool repeat)
{
    if (m_thread)
        return m_thread->AddTimer(id, tmTriggle, repeat);

    return NULL;
}

void BussinessPrcs::KillTimer(Timer *&timer)
{
    if (m_thread)
        m_thread->KillTimer(timer);
}

string BussinessPrcs::PopWaiPrcs()
{
    string data;
    Lock ll(m_mtxQue);
    Utility::PullQue(m_queAddWaiPrcs, data);
    return data;
}

void BussinessPrcs::PushSnd(SendStruct *val, bool bSnding)
{
    Lock ll(m_mtxQue);
    if (bSnding)
        m_sendingDatas.push(val);
    else
        m_sendedDatas.push(val);
}

void BussinessPrcs::ReleaseSndData(SendStruct *v)
{
    m_mgr.m_pfRlsPack(v->data);
    delete v;
}

void BussinessPrcs::ProcessAddLinks()
{
    while (ILink *l = PopQue(m_linksAdd))
    {
        if (!l || l->GetThread())
            continue;

        l->SetThread(this);
        auto  o = l ? l->GetParObject() : NULL;
        auto id = o ? o->GetObjectID() : string();
        if (!id.empty() && m_links.find(id) == m_links.end())
            m_links[id] = l;

        l->OnConnected(true);
        l->SetMutex(GetMutex());
    }
}

bool BussinessPrcs::_prcsSnd(SendStruct *v, ILink *l)
{
    if (!v || !l)
        return false;

    auto link = GetLinkByName(v->linkID);
    if (!link || !link->IsLinked())
    {
        if (!v->bSending)
            return true;
        v->bSending = false;
    }

    int tmp = link->GetSendRemain();
    if (tmp > m_szBuff)
        tmp = m_szBuff;
    tmp = m_mgr.m_pfSrlzPack(v->data, m_buff, tmp);
    if (tmp > 0)
    {
        link->SendData(m_buff, tmp);
        return true;
    }
    return false;
}

void BussinessPrcs::PrcsSend()
{
	SendStruct *lastUnsnd = NULL;
    while (auto dt = PopQue(m_sendingDatas))
    {
        auto t = m_mgr.GetThread(dt->threadId);
        if (!t)
        {
            delete dt;
            continue;
		}

		if (dt && lastUnsnd==dt) ///防止死循环
		{
			t->PushSnd(dt, true);
			break;
		}
		auto suc = _prcsSnd(dt, GetLinkByName(dt->linkID));
		if (!suc && NULL==lastUnsnd)
			lastUnsnd = dt;
		t->PushSnd(dt, !suc);
    }
}

bool BussinessPrcs::PrcsWaitBsns()
{
    string id;
    bool ret = false;
    while (!(id = PopWaiPrcs()).empty())
    {
        auto itr = m_links.find(id);
        if (itr == m_links.end())
            continue;

        auto link = itr->second;
        if (!link->IsLinked())
		{
			link->_disconnect();
			link->OnConnected(false);
			link->SetThread(NULL);
			m_links.erase(itr);
        }
        else
        {
            itr->second->ProcessRcvPacks(Utility::msTimeTick());
            ret = true;
        }
    }
    return ret;
}

void BussinessPrcs::ReleasePrcsdMsgs()
{
    IMessage *tmp = NULL;
    Lock l(m_mtxQue);
    while (Utility::PullQue(m_msgRelease, tmp))
    {
        delete tmp;
    }
}

void BussinessPrcs::PrcsSended()
{
    while (auto *data = PopQue(m_sendedDatas))
    {
        ReleaseSndData(data);
    }
}

ILink *BussinessPrcs::GetLinkByName(const string &id)
{
    auto itr = m_links.find(id);
    if (itr != m_links.end())
        return itr->second;

    return NULL;
}

bool BussinessPrcs::RunLoop()
{
    ReleasePrcsdMsgs();
    bool ret = false;
    if (m_mgr.m_thread == this)
        ret = m_mgr.ProcessBussiness();

    if (m_mgr.IsReceiveData())
    {
        ProcessAddLinks();
        PrcsSend();
        PrcsSended();
        if (PrcsWaitBsns())
            ret = true;
    }
    return ret;
}

bool BussinessPrcs::OnTimerTriggle(const Variant &v, int64_t ms)
{
    return m_mgr.OnTimerTrigger(v.ToString(), ms);
}

void BussinessPrcs::_bindhread(Thread *t)
{
    if (t && !m_thread)
    {
        m_thread = t;
        if (m_bEnableTimer)
        {
            auto f = std::bind(&BussinessPrcs::OnTimerTriggle, this, std::placeholders::_1, std::placeholders::_2);
            m_thread->GetLoop()->SetTimerHandle(f);
        }
        t->AddHandle(&BussinessPrcs::RunLoop, this);
        return;
    }

    delete t;
}
