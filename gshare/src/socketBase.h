#ifndef __SOCKETBASE_H__
#define __SOCKETBASE_H__

#include <string>

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

class IMutex;
class ILink;
class ISocketManager;
class SocketAddress;
class ILog;

class ISocket
{
public:
    enum SocketStat
    {
        UnConnected,
        Binding,
        Binded,
        Connecting,
        Connected,
        Closing,
        Closed,
    };
public:
    virtual ~ISocket() {}
    //ISocketManager调用函数
    virtual SocketAddress *GetAddress()const = 0;
    virtual void SetAddress(SocketAddress *) = 0;
    virtual ISocketManager *GetParent()const = 0;
    virtual int GetHandle()const = 0;
    virtual void SetHandle(int fd) = 0;
    virtual bool IsWriteEnabled()const = 0;
    virtual bool IsReconnectable()const = 0;
    virtual void EnableWrite(bool) = 0;
    virtual void OnWrite(int) = 0;
    virtual void OnRead(void *buf, int len) = 0;
    virtual void OnClose() = 0;
    virtual void OnConnect(bool=true) = 0;
    virtual void OnBind(bool=true) = 0;
    virtual int CopyData(char *buff, int sz)const = 0;
    virtual int GetSendLength()const = 0;
    virtual void ClearBuff()const = 0;
    virtual bool IsAccetSock()const = 0;
    virtual bool ResetSendBuff(uint16_t sz)=0;
    virtual void SetPrcsManager(ISocketManager *) = 0;
    virtual ISocketManager *GetPrcsManager()const = 0;

    //事务处理调用函数
    virtual ILink *GetHandleLink()const = 0;
    virtual void SetHandleLink(ILink *o) = 0;
    virtual bool Bind(int port, const std::string &hostLocal="") = 0;
    virtual bool ConnectTo(const std::string &hostRemote, int port) = 0;
    virtual int Send(int len, const void *buff=NULL) = 0;
    virtual bool ResizeBuff(int sz) = 0;
    virtual bool IsNoWriteData()const = 0;
    virtual bool Reconnect() = 0;

    //无限制调用函数
    virtual SocketStat GetSocketStat()const = 0;
    virtual bool IsListenSocket()const = 0;
    virtual void Close() = 0;
    virtual std::string GetHost()const = 0;
    virtual uint16_t GetPort()const = 0;
};

class ISocketManager
{
protected:
    typedef void(*FuncOnBinded)(ISocket *sock, bool binded);
public:
    virtual bool AddSocket(ISocket *) = 0;
    virtual bool AddWaitPrcsSocket(ISocket *) = 0;
    virtual void ReleaseSocket(ISocket *) = 0;
    virtual bool IsMainManager() const = 0;
    virtual bool Poll(unsigned ms) = 0;
    virtual void AddProcessThread() = 0;
    virtual bool IsRun()const = 0;
    virtual void Exit() = 0;
    virtual void Release() = 0;
    virtual void SetBindedCB(ISocket *, FuncOnBinded) = 0;
    virtual void CloseServer() = 0;

    virtual ~ISocketManager() {}
};

#ifdef SOCKETS_NAMESPACE
}
#endif
#endif //__SOCKETBASE_H__