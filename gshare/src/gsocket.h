#ifndef __GSOCKET_H__
#define __GSOCKET_H__

#include "stdconfig.h"
#include "socketBase.h"

class LoopQueBuff;
#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

class ILink;
class SocketAddress;
class ILog;

class GSocket : public ISocket
{
public:
/***********************************************************************
这是个Socket封装，对于服务端已近够用，客户端可以重载某些需要的函数
GSocket(handle)
handle:是创建他的线程，如果handle不为空，GSocket调用Close才能安全删除
************************************************************************/
    SHARED_DECL GSocket(ISocketManager *parent);
    SHARED_DECL ~GSocket();

    //无限制调用函数
    SHARED_DECL bool Bind(int port, const std::string &hostLocal = "");
    SHARED_DECL bool ConnectTo(const std::string &hostRemote, int port);
    uint16_t GetPort()const;
    std::string GetHost()const;
    void Close();
    bool Reconnect();
public:
     static SHARED_DECL ILog &GetLog();
protected:
    //事务处理调用函数
    ILink *GetHandleLink()const;
    void SetHandleLink(ILink *o);
    int Send(int len, const void *buff); 
    void ClearBuff()const;

    //无限制调用函数
    SocketStat GetSocketStat()const;
    bool IsListenSocket()const;
    bool IsConnect()const;

    //GSockM
    SocketAddress *GetAddress()const;
    void SetAddress(SocketAddress *);
    int GetSocketHandle()const;
    void SetSocketHandle(int fd);
    ISocketManager *GetParent()const;
    bool IsReconnectable()const;
    bool IsWriteEnabled()const;
    void EnableWrite(bool);

    void OnWrite(int);
    void OnRead(const void *buf, int len);
    void OnClose();
    void OnConnect(bool);

    void OnBind(bool);
    int CopyData(char *buf, int sz)const;
    int GetSendLength()const;
    bool ResetSendBuff(uint16_t sz);
    bool IsAccetSock()const;

    void SetPrcsManager(ISocketManager *h);
    ISocketManager *GetPrcsManager()const;
    bool ResizeBuff(int sz);
    bool IsNoWriteData()const;
    void SetLogin(IObjectManager *mgr);
    void SetCheckTime(int64_t);
    int64_t GetCheckTime()const;
protected:
    friend class GSocketManager;
    ISocketManager  *m_parent;
    ISocketManager  *m_mgrPrcs;
    ILink           *m_object;
    int             m_fd;
    bool            m_bListen;
    bool            m_bAccept;
    SocketStat      m_stat;
    SocketAddress   *m_address;
    LoopQueBuff     *m_buffSocket;
    IObjectManager  *m_mgrLogin;
    int64_t         m_checkTm;
};

#ifdef SOCKETS_NAMESPACE
}
#endif
#endif // !__GSOCKET_H__
