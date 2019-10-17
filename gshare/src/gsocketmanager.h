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
    typedef std::pair<ISocket*, bool> SocketPrcs;
    typedef std::list<ISocket *> SocketQue;
    typedef std::list<SocketPrcs> SocketPrcsQue;
public:
    SHARED_DECL static ISocketManager *CreateManager(int nThread = 0, int maxSock = 100000);
protected:
/***********************************************************************
GSocketManager�Ǹ��̳߳أ�����secket������������
ʹ��epollģ��GSocketManager(nThread, maxSock)
nThread:�������̸߳���,0�����������̣߳�һ�㵱�ͻ���ʹ��
maxSock:֧�����������
************************************************************************/
    GSocketManager(int nThread, int maxSock);
    ~GSocketManager();

    bool AddSocket(ISocket *);
    void ReleaseSocket(ISocket *);
    bool IsMainManager()const;
    bool Poll(unsigned ms);
    void AddProcessThread();
    bool AddWaitPrcsSocket(ISocket *);
    void SetLog(ILog *log);
    void Log(int err, const std::string &obj, int evT, const char *fmt, ...);
    bool IsRun()const;
    void Exit();
    void InitThread(int nThread);
    void PrcsAddSockets();
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
#if !defined _WIN32 && !defined _WIN64
    bool _isCloseEvent(uint32_t e)const;
#endif
    void setISocketInvalid(ISocket *s);
private:
    std::map<int, ISocket*>     m_sockets;
    SocketPrcsQue               m_socketsPrcs;      //�ȴ��������
    SocketQue                   m_socketsRemove;    //�ȴ����ٶ���
    SocketQue                   m_socketsAdd;       //�ȴ���ض���
    std::list<GSocketManager*>  m_othManagers;
    IMutex                      *m_mtx;
    IMutex                      *m_mtxSock;
    int                         m_openMax;
#if defined _WIN32 || defined _WIN64 //Windows��epoll��Ӧ����IOCP������������epollһ������
    void _checkMaxSock();
    SOCKET                  m_maxsock;
    fd_set                  m_ep_fd;
#else
    int                         m_ep_fd;
#endif
    Thread                      *m_thread;
    ILog                        *m_log;
    char                        m_buff[1024];
    static bool                 s_bRun;
};

#ifdef SOCKETS_NAMESPACE
}
#endif

#endif //__GSOCKET_MANAGER_H__