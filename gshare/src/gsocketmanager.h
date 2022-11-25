#ifndef __GSOCKET_MANAGER_H__
#define __GSOCKET_MANAGER_H__

#include "LoopQueue.h"
#include "socket_include.h"
#include "socketBase.h"
#include <map>
#include <list>
#include <queue>

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif
class Thread;
class ILog;
class ILink;

class GSocketManager : public ISocketManager
{
    typedef std::queue<ISocket *> SocketQue;
    typedef std::queue<ILink *> SocketHandleQue;
    typedef std::queue<int> SocketPrcsQue;
    typedef std::map<ISocket *, FuncOnBinded> SocketBindedCallbacks;
public:
    SHARED_DECL static ISocketManager *CreateManager(int nThread = 0, int maxSock = 100000);
protected:
/***********************************************************************
GSocketManager是个线程池，管理secket整个生存周期
使用epoll模型GSocketManager(nThread, maxSock)
nThread:创建子线程个数,0将不创建新线程，一般当客户端使用
maxSock:支持最大连接数
************************************************************************/
    GSocketManager(GSocketManager *parent, int nThread, int maxSock);
    ~GSocketManager();

    bool AddSocket(ISocket *);
    void ReleaseSocket(ISocket *);
    bool IsMainManager()const;
    bool Poll(unsigned ms);
    void AddProcessThread();
    bool AddWaitPrcsSocket(ISocket *);
    bool IsRun()const;
    void Exit();
    void InitThread(int nThread);
    void PrcsAddSockets(int64_t sec);
    void PrcsDestroySockets();
    bool PrcsSockets();
    bool SokectPoll(unsigned ms);
    ISocket *GetSockByHandle(int handle)const;
    void CloseThread();
    GSocketManager *GetManagerOfLeastSocket()const;
    bool SetNonblocking(ISocket *sock);
    bool IsOpenEpoll()const;
    void InitEpoll();
    void Release();
    void SetBindedCB(ISocket *s, FuncOnBinded cb);
    void CloseServer();
    uint32_t CountSocket()const;
protected:
    int PopWaitSock();
private:
    int _bind(ISocket *sock);
    int _connect(ISocket *sock);
    void _accept(int fd);
    bool _recv(ISocket *sock);
    void _send(ISocket *sock);
    void _remove(int h);
    void _addSocketHandle(int h, bool bListen=false);
    int _createSocket(int tp=SOCK_STREAM);
    void _close(ISocket *sock);
    void _closeAfterSend(ISocket *sock);
    void _addAcceptSocket(ISocket *sock, int64_t sec);
    void _checkSocketTimeout(int fd, int64_t us);
#if !defined _WIN32 && !defined _WIN64
    bool _isCloseEvent(uint32_t e)const;
#endif
private:
	friend class ThreadMgr;
    int                         m_openMax;
    ISocketManager              *m_parent;
    std::mutex                  *m_mtx;
    Thread                      *m_thread;
    std::map<int, ISocket*>     m_sockets;
    SocketPrcsQue               m_socketsPrcs;      //等待处理队列
    SocketQue                   m_socketsRemove;    //等待销毁队列
    SocketQue                   m_socketsAdd;       //等待监控队列
    std::list<GSocketManager*>  m_othManagers;
    char                        m_buff[1024];
    static bool                 s_bRun;
    SocketBindedCallbacks       m_bindedCBs;
#if defined _WIN32 || defined _WIN64 //Windows与epoll对应的是IOCP，但不好做成epoll一样操作
    void _checkMaxSock();
    SOCKET                      m_maxsock;
    fd_set                      m_ep_fd;
#else
    int                         m_ep_fd;
#endif
};

#ifdef SOCKETS_NAMESPACE
}
#endif

#endif //__GSOCKET_MANAGER_H__