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
class LoopQueBuff;
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
        return new Item;
    }
};

class ObjectManagers
{
    typedef std::pair<int, std::string> ObjectDsc;
    typedef LoopQueue<IObject *> ObjectQueue;
    typedef std::list<ObjectDsc> SubcribeList;
    typedef LoopQueue<ISocket *> SocketQue;
    typedef std::map<int, SubcribeList> SubcribeMap;
    typedef std::map<std::string, SubcribeMap> MessageSubcribes;
    typedef LoopQueue<SubcribeStruct *> SubcribeQueue;
    typedef LoopQueue<IMessage *> MessageQueue;
public:
    static ObjectManagers &Instance();
    static ILog &GetLog();
public:
    bool SendMsg(IMessage *msg);
    void AddManager(IObjectManager *m);
    void RemoveManager(int type);
    void RemoveManager(const IObjectManager *m);
    IObjectManager *GetManagerByType(int tp)const;

    void Destroy(IObject *o);
    void OnSocketClose(ISocket *sock);
    void ProcessReceive(ISocket *sock, void const *buf, int len);
    void Subcribe(IObject *o, const std::string &sender, int tpMsg);
    void Unsubcribe(IObject *o, const std::string &sender, int tpMsg);
    void DestroyMessage(IMessage *msg);
protected:
    bool PrcsRcvBuff();
    void PrcsCloseSocket();
    void PrcsObjectsDestroy();
    void PrcsSubcribes();
    void PrcsMessages();
protected:
    SubcribeList &getMessageSubcribes(IMessage *msg);
private:
    ObjectManagers();
    ~ObjectManagers();

    void _removeBuff(ISocket *sock);
    SubcribeList &_getSubcribes(const std::string &sender, int tpMsg);
private:
    friend class ManagerThread;
    std::map<int, IObjectManager*>      m_managersMap;
    std::map<ISocket*, LoopQueBuff*>    m_socksRcv;
    SocketQue                           m_keysRemove;
    ObjectQueue                         m_objectsDestroy;//销毁队列
    IMutex                              *m_mtxSock;
    IMutex                              *m_mtxObj;
    IMutex                              *m_mtxMsg;
    Thread                              *m_thread;
    MessageSubcribes                    m_subcribes;
    SubcribeQueue                       m_subcribeMsgs;
    MessageQueue                        m_messages;
    MessageQueue                        m_releaseMsgs;
    char                                m_buff[1024];
};

#ifdef SOCKETS_NAMESPACE
}
#endif

#define DECLARE_MANAGER_ITEM(cls) ManagerItem<cls> s_manager_##cls;

#endif // !__OBJECT_MANAGERS_H__