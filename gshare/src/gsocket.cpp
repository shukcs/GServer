#include "gsocket.h"
#include "gsocketmanager.h"
#include "Ipv4Address.h"
#include "Mutex.h"
#include "ObjectBase.h"
#include "ObjectManagers.h"
#include "Lock.h"
#include "ILog.h"
#include "LoopQueue.h"
#include <string.h>
#include <stdarg.h>

#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE
#endif

GSocket::GSocket(ISocketManager *handle): m_manager(handle)
,m_mgrPrcs(NULL), m_object(NULL), m_fd(-1), m_bListen(false)
, m_bAccept(false), m_stat(UnConnected), m_address(NULL)
, m_buffSocket(new LoopQueBuff)
{
}

GSocket::~GSocket()
{
    delete m_address;
    delete m_buffSocket;
}

IObject *GSocket::GetOwnObject() const
{
    return m_object;
}

void GSocket::SetObject(IObject *o)
{
    m_object = o;
}

bool GSocket::Bind(int port, const std::string &hostLocal)
{
    if (!m_address && m_stat==UnConnected && port!=0)
    {
        m_address = new Ipv4Address(hostLocal, port);
        m_stat = Binding;
        m_bListen = true;
        return true;
    }
    return false;
}

bool GSocket::ConnectTo(const std::string &hostRemote, int port)
{
    if (!m_address && m_stat == UnConnected)
    {
        m_address = new Ipv4Address(hostRemote, port);
        m_stat = Connecting;
        m_bListen = false;
        m_buffSocket->ReSize(2048);
        return true;
    }
    return false;
}

int GSocket::Send(int len, const void *buff)
{
    if (len < 1 || !IsConnect())
        return 0;

    int ret = len;
    if(buff)
        ret = m_buffSocket->Push(buff, len) ? len : 0;

    if (ret>0 && m_mgrPrcs)
        m_mgrPrcs->AddWaitPrcsSocket(this);

    return ret;
}

bool GSocket::IsListenSocket() const
{
    return m_bListen;
}

void GSocket::Close()
{
    if (m_stat == Binded || m_stat == Connected)
    {
        m_stat = ISocket::Closing;
        if (m_mgrPrcs)
            m_mgrPrcs->AddWaitPrcsSocket(this);
    }
}

ILog &GSocket::GetLog()
{
    return ObjectManagers::GetLog();
}

std::string GSocket::GetHost() const
{
    return m_address ? m_address->Convert(false) : "";
}

uint16_t GSocket::GetPort() const
{
    return m_address ? m_address->GetPort() : 0;
}

std::string GSocket::GetObjectID() const
{
    return m_object ? m_object->GetObjectID() : std::string();
}

bool GSocket::IsConnect() const
{
    return m_stat == ISocket::Connected;
}

ISocket::SocketStat GSocket::GetSocketStat()const
{
    return m_stat;
}

SocketAddress *GSocket::GetAddress() const
{
    return m_address;
}

void GSocket::SetAddress(SocketAddress *a)
{
    m_address = a;
    m_bAccept = true;
}

int GSocket::GetHandle() const
{
    return m_fd;
}

void GSocket::SetHandle(int fd)
{
    m_fd = fd;
}

ISocketManager *GSocket::GetManager() const
{
    return m_manager;
}

bool GSocket::IsReconnectable() const
{
    return false;
}

bool GSocket::IsWriteEnabled() const
{
    if (m_buffSocket)
        return m_buffSocket->BuffSize() > m_buffSocket->Count();

    return false;
}

void GSocket::EnableWrite(bool)
{
}

void GSocket::OnWrite(int len)
{
    if (len > 0 && m_buffSocket)
    {
        int nTmp = m_buffSocket->Count();
        if (nTmp>0)
        {
            m_buffSocket->Clear(len);
            len -= nTmp;
        }
    }
}

void GSocket::OnRead(void *buf, int len)
{
    if (!buf || len <= 0)
        return;

    if (m_object)
        m_object->Receive(buf, len);
    else
        ObjectManagers::Instance().ProcessReceive(this, buf, len);
}

void GSocket::OnClose()
{
    m_stat = ISocket::Closed;
    if (m_object)
        m_object->OnSockClose(this);
    else
        ObjectManagers::Instance().OnSocketClose(this);

    if (m_manager)
        m_manager->ReleaseSocket(this);
    m_object = NULL;
}

void GSocket::OnConnect(bool b)
{
    m_stat = b ? Connected : Closed;
    if (!b && m_buffSocket)
        m_buffSocket->Clear();

    if (m_object)
        m_object->OnConnected(b);
}

void GSocket::OnBind(bool binded)
{
    m_stat = binded ? Binded : Closed;
}

int GSocket::CopySend(char *buf, int sz) const
{
    if (buf && m_buffSocket && sz> 0 && m_buffSocket->Count()>0)
        return m_buffSocket->CopyData(buf, sz);
    return 0;
}

int GSocket::GetSendLength() const
{
    return m_buffSocket ? m_buffSocket->Count() : 0;
}

bool GSocket::ResetSendBuff(uint16_t sz)
{
    if (m_buffSocket)
        return m_buffSocket->ReSize(sz);

    return false;
}

bool GSocket::IsAccetSock() const
{
    return m_bAccept;
}

void GSocket::SetPrcsManager(ISocketManager *h)
{
    m_mgrPrcs = h;
}

bool GSocket::ResizeBuff(int sz)
{
    if (m_buffSocket)
        return m_buffSocket->ReSize(sz);

    return false;
}

bool GSocket::IsNoWriteData() const
{
    return m_buffSocket && m_buffSocket->Count() == 0;
}
