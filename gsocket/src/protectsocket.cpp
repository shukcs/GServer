#include "protectsocket.h"
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

ProtectSocket::ProtectSocket(ISocketManager *parent): m_parent(parent)
,m_mgrPrcs(NULL), m_fd(-1), m_bListen(false)
, m_bAccept(false), m_stat(UnConnected), m_address(NULL)
, m_buffSocket(new LoopQueBuff(1024))
{
}

ProtectSocket::~ProtectSocket()
{
    delete m_address;
    delete m_buffSocket;
}

ILink *ProtectSocket::GetHandleLink() const
{
    return NULL;
}

void ProtectSocket::SetHandleLink(ILink *)
{
}

bool ProtectSocket::Bind(int port, const std::string &hostLocal)
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

bool ProtectSocket::ConnectTo(const std::string &hostRemote, int port)
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

int ProtectSocket::Send(int len, const void *buff)
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

void ProtectSocket::ClearBuff() const
{
    if (m_buffSocket)
        m_buffSocket->Clear();
}

bool ProtectSocket::IsListenSocket() const
{
    return m_bListen;
}

void ProtectSocket::Close()
{
    if (m_stat == Binded || m_stat == Connected)
    {
        m_stat = ISocket::Closing;
        if (m_mgrPrcs)
            m_mgrPrcs->AddWaitPrcsSocket(this);
    }
}

std::string ProtectSocket::GetHost() const
{
    return m_address ? m_address->Convert(false) : "";
}

uint16_t ProtectSocket::GetPort() const
{
    return m_address ? m_address->GetPort() : 0;
}

bool ProtectSocket::IsConnect() const
{
    return m_stat == ISocket::Connected;
}

ISocket::SocketStat ProtectSocket::GetSocketStat()const
{
    return m_stat;
}

SocketAddress *ProtectSocket::GetAddress() const
{
    return m_address;
}

void ProtectSocket::SetAddress(SocketAddress *a)
{
    m_address = a;
    m_bAccept = true;
}

int ProtectSocket::GetHandle() const
{
    return m_fd;
}

void ProtectSocket::SetHandle(int fd)
{
    m_fd = fd;
}

ISocketManager *ProtectSocket::GetParent() const
{
    return m_parent;
}

bool ProtectSocket::IsReconnectable() const
{
    return false;
}

bool ProtectSocket::IsWriteEnabled() const
{
    if (m_buffSocket)
        return m_buffSocket->BuffSize() > m_buffSocket->Count();

    return false;
}

void ProtectSocket::EnableWrite(bool)
{
}

void ProtectSocket::OnWrite(int len)
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

void ProtectSocket::OnRead(void *buf, int len)
{
}

void ProtectSocket::OnClose()
{
    m_stat = ISocket::Closed;

    if (m_parent)
        m_parent->ReleaseSocket(this);
}

void ProtectSocket::OnConnect(bool b)
{
    m_stat = b ? Connected : Closed;
    if (!b)
    {
        if (m_buffSocket)
            m_buffSocket->Clear();
    }
}

void ProtectSocket::OnBind(bool binded)
{
    m_stat = binded ? Binded : Closed;
}

int ProtectSocket::CopyData(char *buf, int sz) const
{
    if (buf && m_buffSocket && sz> 0 && m_buffSocket->Count()>0)
        return m_buffSocket->CopyData(buf, sz);
    return 0;
}

int ProtectSocket::GetSendLength() const
{
    return m_buffSocket ? m_buffSocket->Count() : 0;
}

bool ProtectSocket::ResetSendBuff(uint16_t sz)
{
    if (m_buffSocket)
        return m_buffSocket->ReSize(sz);

    return false;
}

bool ProtectSocket::IsAccetSock() const
{
    return m_bAccept;
}

void ProtectSocket::SetPrcsManager(ISocketManager *h)
{
    m_mgrPrcs = h;
}

ISocketManager *ProtectSocket::GetPrcsManager() const
{
    return m_mgrPrcs;
}

bool ProtectSocket::ResizeBuff(int sz)
{
    if (m_buffSocket)
        return m_buffSocket->ReSize(sz);

    return false;
}

bool ProtectSocket::IsNoWriteData() const
{
    return m_buffSocket && m_buffSocket->Count() == 0;
}
