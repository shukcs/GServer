#ifndef __OBJECT_BASE_H__
#define __OBJECT_BASE_H__

#include <string>
#include <list>
#include <map>
#include "stdconfig.h"
#include "BaseBuff.h"

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

class IObject;
class ISocket;
class IObjectManager;
class BussinessThread;
class IMutex;
class IMessage;
class ObjectMetuxs;

typedef std::map<std::string, IObject*> ThreadObjects;
typedef std::map<int, ThreadObjects> ObjectsMap;
class IObject
{
public:
    enum TypeObject
    {
        UnKnow = -1,
        Plant,
        GroundStation,
        User,
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

    virtual int GetObjectType()const = 0;
    virtual void OnConnected(bool bConnected) = 0;
public:
    bool RcvMassage(IMessage *msg);
    void RemoveRcvMsg(IMessage *);
    void AddRelease(IMessage *);
    bool PrcsBussiness(uint64_t ms);
    int GetThreadId()const;
    void SetThreadId(int id);
public:
    SHARED_DECL virtual void SetSocket(ISocket *);
    SHARED_DECL virtual void OnSockClose(ISocket *);    //这是可以重载的，可能有多个连接

    virtual int GetSenLength()const;
    virtual int CopySend(char *buf, int sz, unsigned form = 0);
    virtual void SetSended(int sended=-1);//-1,发送完
public:
    SHARED_DECL static bool SendMsg(IMessage *msg);
    SHARED_DECL static IObjectManager *GetManagerByType(int);
public:
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
    virtual void ProcessMassage(const IMessage &msg) = 0;
    virtual int ProcessReceive(void *buf, int len) = 0;
    SHARED_DECL virtual void CheckTimer(uint64_t ms);
protected:
/*******************************************************************************************
这是个连接实体抽象类
ProcessMassage():处理对象之间消息的
ProcessReceive():处理网络数据，ret:已处理数据长度
GetObjectType():连接实体类型，返回值需要与对应的IObjectManager::GetObjectType()相同
OnConnected(bConnected):连接断开处理
IObject(sock, id)；sock:socket;id:连接实体标识
********************************************************************************************/
    SHARED_DECL IObject(ISocket *sock, const std::string &id);
    SHARED_DECL virtual ~IObject();
protected:
    friend class ObjectManagers;
    friend class IObjectManager;
    ISocket                 *m_sock;
    std::string             m_id;
    bool                    m_bRelease;
    BaseBuff                m_buff;
    IMutex                  *m_mtx;
    IMutex                  *m_mtxMsg;
    std::list<IMessage*>    m_lsMsg;            //接收消息队列
    std::list<IMessage*>    m_lsMsgRelease;     //消息释放队列
    int                     m_idThread;
};

class IObjectManager
{
public:
    virtual int GetObjectType()const = 0;
    SHARED_DECL virtual void LoadConfig();
    SHARED_DECL bool AddObject(IObject *obj);
    SHARED_DECL IObject *GetObjectByID(const std::string &id)const;
    SHARED_DECL bool Receive(ISocket *s, const BaseBuff &buff, int &prcs);
    void ReceiveMessage(IMessage *);
    void RemoveRcvMsg(IMessage *);
    void AddRelease(IMessage *);
    bool HasIndependThread()const;
    bool ProcessBussiness();
    bool PrcsObjectsOfThread(int nThread);
    bool Exist(IObject *obj)const;
public:
    SHARED_DECL static bool SendMsg(IMessage *msg);
protected:
    SHARED_DECL IObjectManager(uint16_t nThread = 1);
    SHARED_DECL virtual ~IObjectManager();

    SHARED_DECL virtual bool PrcsRemainMsg(const IMessage &msg);
    SHARED_DECL void InitThread(uint16_t nThread = 1);

    void ProcessMassage(const IMessage &msg);
    int GetPropertyThread()const;
    void AddMessage(IMessage *msg);
    void PrcsReleaseMsg();
    void removeObject(ThreadObjects &objs, const std::string &id);
protected:
    virtual IObject *PrcsReceiveByMgr(ISocket *s, const char *buf, int &len) = 0;
private:
    friend class ObjectManagers;
    IMutex                          *m_mtx;
    std::list<IMessage*>            m_lsMsg;            //接收消息队列
    std::list<IMessage*>            m_lsMsgRelease;     //消息释放队列
    std::list<BussinessThread*>     m_lsThread;
    ObjectsMap                      m_mapThreadObject;
    std::map<int, ObjectMetuxs*>    m_threadMutexts;
};

#ifdef SOCKETS_NAMESPACE
}
#endif
#endif //__OBJECT_BASE_H__