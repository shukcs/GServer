#ifndef __GSOCKET_H__
#define __GSOCKET_H__

#include "stdconfig.h"
#include "socketBase.h"

class LoopQueBuff;
#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

class GSocketHandle;
class IObject;
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
    SHARED_DECL GSocket(ISocketManager *handle);
    SHARED_DECL ~GSocket();

    //无限制调用函数
    SHARED_DECL virtual bool Bind(int port, const std::string &hostLocal = "");
    SHARED_DECL virtual bool ConnectTo(const std::string &hostRemote, int port);
    uint16_t GetPort()const;
    std::string GetHost()const;
    void Close();
public:
     static SHARED_DECL ILog &GetLog();
protected:
    //事务处理调用函数
    IObject *GetOwnObject()const;
    void SetObject(IObject *o);
    int Send(int len, const void *buff);

    //无限制调用函数
    SocketStat GetSocketStat()const;
    bool IsListenSocket()const;
    std::string GetObjectID()const;
    bool IsConnect()const;

    //GSockM
    SocketAddress *GetAddress()const;
    void SetAddress(SocketAddress *);
    int GetHandle()const;
    void SetHandle(int fd);
    ISocketManager *GetManager()const;
    bool IsReconnectable()const;
    bool IsWriteEnabled()const;
    void EnableWrite(bool);

    void OnWrite(int);
    void OnRead(void *buf, int len);
    void OnClose();
    void OnConnect(bool);

    void OnBind(bool);
    int CopySend(char *buf, int sz)const;
    int GetSendLength()const;
    bool ResetSendBuff(uint16_t sz);
    bool IsAccetSock()const;

    void SetPrcsManager(ISocketManager *h);
    bool ResizeBuff(int sz);
    bool IsNoWriteData()const;
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
    LoopQueBuff     *m_buffSocket;
};

#ifdef SOCKETS_NAMESPACE
}
#endif
#endif // !__GSOCKET_H__
