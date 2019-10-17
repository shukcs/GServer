#ifndef __OBJECT_MANAGERS_H__
#define __OBJECT_MANAGERS_H__

#include "stdconfig.h"
#include <list>
#include <map>

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

class IMutex;
class Thread;
class ISocket;
class IMessage;
class IObject;
class IObjectManager;
class BaseBuff;

//数据处理工厂元素抽象
class SHARED_DECL ManagerAbstractItem
{
public:
    virtual ~ManagerAbstractItem();
    void Register();
    void Unregister();
protected:
    virtual IObjectManager *CreateManager() = 0;
protected:
    int m_type;
};

template<class Item>
class ManagerItem : public ManagerAbstractItem
{
public:
    ManagerItem() : ManagerAbstractItem()
    {
        m_type = -1;
        Register();
    }
protected:
    IObjectManager *CreateManager()
    {
        return new Item;
    }
};

class ObjectManagers
{
public:
    static ObjectManagers &Instance();
public:
    bool SendMsg(IMessage *msg);
    void AddManager(IObjectManager *m);
    void RemoveManager(int type);
    void RemoveManager(const IObjectManager *m);
    IObjectManager *GetManagerByType(int tp)const;

    void Destroy(IObject *o);
    void OnSocketClose(ISocket *sock);
    void ProcessReceive(ISocket *sock, void const *buf, int len);
protected:
    bool PrcsRcvBuff();
    void PrcsCloseSocket();
    bool PrcsMangerData();
    void PrcsObjectsDestroy();
private:
    ObjectManagers();
    ~ObjectManagers();

    void _removeBuff(ISocket *sock);
private:
    friend class ManagerThread;
    std::map<int, IObjectManager*> m_managersMap;
    std::map<ISocket *, BaseBuff*> m_socksRcv;
    std::list<ISocket *>           m_keysRemove;
    std::list<IObject *>           m_objectsDestroy;
    IMutex                         *m_mtx;
    IMutex                         *m_mtxObj;
    Thread                         *m_thread;
};

#ifdef SOCKETS_NAMESPACE
}
#endif

#define DECLARE_MANAGER_ITEM(cls) ManagerItem<cls> s_manager_##cls;

#endif // !__OBJECT_MANAGERS_H__