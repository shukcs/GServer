#include "gsocket.h"
#include "gsocketmanager.h"
#include "Ipv4Address.h"
#include "Mutex.h"
#include "ObjectBase.h"
#include "ObjectManagers.h"
#include "Lock.h"
#include <string.h>

#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE
#endif

GSocket::GSocket(ISocketManager *handle): m_manager(handle)
,m_mgrPrcs(NULL), m_object(NULL), m_fd(-1), m_bListen(false)
, m_bAccept(false), m_stat(UnConnected)
, m_address(NULL), m_mtx(NULL)
{
}

GSocket::~GSocket()
{
    delete m_address;
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
        m_buff.InitBuff(2048);
        return true;
    }
    return false;
}

int GSocket::Send(int len, void *buff)
{
    int ret = len;
    if(buff)
    {
        Lock l(m_mtx);
        ret = m_buff.Add(buff, len) ? len : 0;
    }

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
    return m_buff.Count() > 0;
}

void GSocket::EnableWrite(bool)
{
}

void GSocket::OnWrite(int len)
{
    if (len > 0)
    {
        int nTmp = m_buff.Count();
        if (nTmp>0)
        {
            m_mtx->Lock();
            m_buff.Clear(len);
            m_mtx->Unlock();

            len -= nTmp;
        }

        if (len>0 && m_object)
            m_object->SetSended(len);
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
    if (m_object)
        m_object->OnConnected(b);
}

void GSocket::OnBind()
{
    m_stat = Binded;
}

int GSocket::CopySend(char *buf, int sz, unsigned from) const
{
    unsigned len = m_buff.Count();
    int ret = 0;
    if (from < len)
    {
        char *self = (char*)m_buff.GetBuffAddress()+from;
        ret = len - from;
        if (ret > sz)
            ret = sz;
        memcpy(buf, self, ret);
    }

    if (ret < sz && m_object)
        ret += m_object->CopySend(buf + ret, sz - ret, from);

    return ret;
}

int GSocket::GetSendLength() const
{
    int ret = m_buff.Count();
    if (m_object)
        ret += m_object->GetSenLength();
    return  ret;
}

bool GSocket::ResetSendBuff(uint16_t sz)
{
    return m_buff.InitBuff(sz);
}

bool GSocket::IsAccetSock() const
{
    return m_bAccept;
}

void GSocket::SetPrcsManager(ISocketManager *h)
{
    m_mgrPrcs = h;
}

void GSocket::SetMutex(IMutex *mtx)
{
    m_mtx = mtx;
}