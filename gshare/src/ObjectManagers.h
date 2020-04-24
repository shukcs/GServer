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
    typedef std::pair<int, std::string> ObjectDsc;
    typedef LoopQueue<IObject *> ObjectQueue;
    typedef std::list<ObjectDsc> SubcribeList;
    typedef std::map<int, SubcribeList> SubcribeMap;
    typedef std::map<std::string, SubcribeMap> MessageSubcribes;
    typedef LoopQueue<SubcribeStruct *> SubcribeQueue;
    typedef std::map<ISocket*, LoopQueBuff*> MapBuffRecieve;
    typedef LoopQueue<int> RemoveManagerQueue;
public:
    static ObjectManagers &Instance();
    static ILog &GetLog();
public:
    void AddManager(IObjectManager *m);
    void RemoveManager(int type);
    IObjectManager *GetManagerByType(int tp)const;

    void Destroy(IObject *o);
    void OnSocketClose(ISocket *sock);
    void ProcessReceive(ISocket *sock, void const *buf, int len);
    void Subcribe(IObject *o, const std::string &sender, int tpMsg);
    void Unsubcribe(IObject *o, const std::string &sender, int tpMsg);
protected:
    bool PrcsRcvBuff();
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
    void _prcsSendMessages(IObjectManager *mgr);
    void _prcsReleaseMessages(IObjectManager *mgr);
private:
    friend class ManagerThread;
    std::map<int, IObjectManager*>      m_managersMap;
	MapBuffRecieve						m_socksRcv;
    ObjectQueue                         m_objectsDestroy;//销毁队列
    IMutex                              *m_mtxSock;
    IMutex                              *m_mtxObj;
    IMutex                              *m_mtxMsg;
    Thread                              *m_thread;
    MessageSubcribes                    m_subcribes;
    RemoveManagerQueue                  m_mgrsRemove;
    SubcribeQueue                       m_subcribeMsgs;
    char                                m_buff[1024];
};

#ifdef SOCKETS_NAMESPACE
}
#endif

#define DECLARE_MANAGER_ITEM(cls) ManagerItem<cls> s_manager_##cls;

#endif // !__OBJECT_MANAGERS_H__