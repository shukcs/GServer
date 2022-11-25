#ifndef  __GX_CLINENT_H__
#define __GX_CLINENT_H__

#include "ObjectBase.h"

namespace google {
    namespace protobuf {
        class Message;
    }
}

namespace das {
    namespace proto {
        class AckUavIdentityAuthentication;
        class AckOperationInformation;
    }
}

#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif

class ISocketManager;
class GXThread;
class Tracker2GXMessage;
class ProtoMsg;
class GXClientSocket;

class GXClient
{
public:
    enum GX_Stat
    {
        St_Unknow,
        St_Connect,
        St_Authing,
        St_AuthFail,
        St_Authed,
        St_Changed = 0x0100,
    };
public:
    GXClient(const std::string &id, GX_Stat st=St_Unknow);
    ~GXClient();

    const std::string &GetID()const;
    void SetStat(GX_Stat st);
    GX_Stat GetStat()const;
    bool IsChanged()const;
    void ClearChanged();
public:
    static int GXClientType();
private:
    std::string     m_id;
    int             m_stat;
};

class GXManager : public IObjectManager
{
    typedef std::pair<std::string, GXClient::GX_Stat> GXEvent;
    typedef std::list<GXEvent> EventsQue;
    typedef std::list<google::protobuf::Message *> ProtoSendQue;
    typedef std::map<std::string, GXClient*> GXClinetMap;
public:
    GXManager();
    ~GXManager();

    char *GetBuff();
    int BuffLength()const;
    ISocketManager *GetSocketManager()const;
    void PushEvent(const std::string &id, GXClient::GX_Stat st);
    void OnRead(GXClientSocket &s);
    void OnConnect(GXClientSocket *s, bool b);
protected:
    int GetObjectType()const;
    bool PrcsPublicMsg(const IMessage &msg);
    IObject *PrcsNotObjectReceive(ISocket *s, const char *buf, int len);
    void PrcsProtoBuff(const google::protobuf::Message *);
    void LoadConfig(const TiXmlElement *root);
    bool IsReceiveData()const;

    void ProcessLogin(const std::string &id, bool bLogin);
    void ProcessPostInfo(const Tracker2GXMessage &msg);
    void ProcessEvents();
private:
    void _prcsLoginAck(const das::proto::AckUavIdentityAuthentication *prt);
    void _prcsInformationAck(const das::proto::AckOperationInformation *prt);
    void prepareSocket(GXClient *o);
    void uavLoginGx();
    void sendPositionInfo();
    bool sendUavLogin(const std::string &i);
    void checkSocket(uint64_t ms);

    static bool SendProtoBuffTo(ISocket *s, const google::protobuf::Message &msg);
private:
    ISocketManager              *m_sockMgr;
    GXThread                    *m_thread;
    uint64_t                    m_tmCheck;
    uint32_t                    m_seq;
    std::mutex                  *m_mtx;
    char                        m_bufPublic[1024];
    EventsQue                   m_events;
    std::list<GXClientSocket*>  m_sockets;
    GXClinetMap                 m_objects;
    ProtoSendQue                m_protoSnds;
};

#endif//__GX_CLINENT_H__

