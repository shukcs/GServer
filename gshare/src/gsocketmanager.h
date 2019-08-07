#ifndef __GSOCKET_MANAGER_H__
#define __GSOCKET_MANAGER_H__

#include "stdconfig.h"
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
class GSocketManager : public ISocketManager {
public:
/***********************************************************************
GSocketManager是个线程池，管理secket整个生存周期
使用epoll模型GSocketManager(nThread, maxSock)
nThread:创建子线程个数,0将不创建新线程，一般当客户端使用
maxSock:支持最大连接数
************************************************************************/
    SHARED_DECL GSocketManager(int nThread=0, int maxSock=100000);
    SHARED_DECL ~GSocketManager();

    SHARED_DECL bool AddSocket(ISocket *);
    SHARED_DECL void ReleaseSocket(ISocket *);
    SHARED_DECL bool IsMainManager()const;
    SHARED_DECL bool Poll(unsigned ms);
    SHARED_DECL void AddProcessThread();
    SHARED_DECL bool AddSocketWaitPrcs(ISocket *);
    SHARED_DECL void SetLog(ILog *log);
    void Log(int err, const std::string &obj, int evT, const char *fmt, ...);
protected:
    void InitThread(int nThread);
    void PrcsAddSockets();
    void PrcsDestroySockets();
    void PrcsSockets();
    void SokectPoll(unsigned ms);
    ISocket *GetSockByHandle(int handle)const;
    void CloseThread();
    GSocketManager *GetManagerofLeastSocket()const;
    bool SetNonblocking(ISocket *sock);
    bool IsOpenEpoll()const;
    void InitEpoll();
private:
    int _bind(ISocket *sock);
    int _connect(ISocket *sock);
    void _accept(int fd);
    bool _recv(ISocket *sock);
    bool _send(ISocket *sock);
    void _remove(int h);
    void _addSocketHandle(int h, bool bListen=false);
    int _createSocket(int tp=SOCK_STREAM);
    bool _containsSocketPrcs(ISocket *sock)const;
private:
    std::map<int, ISocket*>     m_sockets;
    std::list<ISocket*>         m_socketsPrcs;
    std::list<ISocket*>         m_socketsRemove;
    std::list<ISocket*>         m_socketsAdd;
    std::list<GSocketManager*>  m_othManagers;
    IMutex                      *m_mtx;
    int                         m_openMax;
#if defined _WIN32 || defined _WIN64 //Windows与epoll对应的是IOCP，但不好做成epoll一样操作
    void _checkMaxSock();
    SOCKET                  m_maxsock;
    fd_set                  m_ep_fd;
#else
    int                         m_ep_fd;
#endif
    Thread                      *m_thread;
    char                        m_buff[1024];
    ILog                        *m_log;
};

#ifdef SOCKETS_NAMESPACE
}
#endif

#endif //__GSOCKET_MANAGER_H__