#ifndef _G_LOOP_H
#define _G_LOOP_H

#include <queue>
#include <map>
#include <functional>

namespace std {
    class mutex;
}

class Timer;
class Variant;
class Thread;
class GLoop
{
    typedef std::queue<Timer *> TimerQue;
    typedef std::map<int64_t, Timer *> TimerMap;
public:
    typedef std::function<bool()> LoopHandle;
    typedef std::function<bool(const Variant &, int64_t)> TimerHandle;///当返回fasle 定时器将释放
public:
    GLoop(unsigned us=1000); //ms: sleep time(unit: ms)while nothing to do in loop
    virtual ~GLoop();

    unsigned AddLoopHandle(const LoopHandle &h);///ret=0, add fail
    bool RemoveHandle(unsigned s); ///s: use AddLoopHandle return value
    bool IsRun()const;  ///是否在循环
    void Run();///start Loop;
    void Exit();
    void SetEnableTimer(bool b);
    bool IsEnableTimer()const;
    void SetTimerHandle(const std::function<bool(const Variant &, int64_t)> &hd);

    Timer *AddTimer(const Variant &id, uint32_t tmRepeat, bool repeat);
    void KillTimer(Timer *&timer);
    int GetThreadId()const;
private:
    bool _prcsTimer();
    bool _prcsTrigger();
    void _prcsTimerQue();
    void _pushTimer(Timer *t);
    Timer *_popAddTimer();
    void _addTrigger(Timer *t);
    void _toRelease(Timer *tm);
private:
    int     m_loopStat;
    unsigned m_usSleep;
    std::mutex *m_mtxQue;
    bool m_enableTimer;
    int m_idThread;

    TimerQue    m_timersAdd;
    TimerHandle m_timerHd;
    std::map<unsigned, LoopHandle> m_handles; ///所有线程循环处理函数
    TimerMap    m_timersTrigger; ///所有定时器
};

#ifdef SOCKETS_NAMESPACE
}
#endif

#endif // _SOCKETS_Thread_H

