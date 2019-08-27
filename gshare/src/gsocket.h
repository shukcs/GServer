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
���Ǹ�Socket��װ�����ڷ�����ѽ����ã��ͻ��˿�������ĳЩ��Ҫ�ĺ���
GSocket(handle)
handle:�Ǵ��������̣߳����handle��Ϊ�գ�GSocket����Close���ܰ�ȫɾ��
************************************************************************/
    SHARED_DECL GSocket(ISocketManager *handle);
    SHARED_DECL ~GSocket();

    //�����Ƶ��ú���
    SHARED_DECL virtual bool Bind(int port, const std::string &hostLocal = "");
    SHARED_DECL virtual bool ConnectTo(const std::string &hostRemote, int port);
    SHARED_DECL int GetPort()const;
    SHARED_DECL std::string GetHost()const;
    SHARED_DECL void Close();
protected:
    //��������ú���
    IObject *GetOwnObject()const;
    void SetObject(IObject *o);
    int Send(int len, void *buff);

    //�����Ƶ��ú���
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
