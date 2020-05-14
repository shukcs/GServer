#ifndef __PROTECT_SOCKET_H__
#define __PROTECT_SOCKET_H__

#include "stdconfig.h"
#include "socketBase.h"

class LoopQueBuff;
#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE
#endif

class IObject;
class SocketAddress;
class ILog;

class ProtectSocket : public ISocket
{
public:
    ProtectSocket(ISocketManager *parent);
    ~ProtectSocket();

    //无限制调用函数
    bool Bind(int port, const std::string &hostLocal = "");
    bool ConnectTo(const std::string &hostRemote, int port);
    uint16_t GetPort()const;
    std::string GetHost()const;
    void Close();
protected:
    //事务处理调用函数
    ILink *GetOwnObject()const;
    void SetObject(ILink *o);
    int Send(int len, const void *buff); 
    void ClearBuff()const;

    //无限制调用函数
    SocketStat GetSocketStat()const;
    bool IsListenSocket()const;
    bool IsConnect()const;

    //GSockM
    SocketAddress *GetAddress()const;
    void SetAddress(SocketAddress *);
    int GetHandle()const;
    void SetHandle(int fd);
    ISocketManager *GetParent()const;
    bool IsReconnectable()const;
    bool IsWriteEnabled()const;
    void EnableWrite(bool);

    void OnWrite(int);
    void OnRead(void *buf, int len);
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
protected:
    ISocketManager  *m_parent;
    ISocketManager  *m_mgrPrcs;
    int             m_fd;
    bool            m_bListen;
    bool            m_bAccept;
    SocketStat      m_stat;
    SocketAddress   *m_address;
    LoopQueBuff     *m_buffSocket;
};

#endif // !__PROTECT_SOCKET_H__
