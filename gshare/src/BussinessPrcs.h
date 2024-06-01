#ifndef __cpp_Bussiness_Prcs_H__
#define __cpp_Bussiness_H__

#include <map>
#include <queue>
#include "common/Lock.h"
#include "common/Utility.h"

using namespace std;
#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif

class IMessage;
class IObjectManager;
class ILink;
class SendStruct;
class Thread;
class Variant;
class Timer;

class BussinessPrcs 
{
private:
    typedef std::queue<IMessage*> MessageQue;
    typedef std::map<std::string, ILink*> MapLinks;
    typedef std::queue<ILink*> LinksQue;
    typedef std::queue<SendStruct *>ProcessSendQue;
    typedef std::queue<std::string> WaitPrcsQue;
public:
    BussinessPrcs(IObjectManager &mgr, bool bEnableTimer=false);
    ~BussinessPrcs();
    void SetRunning(bool b = true);
    void InitialBuff(uint16_t sz);
    std::mutex *GetMutex();
    int LinkCount()const;
    bool PushRelaseMsg(IMessage *ms);
    char *AllocBuff()const;
    int m_BuffSize()const;
    bool Send2Link(const std::string &id, void *data);
    bool AddOneLink(ILink *l);
    IObjectManager *GetManager();
    void AddWaiPrcs(const std::string &idLink);
    int GetThreadId()const;
    void SetEnableTimer(bool b);
    bool IsEnableTimer()const;
    Timer *AddTimer(const Variant &id, uint32_t tmRepeat, bool repeat);
    void KillTimer(Timer *&timer);
    template<class T>
    T PopQue(queue<T> &que)
    {
        T data = NULL;
        Lock ll(m_mtxQue);
        if (Utility::PullQue(que, data))
            return data;
        return data;
    }
protected:
    bool RunLoop();
    bool OnTimerTriggle(const Variant &v, int64_t ms);
protected:
    void ProcessAddLinks();
    void PrcsSend();
    void PrcsSended();
    bool PrcsWaitBsns();
    void PushSnd(SendStruct *val, bool bSnding = true);
    void ReleaseSndData(SendStruct *v);
private:
    std::string PopWaiPrcs();
    void ReleasePrcsdMsgs();
    ILink *GetLinkByName(const string &id);
private:
    bool _prcsSnd(SendStruct *v, ILink *l);
    void _bindhread(Thread *t);
private:
    int64_t         m_tmCheckLink;
    IObjectManager  &m_mgr;
    std::mutex      *m_mtx;
    std::mutex      *m_mtxQue;
    int             m_szBuff;
    char            *m_buff;
    Thread          *m_thread;
    bool            m_bEnableTimer;
    MapLinks        m_links;
    ProcessSendQue  m_sendingDatas;     ///发送数据队列
    ProcessSendQue  m_sendedDatas;      ///发送完的数据队列
    WaitPrcsQue     m_queAddWaiPrcs;    ///等待要处理的包队列
private:
    LinksQue        m_linksAdd;         ///添加新网络连接
    MessageQue      m_msgRelease;       ///消息释放等待队列

};

#ifdef SOCKETS_NAMESPACE
}
#endif
#endif //__cpp_BussinessPrcs_H__