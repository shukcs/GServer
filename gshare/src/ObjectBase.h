#ifndef __OBJECT_BASE_H__
#define __OBJECT_BASE_H__

#include <map>
#include <list>
#include <queue>
#include "common/LoopQueue.h"

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
class BussinessPrcs;
class IMessage;
class ObjectSignal;
class ILog;
class SocketHandle;
class IObject;
class ISocketManager;
class TiXmlElement;
class Timer;

typedef std::queue<IMessage*> MessageQue;
typedef void *(*ParsePackFuc)(const char *, uint32_t &len);
typedef uint32_t(*SerierlizePackFuc)(void *data, char *, uint32_t len);
typedef void(*RelaesePackFuc)(void *data);
class ILink
{
    enum LinkStat {
        Stat_Link = 0,
        Stat_Close = 1,
		Stat_Remove = 2,
    };
public:
    SHARED_DECL ILink();
    SHARED_DECL virtual ~ILink();

    SHARED_DECL void CloseLink();
    SHARED_DECL bool IsLinked()const;

    virtual void OnConnected(bool bConnected) = 0;
    virtual const IObject *GetParObject()const = 0;
public:
    bool OnReceive(const ISocket &s, const void *buf, int len);
    void OnSockClose(ISocket *);
    void SetMutex(std::mutex *m);
    void SetThread(BussinessPrcs *t);
    BussinessPrcs *GetThread()const;
    void processSocket(ISocket &s, BussinessPrcs &t);
    void SetSocket(ISocket *s);
	bool IsRemoveThread()const;
	ISocket *GetSocket()const;
protected:
    SHARED_DECL void SetSocketBuffSize(uint16_t sz);
    SHARED_DECL void SetBuffSize(uint16_t sz);
	SHARED_DECL bool SendData2Link(void *data, ISocket *s = NULL);
	SHARED_DECL bool IsConnect()const;
	SHARED_DECL virtual void SetLogined(bool suc, ISocket *s = NULL);
    bool IsLinkThread()const;
    virtual bool ProcessRcvPack(const void *pack) = 0;
    virtual void RefreshRcv(int64_t ms) = 0;
protected:
    int GetSendRemain()const;
    void SendData(char *buf, int len);
    int ParsePack(const char *buf, int len);
    ParsePackFuc GetParsePackFunc()const;
    RelaesePackFuc GetReleasePackFunc()const;
    SerierlizePackFuc GetSerierlizePackFuc()const;
    void AddProcessRcv(void *pack, bool bUsed = true);
    void ProcessRcvPacks(int64_t ms);
    void *PopRcv(bool bUsed = true);

	void _disconnect();
private:
    bool _adjustBuff(const char *buf, int len);
    void _cleanRcvPacks(bool bUnuse=false);
	void _clearSocket();
private:
    friend class BussinessPrcs;
    ISocket                 *m_sock;
    char                    *m_recv;
    uint16_t                m_szRcv;
    uint16_t                m_pos;
	std::mutex              *m_mtxS;
	LinkStat                m_stat;
	bool                    m_bLogined;
    BussinessPrcs         *m_thread;
    std::queue<void *>      m_packetsRcv;
    std::queue<void *>      m_packetsUsed;
};

class IObject
{
public:
    enum TypeObject
    {
        UnKnow = -1,
        Uav,
        GroundStation,
        DBMySql,
        VgFZ,
        Tracker,
        GVMgr,
        GXLink,
        Mail,
        Ntrip,
        Rtcm,
        User,
    };
    enum ObjectStat
    {
        Uninitial,
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
    SHARED_DECL virtual void CheckTimer(uint64_t ms);

    ///接收网络数据接口
    SHARED_DECL Timer *StartTimer(uint32_t ms, bool repeat=true);
    SHARED_DECL void KillTimer(Timer *t);
    SHARED_DECL void ReleaseObject();
    SHARED_DECL bool IsRelease()const;
    SHARED_DECL virtual void InitObject();
private:
    friend class ObjectManagers;
    friend class IObjectManager;
	friend class ILink;
	ObjectStat              m_stInit;
protected:
	std::string             m_id;
};

class IObjectManager
{
protected:
	typedef std::list<std::string> StringList;
    typedef IObject *(IObjectManager::*PrcsLoginHandle)(ISocket *, const void *);
    typedef std::map<std::string, IObject*> MapObjects;
    typedef std::map<ISocket*, void*> LoginMap;
private:
	typedef std::map<int, std::list<std::string>> SubcribeList;
	typedef std::map<std::string, SubcribeList> MessageSubcribes;
	typedef std::queue<SubcribeStruct *> SubcribeQueue;
public:
    SHARED_DECL virtual ~IObjectManager();
    virtual int GetObjectType()const = 0;
    virtual void LoadConfig(const TiXmlElement *root) = 0;
    SHARED_DECL bool AddObject(IObject *obj);
    SHARED_DECL void Log(int err, const std::string &obj, int evT, const char *fmt, ...);
    SHARED_DECL void Subcribe(const std::string &dsub, const std::string &sender, int tpMsg);
    SHARED_DECL void Unsubcribe(const std::string &dsub, const std::string &sender, int tpMsg);
    SHARED_DECL virtual bool IsReceiveData()const;

    BussinessPrcs *GetThread(int id = -1)const;
    BussinessPrcs *GetMainThread()const;
    void OnSocketClose(ISocket *s);
    int AddLoginData(ISocket *s, const char *buf = NULL, uint32_t len = 0);
    void PushReleaseMsg(IMessage *);
	void ObjectRelease(const IObject &obj);
public:
    SHARED_DECL static IObjectManager *MangerOfType(int type);
    SHARED_DECL static bool SendMsg(IMessage *msg);
public:
    SHARED_DECL static std::string GetObjectFlagID(const IObject *o);
protected:
    SHARED_DECL IObjectManager();

    SHARED_DECL void InitPackPrcs(SerierlizePackFuc srlz, ParsePackFuc rcv, RelaesePackFuc rls);
    SHARED_DECL void InitPrcsLogin(PrcsLoginHandle);

    SHARED_DECL IObject *GetObjectByID(const std::string &id)const;
    SHARED_DECL IObject *GetFirstObject()const;
    SHARED_DECL virtual bool PrcsPublicMsg(const IMessage &msg);
    SHARED_DECL virtual void ToCurrntLog(int err, const std::string &obj, int evT, const std::string &dscb);
    SHARED_DECL void InitThread(uint16_t nT = 1, uint16_t bufSz = 1024);
    SHARED_DECL void SetEnableTimer(bool);
    SHARED_DECL bool IsEnableTimer()const;
	virtual bool IsHasReuest(const char *, int)const { return false; }
    virtual void ProcessEvents() {};

    void Run(bool b=true);
protected:
    bool Exist(IObject *obj)const;
    const StringList &getMessageSubcribes(const IMessage *msg);
    void PrcsSubcribes();
    void SubcribesProcess(const IMessage *msg);
    BussinessPrcs *GetPropertyThread()const;
    BussinessPrcs *CurThread()const;
    void PushManagerMessage(IMessage *);
    bool ProcessBussiness();
    void ProcessMessage();
    bool ProcessLogins();
    bool OnTimerTrigger(const std::string &id, int64_t ms);
	IMessage *_popMessage();
	bool _prcsRelease(const ObjectSignal &evt);
    void _prcsMessage(const IMessage &msg);
    SubcribeStruct *_poSubcribe();
private:
    friend class ObjectManagers;
    friend class BussinessPrcs;
    friend class ILink;
    BussinessPrcs                 *m_thread;
    ILog                            *m_log;
    std::mutex                      *m_mtxLogin;
    std::mutex                      *m_mtxMsgs;
    SerierlizePackFuc               m_pfSrlzPack;
    ParsePackFuc                    m_pfParsePack;
    RelaesePackFuc                  m_pfRlsPack;
    PrcsLoginHandle                 m_pfPrcsLogin;
    std::list<BussinessPrcs*>     m_lsThread;
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