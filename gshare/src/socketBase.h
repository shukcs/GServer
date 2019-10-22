#ifndef __SOCKETBASE_H__
#define __SOCKETBASE_H__

#include <string>

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

class IMutex;
class IObject;
class ISocketManager;
class SocketAddress;

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
    //ISocketManager���ú���
    virtual SocketAddress *GetAddress()const = 0;
    virtual void SetAddress(SocketAddress *) = 0;
    virtual ISocketManager *GetManager()const = 0;
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
    virtual int CopySend(char *buff, int sz, unsigned fr=0)const = 0;
    virtual int GetSendLength()const = 0;
    virtual bool IsAccetSock()const = 0;
    virtual bool ResetSendBuff(uint16_t sz)=0;
    virtual void SetPrcsManager(ISocketManager *) = 0;

    //��������ú���
    virtual IObject *GetOwnObject()const = 0;
    virtual void SetObject(IObject *o) = 0;
    virtual bool Bind(int port, const std::string &hostLocal="") = 0;
    virtual bool ConnectTo(const std::string &hostRemote, int port) = 0;
    virtual int Send(int len, void *buff=NULL) = 0;

    //�����Ƶ��ú���
    virtual SocketStat GetSocketStat()const = 0;
    virtual bool IsListenSocket()const = 0;
    virtual void Close() = 0;
    virtual std::string GetHost()const = 0;
    virtual uint16_t GetPort()const = 0;
    virtual std::string GetObjectID()const = 0;
    virtual void SetMutex(IMutex *mtx) = 0;
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
    virtual void Log(int err, const std::string &obj, int evT, const char *fmt, ...) = 0;
    virtual void Release() = 0;
    virtual void SetBindedCB(ISocket *, FuncOnBinded) = 0;

    virtual ~ISocketManager() {}
};

#ifdef SOCKETS_NAMESPACE
}
#endif
#endif //__SOCKETBASE_H__