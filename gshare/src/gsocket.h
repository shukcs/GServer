#ifndef __GSOCKET_H__
#define __GSOCKET_H__

#include "stdconfig.h"
#include "socketBase.h"
#include "BaseBuff.h"

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

class GSocketHandle;
class IObject;
class SocketAddress;
class IMutex;

class GSocket : public ISocket
{
public:
/***********************************************************************
这是个Socket封装，对于服务端已近够用，客户端可以重载某些需要的函数
GSocket(handle)
handle:是创建他的线程，如果handle不为空，GSocket调用Close才能安全删除
************************************************************************/
    SHARED_DECL GSocket(ISocketManager *handle);
    SHARED_DECL ~GSocket();

    //无限制调用函数
    SHARED_DECL virtual bool Bind(int port, const std::string &hostLocal = "");
    SHARED_DECL virtual bool ConnectTo(const std::string &hostRemote, int port);
    SHARED_DECL int GetPort()const;
    SHARED_DECL std::string GetHost()const;
    SHARED_DECL void Close();
protected:
    //事务处理调用函数
    IObject *GetOwnObject()const;
    void SetObject(IObject *o);
    int Send(int len, void *buff);

    //无限制调用函数
    SocketStat GetSocketStat()const;
    bool IsListenSocket()const;
    std::string GetObjectID()const;

    //
    SocketAddress *GetAddress()const;
    void SetAddress(SocketAddress *);
    int GetHandle()const;
    void SetHandle(int fd);
    ISocketManager *GetManager()const;
    bool IsReconnectable()const;
    bool IsWriteEnabled()const;
    void EnableWrite(bool);

    SHARED_DECL virtual void OnWrite(int);
    SHARED_DECL virtual void OnRead(void *buf, int len);
    SHARED_DECL virtual void OnClose();
    SHARED_DECL virtual void OnConnect(bool);

    void OnBind();
    int CopySend(char *buf, int sz, unsigned from=0)const;
    int GetSendLength()const;
    bool ResetSendBuff(uint16_t sz);
    bool IsAccetSock()const;

    void SetPrcsManager(ISocketManager *h);
protected:
    friend class GSocketManager;
    ISocketManager  *m_manager;
    ISocketManager  *m_mgrPrcs;
    IObject         *m_object;
    int             m_fd;
    bool            m_bListen;
    bool            m_bAccept;
    SocketStat      m_stat;
    SocketAddress   *m_address;
    BaseBuff        m_buff;
    IMutex          *m_mtx;
};

#ifdef SOCKETS_NAMESPACE
}
#endif
#endif // !__GSOCKET_H__
