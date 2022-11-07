#include "BussinessThread.h"
#include "ObjectBase.h"
#include "socketBase.h"
#include "Thread.h"
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

enum RcvDataStat {
    Stat_Wait,
    Stat_Used,
    Stat_Release,
};

typedef struct _ProcessSend {
    unsigned            threadId;
    std::string         linkID;
    void                *data;
} ProcessSend;
////////////////////////////////////////////////////////////////////////////////////////
//BussinessThread
////////////////////////////////////////////////////////////////////////////////////////
BussinessThread::BussinessThread(IObjectManager &mgr) :Thread(true), m_mgr(mgr)
, m_mtx(NULL), m_mtxQue(new std::mutex), m_szBuff(0), m_buff(NULL)
{
}

BussinessThread::~BussinessThread()
{
    delete m_mtx;
    delete m_buff;
}

void BussinessThread::InitialBuff(uint16_t sz)
{
    if (sz > m_szBuff)
    {
        m_buff = new char[sz];
        if (m_buff)
            m_szBuff = sz;
    }
}

std::mutex * BussinessThread::GetMutex()
{
    if (!m_mtx)
        m_mtx = new std::mutex;

    return m_mtx;
}

int BussinessThread::LinkCount() const
{
    Lock ll(m_mtxQue);
    return m_linksAdd.size() + m_links.size();
}

bool BussinessThread::PushRelaseMsg(IMessage *ms)
{
    Lock l(m_mtxQue);
    m_lsMsgRelease.push(ms);
    return true;
}

char * BussinessThread::AllocBuff() const
{
    return m_buff;
}

int BussinessThread::m_BuffSize() const
{
    return m_szBuff;
}

bool BussinessThread::Send2Link(const std::string &id, void *data)
{
    if (!data || id.empty())
        return false;

    auto dataSnd = new ProcessSend;
    dataSnd->threadId = GetThreadId();
    dataSnd->data = data;
    dataSnd->linkID = id;
    PushProcessSnd(dataSnd);
    return true;
}

bool BussinessThread::AddOneLink(ILink *l)
{
    Lock ll(m_mtxQue);
    m_linksAdd.push(l);
    return true;
}

IObjectManager *BussinessThread::GetManager()
{
    return &m_mgr;
}

void BussinessThread::OnRcvPacket(const std::string &idLink)
{
    Lock ll(m_mtxQue);
    m_rcvQue.push(idLink);
}

string BussinessThread::PopRcv()
{
    string data;
    Lock ll(m_mtxQue);
    Utility::Pop(m_rcvQue, data);
    return data;
}

void BussinessThread::PushProcessSnd(ProcessSend *val, bool bSnding)
{
    Lock ll(m_mtxQue);
    if (bSnding)
        m_sendingDatas.push(val);
    else
        m_sendedDatas.push(val);
}

void BussinessThread::ReleaseSndData(ProcessSend *v)
{
    m_mgr.m_pfRlsPack(v->data);
    delete v;
}

bool BussinessThread::CheckLinks()
{
    bool ret = false;
    uint64_t ms = Utility::msTimeTick();
    for (auto itr = m_links.begin(); itr != m_links.end(); )
    {
        ILink *l = itr->second;
        l->CheckTimer(ms);
        if (l->IsRealse())
        {
            l->SetThread(NULL);
            itr = m_links.erase(itr);
            auto obj = l->GetParObject();
            if (obj && obj->IsAllowRelease())
                obj->SendMsg(new ObjectSignal(obj, obj->GetObjectType()));
            ret = true;
        }
        else
        {
            ++itr;
        }
    }
    return ret;
}

void BussinessThread::ProcessAddLinks()
{
    while (ILink *l = PopQue(m_linksAdd))
    {
        if (!l || l->GetThread())
            continue;

        l->SetThread(this);
        IObject *o = l ? l->GetParObject() : NULL;
        auto id = o ? o->GetObjectID() : string();
        if (!id.empty() && m_links.find(id) == m_links.end())
            m_links[id] = l;

        l->SetMutex(GetMutex());
    }
}

void BussinessThread::PrcsSend()
{
    while (auto dt = PopQue(m_sendingDatas))
    {
        auto t = m_mgr.GetThread(dt->threadId);
        if (!t)
        {
            delete dt;
            continue;
        }
        if (auto link = GetLinkByName(dt->linkID))
        {
            int tmp = link->GetSendRemain();
            if (tmp > m_szBuff)
                tmp = m_szBuff;

            tmp = m_mgr.m_pfSrlzPack(dt->data, m_buff, tmp);
            if (tmp <= 0)
            {
                t->PushProcessSnd(dt);
                continue;
            }

            link->SendData(m_buff, tmp);
            if (this != t)
                t->PushProcessSnd(dt, false);
            else
                ReleaseSndData(dt);
        }
        else
        {
            if (this == t)
                ReleaseSndData(dt);
            else
                t->PushProcessSnd(dt, false);
        }
    }
}

void BussinessThread::PrcsRcvPacks()
{
    string id;
    while (!(id = PopRcv()).empty())
    {
        if (auto link = GetLinkByName(id))
            link->ProcessRcvPacks();
    }
}

void BussinessThread::ReleasePrcsdMsgs()
{
    IMessage *tmp = NULL;
    Lock l(m_mtxQue);
    while (Utility::Pop(m_lsMsgRelease, tmp))
    {
        delete tmp;
    }
}

void BussinessThread::PrcsSended()
{
    while (auto *data = PopQue(m_sendedDatas))
    {
        ReleaseSndData(data);
    }
}

ILink *BussinessThread::GetLinkByName(const string &id)
{
    auto itr = m_links.find(id);
    if (itr != m_links.end())
        return itr->second;

    return NULL;
}

bool BussinessThread::RunLoop()
{
    ReleasePrcsdMsgs();
    bool ret = m_mgr.ProcessBussiness(this);
    if (m_mgr.IsReceiveData())
    {
        ProcessAddLinks();
        PrcsSend();
        PrcsSended();
        PrcsRcvPacks();
        if (CheckLinks())
            return true;
    }
    return ret;
}
