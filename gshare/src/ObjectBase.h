#ifndef __OBJECT_BASE_H__
#define __OBJECT_BASE_H__

#include "LoopQueue.h"
#include <map>

class LoopQueBuff;
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
typedef LoopQueue<IMessage*> MessageQue;
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
        Initialed,
        InitialFail,
    };
public:
    SHARED_DECL const std::string &GetObjectID()const;
    SHARED_DECL void SetObjectID(const std::string &id);
    SHARED_DECL ISocket *GetSocket()const;
    SHARED_DECL void Release();
    SHARED_DECL bool IsRealse();
    SHARED_DECL void SetBuffSize(uint16_t sz);
    SHARED_DECL IObjectManager *GetManager()const;
    SHARED_DECL bool Receive(const void *buf, int len);
    SHARED_DECL virtual void SetSocket(ISocket *);
    SHARED_DECL virtual void OnSockClose(ISocket *);    //这是可以重载的，可能有多个连接
    SHARED_DECL void Subcribe(const std::string &sender, int msg);
    SHARED_DECL void Unsubcribe(const std::string &sender, int msg);
    virtual int GetObjectType()const = 0;
    virtual void OnConnected(bool bConnected) = 0;
public:
    bool PushMessage(IMessage *msg);
    void PushReleaseMsg(IMessage *);
    bool PrcsBussiness(uint64_t ms);
    BussinessThread *GetThread()const;
    void SetThread(BussinessThread *t);
public:
    SHARED_DECL static bool SendMsg(IMessage *msg);
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
    virtual void ProcessMessage(IMessage *msg) = 0;
    virtual int ProcessReceive(void *buf, int len) = 0;
    virtual void InitObject() = 0;
    SHARED_DECL virtual void CheckTimer(uint64_t ms);

    SHARED_DECL void *getThreadBuff()const;
    SHARED_DECL int getThreadBuffLen()const;
    SHARED_DECL void ClearRead();
protected:
/*******************************************************************************************
这是个连接实体抽象类
ProcessMessage():处理对象之间消息的
ProcessReceive():处理网络数据，ret:已处理数据长度
GetObjectType():连接实体类型，返回值需要与对应的IObjectManager::GetObjectType()相同
OnConnected(bConnected):连接断开处理
IObject(sock, id)；sock:socket;id:连接实体标识
********************************************************************************************/
    SHARED_DECL IObject(ISocket *sock, const std::string &id);
    SHARED_DECL virtual ~IObject();
private:
    void _prcsMessage();
protected:
    friend class ObjectManagers;
    friend class IObjectManager;
    int64_t                 m_tmLastInfo;
    ISocket                 *m_sock;
    std::string             m_id;
    bool                    m_bRelease;
    ObjectStat              m_stInit;
    LoopQueBuff             *m_buff;
    MessageQue              m_lsMsg;            //接收消息队列
    MessageQue              m_lsMsgRelease;     //消息释放队列
    BussinessThread         *m_thread;
};

class IObjectManager
{
    typedef std::map<std::string, IObject*> ThreadObjects;
    typedef std::map<BussinessThread *, ThreadObjects> ObjectsMap;
public:
    SHARED_DECL virtual ~IObjectManager();
    virtual int GetObjectType()const = 0;
    SHARED_DECL virtual void LoadConfig();
    SHARED_DECL bool AddObject(IObject *obj);
    SHARED_DECL IObject *GetObjectByID(const std::string &id)const;
    SHARED_DECL bool Receive(ISocket *s, int len, const char *buf);
    SHARED_DECL void SetLog(ILog *);
    SHARED_DECL void Log(int err, const std::string &obj, int evT, const char *fmt, ...);

    void PushMessage(IMessage *);
    void PushReleaseMsg(IMessage *);
    bool ProcessBussiness(BussinessThread *t);
    bool PrcsObjectsOfThread(BussinessThread &t);
    bool Exist(IObject *obj)const;
public:
    SHARED_DECL static bool SendMsg(IMessage *msg);
protected:
    SHARED_DECL IObjectManager();

    SHARED_DECL virtual bool PrcsPublicMsg(const IMessage &msg);
    SHARED_DECL void InitThread(uint16_t nT = 1, uint16_t bufSz = 1024);
    virtual bool InitManager() = 0;
protected:
    void ProcessMessage(IMessage *msg);
    BussinessThread *GetPropertyThread()const;
    void PrcsReleaseMsg();
    void removeObject(ThreadObjects &objs, const std::string &id);
protected:
    virtual IObject *PrcsNotObjectReceive(ISocket *s, const char *buf, int len) = 0;
private:
    friend class ObjectManagers;
    IMutex                          *m_mtx;
    ILog                            *m_log;
    MessageQue                      m_lsMsg;            //接收消息队列
    MessageQue                      m_lsMsgRelease;     //消息释放队列
    std::list<BussinessThread*>     m_lsThread;
    ObjectsMap                      m_mapThreadObject;
};

#ifdef SOCKETS_NAMESPACE
}
#endif
#endif //__OBJECT_BASE_H__