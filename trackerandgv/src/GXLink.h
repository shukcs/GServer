#ifndef __GX_LINK_H__
#define __GX_LINK_H__

#include "socketBase.h"

class LoopQueBuff;
#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE
#endif

class ObjectGXClinet;
class SocketAddress;
class ILog;

class GXClientSocket : public ISocket
{
public:
    GXClientSocket(ObjectGXClinet *gx);
    ~GXClientSocket();

    //无限制调用函数
    bool Bind(int port, const std::string &hostLocal = "");
    bool ConnectTo(const std::string &hostRemote, int port);
    uint16_t GetPort()const;
    std::string GetHost()const;
    void Close();
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

    bool Reconnect();
protected:
    ObjectGXClinet  *m_parent;
    ISocketManager  *m_mgrPrcs;
    int             m_fd;
    SocketStat      m_stat;
    SocketAddress   *m_address;
    LoopQueBuff     *m_buffSocket;
};

#endif // !__GX_LINK_H__
