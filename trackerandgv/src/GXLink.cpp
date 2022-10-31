#include "GXLink.h"
#include "gsocketmanager.h"
#include "Ipv4Address.h"
#include "GXClient.h"

#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE
#endif

GXClientSocket::GXClientSocket(GXManager *parent): m_parent(parent)
,m_mgrPrcs(NULL), m_fd(-1), m_stat(UnConnected), m_address(NULL)
, m_buffSnd(new LoopQueBuff(1024)), m_buffRcv(new LoopQueBuff(1024))
{
}

GXClientSocket::~GXClientSocket()
{
    delete m_address;
    delete m_buffSnd;
}

ILink *GXClientSocket::GetHandleLink() const
{
    return NULL;
}

void GXClientSocket::SetHandleLink(ILink *)
{
}

bool GXClientSocket::Bind(int, const std::string &)
{
    return false;
}

bool GXClientSocket::ConnectTo(const std::string &hostRemote, int port)
{
    if (!m_address && m_stat == UnConnected)
    {
        m_address = new Ipv4Address(hostRemote, port);
        m_stat = Connecting;
        m_buffSnd->ReSize(2048);
        return true;
    }
    return false;
}

int GXClientSocket::Send(int len, const void *buff)
{
    if (len < 1 || !IsConnect())
        return 0;

    int ret = len;
    if(buff)
        ret = m_buffSnd->Push(buff, len) ? len : 0;

    if (ret>0 && m_mgrPrcs)
        m_mgrPrcs->AddWaitPrcsSocket(this);

    return ret;
}

void GXClientSocket::ClearBuff() const
{
    if (m_buffSnd)
        m_buffSnd->Clear();
}

bool GXClientSocket::IsListenSocket() const
{
    return false;
}

void GXClientSocket::Close(bool)
{
    if (m_stat == Binded || m_stat == Connected)
    {
        m_stat = ISocket::Closing;
        if (m_mgrPrcs)
            m_mgrPrcs->AddWaitPrcsSocket(this);
    }
}

int GXClientSocket::CopyRcv(char *buf, int sz) const
{
    if (buf && m_buffRcv && sz > 0 && m_buffRcv->Count() > 0)
        return m_buffRcv->CopyData(buf, sz);
    return 0;
}

void GXClientSocket::ClearRcv(int len)
{
    if(m_buffRcv)
        m_buffRcv->Clear(len);
}

std::string GXClientSocket::GetHost() const
{
    return m_address ? m_address->Convert(false) : "";
}

uint16_t GXClientSocket::GetPort() const
{
    return m_address ? m_address->GetPort() : 0;
}

bool GXClientSocket::IsConnect() const
{
    return m_stat == ISocket::Connected;
}

ISocket::SocketStat GXClientSocket::GetSocketStat()const
{
    return m_stat;
}

SocketAddress *GXClientSocket::GetAddress() const
{
    return m_address;
}

void GXClientSocket::SetAddress(SocketAddress *)
{
}

int GXClientSocket::GetSocketHandle() const
{
    return m_fd;
}

void GXClientSocket::SetSocketHandle(int fd)
{
    m_fd = fd;
}

ISocketManager *GXClientSocket::GetParent() const
{
    return NULL;
}

bool GXClientSocket::IsReconnectable() const
{
    return false;
}

bool GXClientSocket::IsWriteEnabled() const
{
    if (m_buffSnd)
        return m_buffSnd->BuffSize() > m_buffSnd->Count();

    return false;
}

void GXClientSocket::EnableWrite(bool)
{
}

void GXClientSocket::OnWrite(int len)
{
    if (len > 0 && m_buffSnd)
    {
        int nTmp = m_buffSnd->Count();
        if (nTmp>0)
        {
            m_buffSnd->Clear(len);
            len -= nTmp;
        }
    }
}

void GXClientSocket::OnRead(const void *buf, int len)
{
    m_buffRcv->Push(buf, len);
    if (m_parent)
        m_parent->OnRead(*this);
}

void GXClientSocket::OnClose()
{
    m_stat = ISocket::Closed;
    OnConnect(false);
}

void GXClientSocket::OnConnect(bool b)
{
    m_stat = b ? Connected : Closed;
    if (!b)
    {
        if (m_buffSnd)
            m_buffSnd->Clear();
        if (m_buffRcv)
            m_buffRcv->Clear();
    }

    if (m_parent)
        m_parent->OnConnect(this, b);
}

void GXClientSocket::OnBind(bool binded)
{
    m_stat = binded ? Binded : Closed;
}

int GXClientSocket::CopyData(char *buf, int sz) const
{
    if (buf && m_buffSnd && sz> 0 && m_buffSnd->Count()>0)
        return m_buffSnd->CopyData(buf, sz);
    return 0;
}

int GXClientSocket::GetSendLength() const
{
    return m_buffSnd ? m_buffSnd->Count() : 0;
}

int GXClientSocket::GetSendRemain() const
{
    return m_buffSnd ? (m_buffSnd->BuffSize() - m_buffSnd->Count()) : 0;
}

bool GXClientSocket::ResetSendBuff(uint16_t sz)
{
    if (m_buffSnd)
        return m_buffSnd->ReSize(sz);
    if (m_buffRcv)
        return m_buffRcv->ReSize(sz);

    return false;
}

bool GXClientSocket::IsAccetSock() const
{
    return false;
}

void GXClientSocket::SetPrcsManager(ISocketManager *h)
{
    m_mgrPrcs = h;
}

ISocketManager *GXClientSocket::GetPrcsManager() const
{
    return m_mgrPrcs;
}

bool GXClientSocket::ResizeBuff(int sz)
{
    if (m_buffSnd)
        return m_buffSnd->ReSize(sz);

    return false;
}

bool GXClientSocket::IsNoWriteData() const
{
    return m_buffSnd && m_buffSnd->Count() == 0;
}

bool GXClientSocket::Reconnect()
{
    if (m_stat != ISocket::Closed || !m_mgrPrcs)
        return false;

    m_stat = Connecting;
    m_mgrPrcs->AddSocket(this);
    return true;
}

void GXClientSocket::SetLogin(IObjectManager *)
{
}

void GXClientSocket::SetCheckTime(int64_t)
{
}

int64_t GXClientSocket::GetCheckTime() const
{
    return -1;
}
