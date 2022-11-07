#ifndef __cpp_BussinessThread_H__
#define __cpp_BussinessThread_H__

#include <map>
#include <queue>
#include "Lock.h"
#include "Thread.h"
#include "Utility.h"

using namespace std;
#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif

namespace std {
    class mutex;
}

class IMessage;
class IObjectManager;
class ILink;

typedef struct _ProcessSend ProcessSend;
class BussinessThread : public Thread
{
    typedef std::queue<IMessage*> MessageQue;
    typedef std::map<std::string, ILink*> MapLinks;
    typedef std::queue<ILink*> LinksQue;
    typedef std::queue<ProcessSend *>ProcessSendQue;
    typedef std::queue<std::string> ProcessRcvQue;
public:
    BussinessThread(IObjectManager &mgr);
    ~BussinessThread();

    void InitialBuff(uint16_t sz);
    std::mutex *GetMutex();
    int LinkCount()const;
    bool PushRelaseMsg(IMessage *ms);
    char *AllocBuff()const;
    int m_BuffSize()const;
    bool Send2Link(const std::string &id, void *data);
    bool AddOneLink(ILink *l);
    IObjectManager *GetManager();
    void OnRcvPacket(const std::string &idLink);

    template<class T>
    T PopQue(queue<T> &que)
    {
        T data = NULL;
        Lock ll(m_mtxQue);
        if (Utility::Pop(que, data))
            return data;
        return data;
    }
    std::string PopRcv();
protected:
    void PushProcessSnd(ProcessSend *val, bool bSnding = true);
    void ReleaseSndData(ProcessSend *v);
private:
    bool CheckLinks();
    void ProcessAddLinks();
    void PrcsSend();
    void PrcsRcvPacks();
    void ReleasePrcsdMsgs();
    void PrcsSended();
    ILink *GetLinkByName(const string &id);
    bool RunLoop();
private:
    IObjectManager  &m_mgr;
    std::mutex      *m_mtx;
    std::mutex      *m_mtxQue;
    int             m_szBuff;
    char            *m_buff;
    MapLinks        m_links;
    LinksQue        m_linksAdd;
    MessageQue      m_lsMsgRelease;     ///释放等待队列
    ProcessSendQue  m_sendingDatas;     ///发送数据队列
    ProcessSendQue  m_sendedDatas;      ///发送完的数据队列
    ProcessRcvQue   m_rcvQue;           ///接收解析数据队列
};

#ifdef SOCKETS_NAMESPACE
}
#endif
#endif //__cpp_BussinessThread_H__