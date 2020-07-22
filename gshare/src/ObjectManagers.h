#ifndef __OBJECT_MANAGERS_H__
#define __OBJECT_MANAGERS_H__

#include "LoopQueue.h"
#include <map>

class SubcribeStruct;
#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

class IMutex;
class Thread;
class ISocket;
class IMessage;
class IObject;
class IObjectManager;
class ILog;

//数据处理工厂元素抽象
class SHARED_DECL ManagerAbstractItem
{
public:
    ManagerAbstractItem();
    virtual ~ManagerAbstractItem();
    void Register();
    void Unregister();
protected:
    virtual IObjectManager *CreateManager() = 0;
protected:
    int             m_type;
    IObjectManager  *m_mgr;
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
        if (!m_mgr)
            m_mgr = new Item;
        return m_mgr;
    }
};

class ObjectManagers
{
    typedef std::map<ISocket*, LoopQueBuff*> MapBuffRecieve;
public:
    static ObjectManagers &Instance();
    static ILog &GetLog();
    static bool SndMessage(IMessage *msg);
    static bool RlsMessage(IMessage *msg);
public:
    void AddManager(IObjectManager *m);
    void RemoveManager(int type);
    IObjectManager *GetManagerByType(int tp)const;

    void ProcessReceive(ISocket *sock, void const *buf, int len);
protected:
    void PrcsMessages();
private:
    ObjectManagers();
    ~ObjectManagers();
private:
    std::map<int, IObjectManager*>      m_managersMap;
    char                                m_buff[1024];
};

#ifdef SOCKETS_NAMESPACE
}
#endif

#define DECLARE_MANAGER_ITEM(cls) ManagerItem<cls> s_manager_##cls;

#endif // !__OBJECT_MANAGERS_H__