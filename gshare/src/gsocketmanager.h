﻿#ifndef __GSOCKET_MANAGER_H__
#define __GSOCKET_MANAGER_H__

#include "LoopQueue.h"
#include "socket_include.h"
#include "socketBase.h"
#include <map>
#include <list>

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif
class IMutex;
class Thread;
class ILog;
class ILink;

class GSocketManager : public ISocketManager
{
    typedef LoopQueue<ISocket *> SocketQue;
    typedef LoopQueue<ILink *> SocketHandleQue;
    typedef LoopQueue<int> SocketPrcsQue;
    typedef std::map<ISocket *, FuncOnBinded> SocketBindedCallbacks;
    typedef std::list<ISocket *> SocketList;
    typedef std::map<int64_t, SocketList> SecSocketMap;
public:
    SHARED_DECL static ISocketManager *CreateManager(int nThread = 0, int maxSock = 100000);
protected:
/***********************************************************************
GSocketManager是个线程池，管理secket整个生存周期
使用epoll模型GSocketManager(nThread, maxSock)
nThread:创建子线程个数,0将不创建新线程，一般当客户端使用
maxSock:支持最大连接数
************************************************************************/
    GSocketManager(int nThread, int maxSock);
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
    GSocketManager *GetManagerofLeastSocket()const;
    bool SetNonblocking(ISocket *sock);
    bool IsOpenEpoll()const;
    void InitEpoll();
    void Release();
    void SetBindedCB(ISocket *s, FuncOnBinded cb);
    void CloseServer();
private:
    int _bind(ISocket *sock);
    int _connect(ISocket *sock);
    void _accept(int fd);
    bool _recv(ISocket *sock);
    void _send(ISocket *sock);
    void _remove(int h);
    void _addSocketHandle(int h, bool bListen=false);
    int _createSocket(int tp=SOCK_STREAM);
    void _close(ISocket *sock, bool prcs=true);
    void _addAcceptSocket(ISocket *sock, int64_t sec);
    void _checkSocketTimeout(int64_t sec);
#if !defined _WIN32 && !defined _WIN64
    bool _isCloseEvent(uint32_t e)const;
#endif
private:
    int                         m_openMax;
    IMutex                      *m_mtx;
    Thread                      *m_thread;
    std::map<int, ISocket*>     m_sockets;
    SocketPrcsQue               m_socketsPrcs;      //等待处理队列
    SocketQue                   m_socketsRemove;    //等待销毁队列
    SocketQue                   m_socketsAdd;       //等待监控队列
    SocketHandleQue             m_handlesRelease;   //等待监控队列
    std::list<GSocketManager*>  m_othManagers;
    char                        m_buff[1024];
    static bool                 s_bRun;
    SocketBindedCallbacks       m_bindedCBs;
    SecSocketMap                m_socketsAccept;
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