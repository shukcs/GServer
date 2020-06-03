#ifndef __OBJECT_BASE_H__
#define __OBJECT_BASE_H__

#include <map>
#include "LoopQueue.h"

#ifdef __GNUC__
#define PACKEDSTRUCT( _Strc ) _Strc __attribute__((packed))
#else
#define PACKEDSTRUCT( _Strc ) __pragma( pack(push, 1) ) _Strc __pragma( pack(pop) )
#endif

class SubcribeStruct;
#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif
class IObject;
class ISocket;
class IObjectManager;
class BussinessThread;
class IMutex;
class IMessage;
class ILog;
class SocketHandle;
class IObject;
class ISocketManager;

typedef LoopQueue<IMessage*> MessageQue;

class ILink
{
public:
    SHARED_DECL ILink();
    SHARED_DECL virtual ~ILink();
    SHARED_DECL virtual void SetSocket(ISocket *s);

    SHARED_DECL void OnSockClose(ISocket *s);
    SHARED_DECL virtual void CheckTimer(uint64_t ms);
    SHARED_DECL bool IsRealse();
    SHARED_DECL ISocket *GetSocket()const;
public:
    bool PrcsBussiness(uint64_t ms, BussinessThread &t);
    virtual int ProcessReceive(void *buf, int len) = 0;
    bool Receive(const void *buf, int len);
    bool IsChanged()const;
    int CopyData(void *data, int len)const;
    virtual void OnConnected(bool bConnected) = 0;
    virtual IObject *GetParObject() = 0;
    void SetMutex(IMutex *m);
    void SetThread(BussinessThread *t);
    void processSocket(ISocket *s, BussinessThread &t);
protected:
    SHARED_DECL void SetBuffSize(uint16_t sz);
    SHARED_DECL bool ChangeLogind(bool b);
    SHARED_DECL virtual void OnLogined(bool suc, ISocket *s = NULL);
    SHARED_DECL bool CanSend()const;
    SHARED_DECL int Send(const char *buf, int len); //调用需在CheckTimer中
    SHARED_DECL void ClearRecv(int n = -1);
    SHARED_DECL void Release();
    SHARED_DECL char *GetThreadBuff()const;
    SHARED_DECL int GetThreadBuffLength()const;
protected:
    int64_t                 m_tmLastInfo;
    ISocket                 *m_sock;
    LoopQueBuff             *m_recv;
    IMutex                  *m_mtx;
    BussinessThread         *m_thread;
    bool                    m_bLogined;
    bool                    m_bChanged;
    bool                    m_bRelease;
};

class IObject
{
public:
    enum TypeObject
    {
        UnKnow = -1,
        Plant,
        GroundStation,
        DBMySql,
        User,
    };
    enum ObjectStat
    {
        Uninitial,
        Initialing,
        InitialFail,
        Initialed,
    };
public:
    SHARED_DECL const std::string &GetObjectID()const;
    SHARED_DECL void SetObjectID(const std::string &id);
    SHARED_DECL IObjectManager *GetManager()const;
    SHARED_DECL bool SendMsg(IMessage *msg);
    SHARED_DECL virtual ILink *GetLink();
    SHARED_DECL bool IsInitaled()const;
    SHARED_DECL bool IsAllowRelease()const;
    virtual int GetObjectType()const = 0;
    virtual void InitObject() = 0;
public:
    virtual void ProcessMessage(IMessage *msg) = 0;
public:
    SHARED_DECL static IObjectManager *GetManagerByType(int);
    template<typename T, typename Contianer = std::list<T> >
    static bool IsContainsInList(const Contianer ls, const T &e)
    {
        for (const T &itr : ls)
        {
            if (itr == e)
                return true;
        }
        return false;
    }
protected:
    void PushReleaseMsg(IMessage *);
protected:
/*******************************************************************************************
这是个连接实体抽象类
ProcessMessage():处理对象之间消息的
ProcessReceive():处理网络数据，ret:已处理数据长度
GetObjectType():连接实体类型，返回值需要与对应的IObjectManager::GetObjectType()相同
OnConnected(bConnected):连接断开处理
IObject(sock, id)；sock:socket;id:连接实体标识
********************************************************************************************/
    SHARED_DECL IObject(const std::string &id);
    SHARED_DECL virtual ~IObject();

    SHARED_DECL void Subcribe(const std::string &sender, int tpMsg);
    SHARED_DECL void Unsubcribe(const std::string &sender, int tpMsg);
protected:
    friend class ObjectManagers;
    friend class IObjectManager;
    friend class ILink;
    std::string             m_id;
    ObjectStat              m_stInit;
};

class IObjectManager
{
protected:
    typedef std::map<std::string, IObject*> MapObjects;
    typedef std::list<std::string> StringList;
    typedef std::map<int, StringList> SubcribeList;
    typedef std::map<std::string, SubcribeList> MessageSubcribes;
    typedef LoopQueue<SubcribeStruct *> SubcribeQueue;
    typedef struct _LoginBuff
    {
        uint16_t pos;
        char buff[1024];
        void initial()
        {
            pos = 0;
        }
    }LoginBuff;
    typedef std::map<ISocket *, LoginBuff> LoginMap;
public:
    SHARED_DECL virtual ~IObjectManager();
    virtual int GetObjectType()const = 0;
    SHARED_DECL virtual void LoadConfig();
    SHARED_DECL bool AddObject(IObject *obj);
    SHARED_DECL bool SendMsg(IMessage *msg);
    SHARED_DECL void Log(int err, const std::string &obj, int evT, const char *fmt, ...);
    SHARED_DECL void Subcribe(const std::string &dsub, const std::string &sender, int tpMsg);
    SHARED_DECL void Unsubcribe(const std::string &dsub, const std::string &sender, int tpMsg);
    SHARED_DECL virtual bool IsReceiveData()const;

    void PushManagerMessage(IMessage *);
    IMessage *PopRecycleMessage();
    void PushReleaseMsg(IMessage *);
    bool ProcessBussiness(BussinessThread *t);
    void ProcessMessage();
    bool ProcessLogins(BussinessThread *s);
    MessageQue *GetSendQue(int idThread)const;
    bool ParseRequest(const char *buf, int len);
    bool Exist(IObject *obj)const;
    BussinessThread *GetThread(int id = -1)const;
    void OnSocketClose(ISocket *s);
    void AddLoginData(ISocket *s, const void *buf, int len);
public:
    SHARED_DECL static IObjectManager *MangerOfType(int type);
protected:
    SHARED_DECL IObjectManager();

    SHARED_DECL IObject *GetObjectByID(const std::string &id)const;
    SHARED_DECL virtual bool PrcsPublicMsg(const IMessage &msg);
    SHARED_DECL virtual void ToCurrntLog(int err, const std::string &obj, int evT, const std::string &dscb);
    SHARED_DECL void InitThread(uint16_t nT = 1, uint16_t bufSz = 1024);
    SHARED_DECL virtual bool IsHasReuest(const char *buf, int len)const;
    const StringList &getMessageSubcribes(IMessage *msg);
    void PrcsSubcribes();
protected:
    BussinessThread *GetPropertyThread()const;
protected:
    virtual IObject *PrcsNotObjectReceive(ISocket *s, const char *buf, int len) = 0;
protected:
    friend class ObjectManagers;
    ILog                            *m_log;
    IMutex                          *m_mtx;
    std::list<BussinessThread*>     m_lsThread;
    MapObjects                      m_objects;
    MessageQue                      m_messages;         //接收消息队列
    MessageQue                      m_lsMsgRecycle;     //消息回收队列
    MessageSubcribes                m_subcribes;        //订阅消息
    SubcribeQueue                   m_subcribeQue;
    LoginMap                        m_loginSockets;
};

#ifdef SOCKETS_NAMESPACE
}
#endif
#endif //__OBJECT_BASE_H__