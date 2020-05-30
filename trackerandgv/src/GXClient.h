#ifndef  __GX_CLINENT_H__
#define __GX_CLINENT_H__

#include "ObjectBase.h"

namespace google {
    namespace protobuf {
        class Message;
    }
}

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

class ISocketManager;
class GXLinkThread;
class TrackerMessage;
class IMutex;
class ProtoMsg;

class ObjectGXClinet : public IObject
{
public:
    enum GXLink_Stat
    {
        St_Unconnect,
        St_Authing,
        St_AuthFail,
        St_Authed,
        St_AcceptPos,
    };
public:
    ObjectGXClinet(const std::string &id);
    ~ObjectGXClinet();

    void OnRead(ISocket *s, const void *buff, int len);
    void Login(bool b);
    void OnConnect(bool b);
    void Send(const google::protobuf::Message &msg);
    void SetMutex(IMutex *mtx);
public:
    static int GXClinetType();
protected:
    int GetObjectType()const;
    void InitObject();

    void ProcessMessage(IMessage *msg);
    void _prcsRcv();
private:
    ISocket     *m_gxClient;
    LoopQueBuff m_buff;
    int         m_seq;
    bool        m_bConnect;
    IMutex      *m_mtx;
    GXLink_Stat m_stat;
    ProtoMsg    *m_p;
};

class GXClinetManager : public IObjectManager
{
public:
    GXClinetManager();
    ~GXClinetManager();

    char *GetBuff();
    int BuffLength()const;
    ISocketManager *GetSocketManager()const;
protected:
    int GetObjectType()const;
    bool PrcsPublicMsg(const IMessage &msg);
    IObject *PrcsNotObjectReceive(ISocket *s, const char *buf, int len);
    void LoadConfig();
    bool IsReceiveData()const;

    void ProcessLogin(const std::string sender, int sendTy, bool bLogin);
    void ProcessPostInfo(const TrackerMessage &msg);
private:
    ISocketManager  *m_sockMgr;
    GXLinkThread    *m_thread;
    char            m_bufPublic[1024];
};

#ifdef SOCKETS_NAMESPACE
}
#endif
#endif//__GX_CLINENT_H__

