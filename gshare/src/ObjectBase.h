﻿#ifndef __OBJECT_BASE_H__
#define __OBJECT_BASE_H__

#include <map>
#include <list>
#include <queue>
#include <functional>
#include "LoopQueue.h"

#ifdef __GNUC__
#define PACKEDSTRUCT( _Strc ) _Strc __attribute__((packed))
#else
#define PACKEDSTRUCT( _Strc ) __pragma( pack(push, 1) ) _Strc __pragma( pack(pop) )
#endif

namespace std {
    class mutex;
}
class SubcribeStruct;
#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif
class IObject;
class ISocket;
class IObjectManager;
class BussinessThread;
class IMessage;
class ILog;
class SocketHandle;
class IObject;
class ISocketManager;
class TiXmlElement;

typedef std::queue<IMessage*> MessageQue;
typedef std::function<int(char *, int len)> SerierlizeDataFuc;
typedef std::function<void(void)> ProcessDataFuc;
class ILink
{
    enum LinkStat {
        Stat_Link = 0,
        Stat_Close = 1,
        Stat_Release = Stat_Close << 1,
        Stat_CloseAndRealese,
    };
public:
    SHARED_DECL ILink();
    SHARED_DECL virtual ~ILink();

    SHARED_DECL void OnSockClose(ISocket *s);
    SHARED_DECL virtual void CheckTimer(uint64_t ms);
    SHARED_DECL bool IsRealse();
    SHARED_DECL ISocket *GetSocket()const;
    SHARED_DECL void CloseLink();
public:
    bool PrcsBussiness(uint64_t ms, BussinessThread &t);
    virtual int ProcessReceive(void *buf, int len, uint64_t) = 0;
    bool Receive(const void *buf, int len);
    bool IsChanged()const;
    int CopyData(void *data, int len)const;
    virtual void OnConnected(bool bConnected) = 0;
    virtual IObject *GetParObject() = 0;
    void SetMutex(std::mutex *m);
    void SetThread(BussinessThread *t);
    BussinessThread *GetThread()const;
    void processSocket(ISocket &s, BussinessThread &t);
protected:
    SHARED_DECL void SetSocketBuffSize(uint16_t sz);
    SHARED_DECL void SetBuffSize(uint16_t sz);
    SHARED_DECL bool IsConnect()const;
    SHARED_DECL bool ChangeLogind(bool b);
    SHARED_DECL virtual void OnLogined(bool suc, ISocket *s = NULL);
    SHARED_DECL void ClearRecv(int n = -1);
    SHARED_DECL void Release();
    SHARED_DECL bool IsLinkThread()const;
    virtual void FreshLogin(uint64_t ms) = 0;
protected:
    void SetSocket(ISocket *s);
    bool IsLinked()const;
    int GetSendRemain()const;
    void SendData(char *buf, int len);
protected:
    bool                    m_bLogined;
private:
    friend class BussinessThread;
    ISocket                 *m_sock;
    LoopQueBuff             *m_recv;
    std::mutex              *m_mtxS;
    BussinessThread         *m_thread;
    bool                    m_bChanged;
    uint32_t                m_Stat;
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
        VgFZ,
        Tracker,
        GVMgr,
        GXLink,
        Mail,
        User,
    };
    enum ObjectStat
    {
        Uninitial,
        Initialing,
        InitialFail,
        Initialed,
        ReleaseLater,
    };
    enum AuthorizeType
    {
        Type_Common = 1,
        Type_Manager = Type_Common << 1,
        Type_Admin = Type_Manager << 1,
        Type_ALL = Type_Common | Type_Manager | Type_Admin,

        Type_ReqNewUser = Type_Admin << 1,
        Type_GetPswd = Type_ReqNewUser << 1,
    };
public:
    SHARED_DECL const std::string &GetObjectID()const;
    SHARED_DECL void SetObjectID(const std::string &id);
    SHARED_DECL IObjectManager *GetManager()const;
    SHARED_DECL virtual ILink *GetLink();
    SHARED_DECL bool IsInitaled()const;
    SHARED_DECL virtual bool IsAllowRelease()const;
    virtual int GetObjectType()const = 0;
    virtual void InitObject() = 0;
public:
    virtual void ProcessMessage(const IMessage *msg) = 0;
public:
    SHARED_DECL static IObjectManager *GetManagerByType(int);
    SHARED_DECL static bool SendMsg(IMessage *msg);
protected:
    void PushReleaseMsg(IMessage *);
protected:
/*******************************************************************************************
这是个连接实体抽象类
ProcessMessage():处理对象之间消息的
ProcessReceive():处理网络数据，ret:已处理数据长度
GetObjectType():连接实体类型，返回值需要与对应的IObjectManager::GetObjectType()相同
OnConnected(bConnected):连接断开处理
IObject(id) id:连接实体标识
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
    typedef std::queue<SubcribeStruct *> SubcribeQueue;
    typedef struct _LoginBuff {
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
    virtual void LoadConfig(const TiXmlElement *root) = 0;
    SHARED_DECL bool AddObject(IObject *obj);
    SHARED_DECL void Log(int err, const std::string &obj, int evT, const char *fmt, ...);
    SHARED_DECL void Subcribe(const std::string &dsub, const std::string &sender, int tpMsg);
    SHARED_DECL void Unsubcribe(const std::string &dsub, const std::string &sender, int tpMsg);
    SHARED_DECL bool SendData2Link(const std::string &id, const SerierlizeDataFuc &pfSrlz, const ProcessDataFuc &pfPrc);
    SHARED_DECL virtual bool IsReceiveData()const;

    void PushManagerMessage(IMessage *);
    void PushReleaseMsg(IMessage *);
    bool ProcessBussiness(BussinessThread *t);
    void ProcessMessage();
    bool ProcessLogins(BussinessThread *s);
    BussinessThread *GetThread(int id = -1)const;
    void OnSocketClose(ISocket *s);
    void AddLoginData(ISocket *s, const void *buf, int len);
public:
    SHARED_DECL static IObjectManager *MangerOfType(int type);
    SHARED_DECL static bool SendMsg(IMessage *msg);
public:
    SHARED_DECL static std::string GetObjectFlagID(IObject *o);
protected:
    SHARED_DECL IObjectManager();

    SHARED_DECL IObject *GetObjectByID(const std::string &id)const;
    SHARED_DECL IObject *GetFirstObject()const;
    SHARED_DECL virtual bool PrcsPublicMsg(const IMessage &msg);
    SHARED_DECL virtual void ToCurrntLog(int err, const std::string &obj, int evT, const std::string &dscb);
    SHARED_DECL void InitThread(uint16_t nT = 1, uint16_t bufSz = 1024);
    SHARED_DECL virtual bool IsHasReuest(const char *buf, int len)const;
    SHARED_DECL virtual void ProcessEvents();
    SHARED_DECL bool Exist(IObject *obj)const;
    const StringList &getMessageSubcribes(IMessage *msg);
    void PrcsSubcribes();
    void SubcribesProcess(IMessage *msg);
protected:
    BussinessThread *GetPropertyThread()const;
    BussinessThread *CurThread()const;
protected:
    virtual IObject *PrcsNotObjectReceive(ISocket *s, const char *buf, int len) = 0;
private:
    friend class ObjectManagers;
    friend class BussinessThread;
    ILog                            *m_log;
    std::mutex                      *m_mtxLogin;
    std::mutex                      *m_mtxMsgs;
    std::list<BussinessThread*>     m_lsThread;
    MapObjects                      m_objects;
    MessageQue                      m_messages;         //接收消息队列
    MessageSubcribes                m_subcribes;        //订阅消息
    SubcribeQueue                   m_subcribeQue;
    LoginMap                        m_loginSockets;
};

#ifdef SOCKETS_NAMESPACE
}
#endif
#endif //__OBJECT_BASE_H__