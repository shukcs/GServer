#include "GLoop.h"

#include <mutex>
#include <thread>
#include <chrono>
#include "Utility.h"
#include "Varient.h"
#include "Lock.h"

enum LoopStat {
    St_Stop,
    St_Loop,
    St_Exit,
};
#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif
using namespace std;
////////////////////////////////////////////////////////////////////////////////////////////
///ThreadManager
////////////////////////////////////////////////////////////////////////////////////////////
class LoopManager
{
public:
    static LoopManager &Instance()
    {
        static LoopManager s_mgr;
        return s_mgr;
    }
    static GLoop *GetLooopByThreadId(int id)
    {
        return Instance()._findThread(id);
    }
    void AddLoop(GLoop *lp)
    {
        if (lp)
        {
            Lock l(&m_mutex);
            m_threads[lp->GetThreadId()] = lp;
        }
    }
    void RemoveLoop(GLoop *t)
    {
        if (t)
        {
            Lock l(&m_mutex);
            m_threads.erase(t->GetThreadId());
        }
    }
protected:
    LoopManager() {}
    GLoop *_findThread(int id)
    {
        Lock l(&m_mutex);
        auto itr = m_threads.find(id);
        if (itr != m_threads.end())
            return itr->second;

        return NULL;
    }
private:
    std::mutex              m_mutex;
    std::map<int, GLoop*>  m_threads;
};
///////////////////////////////////////////////////////////////////////////////////////////////
//Timer
///////////////////////////////////////////////////////////////////////////////////////////////
class Timer
{
public:
    Timer(const Variant &v, uint32_t ms, bool repeat = true) : m_usStart(Utility::usTimeTick())
	, m_id(v), m_threadID(Utility::ThreadID()), m_usSpace(ms*1000), m_count(1), m_offSet(0)
    , m_repeat(repeat), m_release(false)
    {
		m_usStart = Utility::usTimeTick();
        if (m_usSpace == 0)
			m_usSpace = 1000;
    }
	~Timer()
	{
	}
    int GetThreadID()const
    {
        return m_threadID;
    }
    void Kill()
    {
        m_release = true;
    }
    bool IsKilled()const
    {
        return m_release;
    }
    bool IsRepeat()const
    {
        return m_repeat;
    }
    void SetOffsetTime(uint32_t us)
    {
        m_offSet = us;
    }
    int64_t TriggerTime()const
    {
        return m_usStart+ m_usSpace*m_count+m_offSet;
    }
    void Trigger(int64_t curUs)
    {
		m_count = uint32_t((curUs - m_usStart) / m_usSpace + 1);
		m_offSet = 0;
    }
    const Variant &GetIdentify()const
    {
        return m_id;
    }
private:
    int64_t     m_usStart;
    Variant     m_id;
    int         m_threadID;
	uint32_t    m_usSpace;
	uint32_t    m_count;
	uint32_t    m_offSet;
	bool        m_release;
	bool        m_repeat;
};
//////////////////////////////////////////////////////////////////////////////////////
///GLoop
//////////////////////////////////////////////////////////////////////////////////////
GLoop::GLoop(unsigned us) : m_loopStat(St_Stop), m_usSleep(us)
, m_enableTimer(false), m_mtxQue(new mutex), m_idThread(-1)
{
}

GLoop::~GLoop()
{
    Exit();
    while (St_Stop != m_loopStat)
    {
        this_thread::sleep_for(chrono::milliseconds(m_usSleep));
    }
}

unsigned GLoop::AddLoopHandle(const LoopHandle &h)
{
    if (m_loopStat == St_Loop)
        return 0;

    auto itr = m_handles.end();
    int ret = itr == m_handles.begin() ? 1 : (--itr)->first + 1;
    m_handles[ret] = h;
    return ret;
}

bool GLoop::RemoveHandle(unsigned s)
{
    if (m_loopStat != St_Loop)
        return false;

    auto itr = m_handles.find(s);
    if (itr != m_handles.end())
    {
        m_handles.erase(s);
        return true;
    }
    return false;
}

int GLoop::GetThreadId()const
{
    return m_idThread;
}

bool GLoop::IsRun()const
{
 	return St_Stop != m_loopStat;
}

void GLoop::Run()
{
    if (St_Stop != m_loopStat)
        return;

    m_loopStat = St_Loop;

    m_idThread = Utility::ThreadID();
    LoopManager::Instance().AddLoop(this);
    bool bSleep = true;
    while (St_Loop == m_loopStat)
    {
        bSleep = !_prcsTimer();
        for (auto itr = m_handles.begin(); itr != m_handles.end(); ++itr)
        {
            if (itr->second())
                bSleep = false;
        }

        if (bSleep)
            this_thread::sleep_for(chrono::milliseconds(m_usSleep));
    }
    m_loopStat = St_Stop;
}


void GLoop::Exit()
{
    if (St_Loop == m_loopStat)
        m_loopStat = St_Exit;
}

void GLoop::SetEnableTimer(bool b)
{
    m_enableTimer = b;
}


bool GLoop::IsEnableTimer() const
{
    return m_enableTimer;
}

void GLoop::SetTimerHandle(const std::function<bool(const Variant &, int64_t)> &hd)
{
    if (!IsRun())
        m_timerHd = hd;
}

Timer *GLoop::AddTimer(const Variant &id, uint32_t tmRepeat, bool repeat)
{
    if (IsEnableTimer() && m_timerHd && id.IsValid())
    {
        if (auto tm = new Timer(id, tmRepeat, repeat))
        {
            _pushTimer(tm);
            return tm;
        }
    }
    return NULL;
}

void GLoop::KillTimer(Timer *&timer)
{
    timer->Kill();
    timer = NULL;
}

bool GLoop::_prcsTimer()
{
    _prcsTimerQue();
    return _prcsTrigger();
}

bool GLoop::_prcsTrigger()
{
    if (!IsEnableTimer() || !m_timerHd)
        return false;

    auto us = Utility::usTimeTick();
    bool ret = false;
    for (auto itr = m_timersTrigger.begin(); itr!=m_timersTrigger.end(); )
    {
        if (us < itr->first)
            break;
        auto tm = itr->second;
        itr = m_timersTrigger.erase(itr);
        ret = true;
        if (tm->IsKilled())
        {
            _toRelease(tm);
            continue;
        }
		tm->Trigger(us);
        if (m_timerHd(tm->GetIdentify(), us/1000) && tm->IsRepeat())
            _addTrigger(tm);
        else
            _toRelease(tm);
    }
    return ret;
}

void GLoop::_prcsTimerQue()
{
    while (auto tm = _popAddTimer())
    {
        if (tm->IsKilled())
            _toRelease(tm);
        else
            _addTrigger(tm);
    }
}

Timer *GLoop::_popAddTimer()
{
    Timer *ret = NULL;
    Lock ll(m_mtxQue);
    Utility::PullQue(m_timersAdd, ret);
    return ret;
}

void GLoop::_addTrigger(Timer *t)
{
    if (!t)
        return;

    int64_t tm = t->TriggerTime();
    auto itr = m_timersTrigger.upper_bound(t->TriggerTime());
    while (itr!=m_timersTrigger.end() && itr->first==tm)
    {
        tm++;
        itr++;
    }
    t->SetOffsetTime(uint32_t(tm - t->TriggerTime()));
    m_timersTrigger[tm] = t;
}

void GLoop::_pushTimer(Timer *t)
{
    Lock ll(m_mtxQue);
    m_timersAdd.push(t);
}

void GLoop::_toRelease(Timer *tm)
{
    if (!tm)
        return;
    auto t = LoopManager::GetLooopByThreadId(tm->GetThreadID());
    if (!t || this ==t)
    {
        delete tm;
        return;
    }
    tm->Kill();
    t->_pushTimer(tm);
}