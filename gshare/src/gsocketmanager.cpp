#include "gsocketmanager.h"
#include "Mutex.h"
#include "gsocket.h"
#include "Thread.h"
#include "Utility.h"
#include "Ipv4Address.h"
#include "Lock.h"
#include "ObjectBase.h"
#include "socket_include.h"
#include <stdio.h>
#if !defined _WIN32 && !defined _WIN64
#include <sys/epoll.h>
#include <sys/ioctl.h>
#endif

#define MAX_EVENT 20
#define SocketTimeOut 20
using namespace std;

class WSAInitializer // Winsock Initializer
{
public:
    WSAInitializer()
    {
#if defined _WIN32 || defined _WIN64
        WSADATA wsadata;
        if (WSAStartup(MAKEWORD(2, 2), &wsadata))
        {
            exit(-1);
        }
#endif //defined _WIN32 || defined _WIN64
    }
    ~WSAInitializer()
    {
#if defined _WIN32 || defined _WIN64
        WSACleanup();
#endif //defined _WIN32 || defined _WIN64
    }
private:
};

#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE
#endif

class ThreadMgr : public Thread
{
public:
    ThreadMgr(ISocketManager *sm):Thread(), m_mgr(sm) {}
    ~ThreadMgr()
    {
        delete m_mgr;
    }
protected:
    bool RunLoop()
    {
        if (!m_mgr->IsRun())
            return false;

        return m_mgr->Poll(50);
    }
private:
    ISocketManager *m_mgr;
};
////////////////////////////////////////////////////////////////////////////
//GSocketManager
////////////////////////////////////////////////////////////////////////////
bool GSocketManager::s_bRun = true;
GSocketManager::GSocketManager(int nThread, int maxSock) : m_openMax(maxSock)
,m_mtx(new Mutex) , m_thread(nullptr)
{
    InitEpoll();
    InitThread(nThread);
}

GSocketManager::~GSocketManager()
{
    if(IsOpenEpoll())
    {
#if !defined _WIN32 && !defined _WIN64
        close(m_ep_fd);       /* 创建epoll模型,ep_fd指向红黑树根节点 */
#endif
    }

    for (GSocketManager *m : m_othManagers)
    {
        m->CloseThread();
    }
    Utility::Sleep(100);
    delete m_thread;
}

ISocketManager *GSocketManager::CreateManager(int nThread, int maxSock)
{
    return  new GSocketManager(nThread, maxSock);
}

bool GSocketManager::AddSocket(ISocket *s)
{
    if (int(m_sockets.size()+1) >= m_openMax)
        return false;

    Lock l(m_mtx);
    m_socketsAdd.push(s);
    s->SetPrcsManager(this);
    return true;
}

void GSocketManager::ReleaseSocket(ISocket *s)
{
    Lock l(m_mtx);
    m_socketsRemove.push(s);
}

bool GSocketManager::IsMainManager()const
{
    return m_thread==NULL;
}

bool GSocketManager::Poll(unsigned ms)
{
    auto sec = Utility::secTimeCount();
    PrcsAddSockets(sec);
    PrcsDestroySockets();
    bool ret = PrcsSockets();
    _checkSocketTimeout(sec);
    ret = SokectPoll(ms) ? true : ret;
    return ret;
}

void GSocketManager::AddProcessThread()
{
    GSocketManager *m = new GSocketManager(0, m_openMax);
    ThreadMgr *t = new ThreadMgr(m);
    if (m &&t)
    {
        m->m_thread = t;
        m_othManagers.push_back(m);
        t->SetRunning();
    }
}

bool GSocketManager::AddWaitPrcsSocket(ISocket *s)
{
    int fd = s != nullptr ? s->GetSocketHandle() : -1;
    if (fd==-1 || m_sockets.find(fd)==m_sockets.end())
        return false;

    Lock l(m_mtx);//应对不同线程
    m_socketsPrcs.push(s->GetSocketHandle());
    return true;
}

void GSocketManager::InitThread(int nThread)
{
    if (nThread <= 0 || nThread>255)
        return;

    for (int i = 0; i < nThread; i++)
    {
        AddProcessThread();
    }
}

void GSocketManager::PrcsAddSockets(int64_t sec)
{
    ISocket *s = NULL;
    Lock l(m_mtx);
    while (Utility::Pop(m_socketsAdd, s))
    {
        int handle = s->GetSocketHandle();
        ISocket::SocketStat st = s->GetSocketStat();
        switch (st)
        {
        case ISocket::Binding:
            handle = _bind(s);
            break;
        case ISocket::Connecting:
            handle = _connect(s);
        default:
            break;
        }
        if (handle != -1)
        {
            SetNonblocking(s);
            _addSocketHandle(handle, s->IsListenSocket());
            _addAcceptSocket(s, sec);
            m_sockets[handle] = s;
        }
        if (ISocket::Connecting == st)
            s->OnConnect(-1 != handle);
    }
}

void GSocketManager::PrcsDestroySockets()
{
    ISocket *s;
    Lock l(m_mtx);
    while (Utility::Pop(m_socketsRemove, s))
    {
        delete s;
    }
}

bool GSocketManager::PrcsSockets()
{
    bool ret = false;
    int fd;
    Lock l(m_mtx);
    while (Utility::Pop(m_socketsPrcs, fd))
    {
        ISocket *s = GetSockByHandle(fd);
        if (NULL == s)
            continue;

        ISocket::SocketStat st = s->GetSocketStat();
        if (ISocket::Closing == st)
            _close(s);
        else if (ISocket::CloseLater == st)
            _closeAfterSend(s);
        else if (ISocket::Connected==st && !s->IsListenSocket())
            _send(s);
        ret = true;
    }
    return ret;
}

ISocket *GSocketManager::GetSockByHandle(int handle) const
{
    map<int, ISocket*>::const_iterator itr = m_sockets.find(handle);
    if (itr != m_sockets.end())
        return itr->second;

    return NULL;
}

void GSocketManager::CloseThread()
{
    if (m_thread)
        m_thread->SetRunning(false);
}

GSocketManager *GSocketManager::GetManagerOfLeastSocket() const
{
    int count = -1;
    GSocketManager *ret = nullptr;
    for (GSocketManager *itr : m_othManagers)
    {
        int tmp = itr->m_sockets.size();
        if (count < 0 || tmp < count)
        {
            count = tmp;
            ret = itr;
        }
    }
    return ret;
}

bool GSocketManager::SokectPoll(unsigned ms)
{
    list<ISocket *> lsRm;
    bool ret = false;
#if defined _WIN32 || defined _WIN64
    if (m_maxsock <= 0)
        return ret;
    fd_set rfds = m_ep_fd;
    struct timeval tv = {0};    //windows select不会挂起线程的
    int n = select((int)(m_maxsock + 1), &rfds, nullptr, nullptr, &tv);
    for (int i=0; i<n; ++i)
    {
        ISocket *sock = GetSockByHandle(rfds.fd_array[i]);
        if (sock)
        {
            if (sock->IsListenSocket())
                _accept(rfds.fd_array[i]);
            else if (!_recv(sock))
                _close(sock);
            ret = true;
        }
    }
#else
    struct epoll_event events[MAX_EVENT] = { {0} };
    int nfds = epoll_wait(m_ep_fd, events, 20, ms);
    for (int i = 0; i < nfds; ++i)
    {
        struct epoll_event *ev = events + i;
        ISocket *sock = GetSockByHandle(ev->data.fd);
        if (!sock)
        {
            _remove(ev->data.fd);
            continue;
        }
        if (sock->IsListenSocket()) //有新的连接
            _accept(ev->data.fd);
        else if (_isCloseEvent(ev->events))
            _close(sock);
        else if ((ev->events & EPOLLIN) && !_recv(sock))  //接收到数据，读socket
            _close(sock);
        else if (ev->events&EPOLLOUT)                      //数据发送就绪
            sock->EnableWrite(true);
        ret = true;
    }
#endif

    return ret;
}

bool GSocketManager::SetNonblocking(ISocket *sock)
{
    int h = sock ? sock->GetSocketHandle() : -1;
    if (-1 == h)
        return false;

    unsigned long ul = 1;
#if defined _WIN32 || defined _WIN64
    if(SOCKET_ERROR == ioctlsocket(h, FIONBIO, &ul))
#else
    if (0 != ioctl(h, FIONBIO, &ul))//设置成非阻塞模式；
#endif
        return false;
    return true;
}

bool GSocketManager::IsOpenEpoll() const
{
#if defined _WIN32 || defined _WIN64
    return true;
#else
    return m_ep_fd != -1;
#endif
}

void GSocketManager::InitEpoll()
{
    static WSAInitializer s;
#if defined _WIN32 || defined _WIN64
    FD_ZERO(&m_ep_fd);
    if (m_openMax > FD_SETSIZE)
        m_openMax = FD_SETSIZE;
    m_maxsock = 0;
#else
    m_ep_fd = epoll_create(m_openMax);       /* 创建epoll模型,ep_fd指向红黑树根节点 */
#endif
}

void GSocketManager::Release()
{
    delete this;
}

void GSocketManager::SetBindedCB(ISocket *s, FuncOnBinded cb)
{
    if (s)
        m_bindedCBs[s] = cb;
}

void GSocketManager::CloseServer()
{
    for (const pair<int, ISocket*> &itr : m_sockets)
    {
        int h = itr.first;
        closesocket(h);
#if defined _WIN32 || defined _WIN64
        if (FD_ISSET(h, &m_ep_fd))
            FD_CLR(h, &m_ep_fd);
        if (h >= (int)m_maxsock)
            _checkMaxSock();
#else
        epoll_ctl(m_ep_fd, EPOLL_CTL_DEL, h, nullptr);
#endif
    }
    
    m_sockets.clear();
    for (GSocketManager* itr : m_othManagers)
    {
        itr->CloseServer();
    }
}

bool GSocketManager::IsRun() const
{
    return s_bRun;
}

void GSocketManager::Exit()
{
    s_bRun = false;
}

void GSocketManager::_remove(int h)
{
    if (-1==h)
        return;

    m_sockets.erase(h);
    closesocket(h);
#if defined _WIN32 || defined _WIN64
    if (FD_ISSET(h, &m_ep_fd))
        FD_CLR(h, &m_ep_fd);
    if (h >= (int)m_maxsock)
        _checkMaxSock();
#else
    epoll_ctl(m_ep_fd, EPOLL_CTL_DEL, h, nullptr);
#endif
}

void GSocketManager::_addSocketHandle(int h, bool bListen)
{
#if defined _WIN32 || defined _WIN64
    if (!FD_ISSET(h, &m_ep_fd))
        FD_SET(h, &m_ep_fd);
    if (h > (int)m_maxsock)
        m_maxsock = h;
#else
    struct epoll_event e = { 0 };
    e.events = EPOLLIN | EPOLLRDHUP;    //指定监听读关闭事件 注意:默认为水平触发LT
    if (!bListen)           //非监听
        e.events |= EPOLLET | EPOLLOUT;
    e.data.fd = h;                 //一般的epoll在这里放fd 
    epoll_ctl(m_ep_fd, EPOLL_CTL_ADD, h, &e);
#endif
}

int GSocketManager::_createSocket(int tp)
{
    return socket(AF_INET, tp, IPPROTO_TCP);
}

void GSocketManager::_close(ISocket *sock, bool prcs)
{
    _remove(sock->GetSocketHandle());
    if (prcs)
        _earseListen(*sock);
    sock->OnClose();
}

void GSocketManager::_closeAfterSend(ISocket *sock)
{
    _send(sock);
    _close(sock);
}

void GSocketManager::_addAcceptSocket(ISocket *sock, int64_t sec)
{
    if (!sock || !sock->IsAccetSock())
        return;

    sock->SetCheckTime(sec);
    auto itr = m_socketsAccept.find(sec);
    if (itr == m_socketsAccept.end())
        m_socketsAccept[sec].push_back(sock);
    else
        itr->second.push_back(sock);
}

void GSocketManager::_checkSocketTimeout(int64_t sec)
{
    if (m_socketsAccept.empty())
        return;

    auto itr = m_socketsAccept.begin();
    if (sec - itr->first > SocketTimeOut)
    {
        for (auto sock : itr->second)
        {
            if (!sock->GetHandleLink())
                _close(sock, false);
        }
        itr->second.clear();
        m_socketsAccept.erase(itr);
    }
}

#if defined _WIN32 || defined _WIN64 
void GSocketManager::_checkMaxSock()
{
    map<int, ISocket*>::iterator itr = m_sockets.end();
    m_maxsock = itr == m_sockets.begin() ? 0 : (--itr)->first;
}
#else
bool GSocketManager::_isCloseEvent(uint32_t evt) const
{
    //socket断开，这些消息不定有
    return (evt&EPOLLERR) || (evt&EPOLLRDHUP) || (evt&EPOLLHUP);
}
#endif

int GSocketManager::_bind(ISocket *sock)
{
    if (!sock || sock->GetSocketStat()!=ISocket::Binding || !sock->GetAddress())
        return -1;

    int listenfd = _createSocket();
    if (-1 == listenfd)
        return -1;

    struct sockaddr_in serveraddr = {0};
    serveraddr = *(sockaddr_in*)&**sock->GetAddress();//htons(portnumber);
    bool binded = true;
    if(SOCKET_ERROR == bind(listenfd, (sockaddr *)&serveraddr, sizeof(serveraddr)))
    {
        closesocket(listenfd);
        listenfd = -1;
        binded = false;
    }
    else
    {
        listen(listenfd, 20);
        sock->SetSocketHandle(listenfd);
        sock->OnBind();
    }
    SocketBindedCallbacks::iterator itr = m_bindedCBs.find(sock);
    if (itr != m_bindedCBs.end())
        itr->second(sock, binded);
    return listenfd;
}

int GSocketManager::_connect(ISocket *sock)
{
    if (!sock || sock->GetSocketStat() != ISocket::Connecting)
        return -1;

    int listenfd = _createSocket();
    if (-1 == listenfd)
    {
        sock->OnConnect(false);
        return -1;
    }

    auto addr = sock->GetAddress();
    if (connect(listenfd, *addr, socklen_t(*addr)) != 0)
    {
        closesocket(listenfd);
        listenfd = -1;
    }
    sock->SetSocketHandle(listenfd);
    return listenfd;
}

void GSocketManager::_accept(int listenfd)
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    int connfd = accept(listenfd, (sockaddr *)&addr, &len); //accept这个连接
    if (-1 == connfd)
        return;

    if (ISocket *s = new GSocket(this))
    {
        Ipv4Address *ad = new Ipv4Address(addr);
        s->SetAddress(ad);
        s->SetSocketHandle(connfd);
        s->OnConnect(true);
        GSocketManager *m = GetManagerOfLeastSocket();
        if (!m)
            m = this;
        if (!m->AddSocket(s))
            _close(s);
    }
}

bool GSocketManager::_recv(ISocket *sock)
{
    int fd = sock ? sock->GetSocketHandle() : -1;
    if (fd == -1)
        return false;

    int n = sizeof(m_buff);
    while (sizeof(m_buff) == n)
    {
#if defined _WIN32 ||  defined _WIN64
        n = recv(fd, m_buff, sizeof(m_buff), 0);
#else
        n = read(fd, m_buff, sizeof(m_buff));
#endif
        if (n > 0)
            sock->OnRead(m_buff, n);
        else //关闭触发EPOLLIN，但接收不到数据
            return n == 0;
    }
    return true;
}

void GSocketManager::_send(ISocket *sock)
{
    int fd = sock ? sock->GetSocketHandle() : -1;
    if (-1 == fd)
        return;

    int sended = 0;
    int len = sock->GetSendLength();
    while (len>sended)
    {
        int res = sock->CopyData(m_buff, sizeof(m_buff));
#if defined _WIN32 ||  defined _WIN64
        res = send(fd, m_buff, res, 0);
#else
        res = write(fd, m_buff, res);
#endif
        sended += res;
        if (res > 0)
            sock->OnWrite(res);
        if(res < (int)sizeof(m_buff))
            break;
    }
    if (sended < 0)
        _close(sock);
}

void GSocketManager::_earseListen(ISocket &sock)
{
    int64_t t = sock.GetCheckTime();
    auto itr = t > 0 ? m_socketsAccept.find(t) : m_socketsAccept.end();
    if (itr != m_socketsAccept.end())
    {
        itr->second.remove(&sock);
        if (itr->second.empty())
            m_socketsAccept.erase(itr);
    }
}