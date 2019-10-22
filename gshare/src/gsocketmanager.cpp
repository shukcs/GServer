#include "gsocketmanager.h"
#include "Mutex.h"
#include "gsocket.h"
#include "Thread.h"
#include "Utility.h"
#include "Ipv4Address.h"
#include "GOutLog.h"
#include "Lock.h"
#include "ObjectBase.h"
#include "socket_include.h"
#include <stdarg.h>
#include <stdio.h>
#if !defined _WIN32 && !defined _WIN64
#include <sys/epoll.h>
#include <sys/ioctl.h>
#endif

#define MAX_EVENT 20
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

static GOutLog s_log;
bool GSocketManager::s_bRun = true;
////////////////////////////////////////////////////////////////////////////
//GSocketManager
////////////////////////////////////////////////////////////////////////////
GSocketManager::GSocketManager(int nThread, int maxSock) : m_mtx(new Mutex)
, m_mtxSock(new Mutex), m_openMax(maxSock), m_thread(NULL), m_log(&s_log)
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

    s->SetMutex(m_mtxSock);
    m_socketsAdd.push_back(s);
    s->SetPrcsManager(this);
    return true;
}

void GSocketManager::ReleaseSocket(ISocket *s)
{
    if (IObject::IsContainsInList(m_socketsRemove, s))
        return;

    m_mtx->Lock();
    m_socketsRemove.push_back(s);
    m_mtx->Unlock();
}

bool GSocketManager::IsMainManager()const
{
    return m_thread==NULL;
}

bool GSocketManager::Poll(unsigned ms)
{
    PrcsAddSockets();
    PrcsDestroySockets();
    bool ret = PrcsSockets();
    ret = SokectPoll(ms)?true:ret;
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
    if (!s || m_sockets.find(s->GetHandle()) == m_sockets.end())
        return false;

    m_mtx->Lock();      //应对不同线程
    m_socketsPrcs.push_back(SocketPrcs(s,true));
    m_mtx->Unlock();
    return true;
}

void GSocketManager::SetLog(ILog *log)
{
    m_log = log;
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

void GSocketManager::PrcsAddSockets()
{
    while (m_socketsAdd.size() > 0)
    {
        ISocket *s = m_socketsAdd.front();
        int handle = s->GetHandle();
        switch (s->GetSocketStat())
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
            m_sockets[handle] = s;
        }
        m_socketsAdd.pop_front();
    }
}

void GSocketManager::PrcsDestroySockets()
{
    while (m_socketsRemove.size() > 0)
    {
        delete m_socketsRemove.front();
        m_socketsRemove.pop_front();
    }
}

bool GSocketManager::PrcsSockets()
{
    if (m_socketsPrcs.size() <= 0)
        return false;

    SocketPrcsQue::iterator itr = m_socketsPrcs.begin();
    while (itr != m_socketsPrcs.end())
    {
        ISocket *s = itr->first;
        if (itr->second)
        {
            ISocket::SocketStat st = s->GetSocketStat();
            if (ISocket::Closing == st)
                _close(s);

            if (!s->IsListenSocket() && ISocket::Connected == st)
            {
                _send(s);
                if (s->GetSendLength() == 0)
                    itr->second = false;
            }
        }

        if (!itr->second)
        {
            SocketPrcsQue::iterator itrTmp = itr++;
            m_socketsPrcs.erase(itrTmp);
            continue;
        }
        ++itr;
    }
    return true;
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

GSocketManager *GSocketManager::GetManagerofLeastSocket() const
{
    int count = -1;
    GSocketManager *ret = NULL;
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
    struct timeval tv;
    tv.tv_sec = ms / 1000;
    tv.tv_usec = (ms % 1000) * 1000;
    int n = select((int)(m_maxsock + 1), &rfds, NULL, NULL, &tv);
    for (int i=0; i<n; ++i)
    {
        ISocket *sock = GetSockByHandle(rfds.fd_array[i]);
        if (sock)
        {
            if (sock->IsListenSocket())
                _accept(rfds.fd_array[i]);
            else if (!_recv(sock))
                lsRm.push_back(sock);
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
            lsRm.push_back(sock);
        else if ((ev->events & EPOLLIN) && !_recv(sock))  //接收到数据，读socket
            lsRm.push_back(sock);
        else if (ev->events&EPOLLOUT)                      //数据发送就绪
            sock->EnableWrite(true);
        ret = true;
    }
#endif
    for (ISocket *s : lsRm)
    {
        setISocketInvalid(s);
        s->OnClose();
        _remove(s->GetHandle());
    }

    return ret;
}

bool GSocketManager::SetNonblocking(ISocket *sock)
{
    int h = sock ? sock->GetHandle() : -1;
    if (-1 == h)
        return false;
#if defined _WIN32 || defined _WIN64
    unsigned long ul=1;
    if(SOCKET_ERROR == ioctlsocket(h, FIONBIO, &ul))
#else
    if (0 != ioctl(h, FIONBIO, 1))//设置成非阻塞模式；
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

void GSocketManager::Log(int err, const std::string &obj, int evT, const char *fmt, ...)
{
    if (m_log)
    {
        char slask[1024];
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(slask, 1023, fmt, ap);
        va_end(ap);

        m_log->Log(slask, obj, evT, err);
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
    epoll_ctl(m_ep_fd, EPOLL_CTL_DEL, h, NULL);
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

void GSocketManager::_close(ISocket *sock)
{
    _remove(sock->GetHandle());
    sock->OnClose();
    setISocketInvalid(sock);
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
        sock->SetHandle(listenfd);
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

    sock->SetHandle(listenfd);
    struct sockaddr_in serveraddr = { 0 };
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(sock->GetHost().c_str());//htons(portnumber);
    serveraddr.sin_port = htons(sock->GetPort());
    sock->OnConnect(0 == connect(listenfd, (sockaddr *)&serveraddr, sizeof(serveraddr)));
    return listenfd;
}

void GSocketManager::_accept(int listenfd)
{
    struct sockaddr_in clientaddr;
    socklen_t len = sizeof(clientaddr);
    int connfd = accept(listenfd, (sockaddr *)&clientaddr, &len); //accept这个连接
    if (-1 == connfd)
        return;

    if (ISocket *s = new GSocket(this))
    {
        Ipv4Address *ad = new Ipv4Address(clientaddr);
        s->SetAddress(ad);
        s->SetHandle(connfd);
        s->OnConnect(true);
        GSocketManager *m = GetManagerofLeastSocket();
        if (!m)
            m = this;
        m->AddSocket(s);
    }
}

    bool GSocketManager::_recv(ISocket *sock)
{
    int fd = sock ? sock->GetHandle() : -1;
    if (fd == -1)
        return false;

#if defined _WIN32 ||  defined _WIN64
    int n = recv(fd, m_buff, sizeof(m_buff), 0);
#else
    int n = read(fd, m_buff, sizeof(m_buff));
#endif
    if (n <= 0)//关闭触发EPOLLIN，但接收不到数据
        return false;

    sock->OnRead(m_buff, n);
    while (n >= int(sizeof(m_buff)))
    {
#if defined _WIN32 ||  defined _WIN64
        n = recv(fd, m_buff, sizeof(m_buff), 0);
#else
        n = read(fd, m_buff, sizeof(m_buff));
#endif
        sock->OnRead(m_buff, n);
    }
    return true;
}

void GSocketManager::_send(ISocket *sock)
{
    int fd = sock ? sock->GetHandle() : -1;
    int len = sock->GetSendLength();
    if (-1 == fd)
        return;

    int count = 0;
    while (len>count)
    {
        int res = sock->CopySend(m_buff, sizeof(m_buff), count);
#if defined _WIN32 ||  defined _WIN64
        res = send(fd, m_buff, res, 0);
#else
        res = write(fd, m_buff, res);
#endif
        count += res;
        if(res < (int)sizeof(m_buff))
            break;
    }
    if (count > 0)
        sock->OnWrite(count);
    else if (count < 0)
        _close(sock);
}

void GSocketManager::setISocketInvalid(ISocket *s)
{
    for (SocketPrcs &itr : m_socketsPrcs)
    {
        if (itr.first == s)
            itr.second = false;
    }
}