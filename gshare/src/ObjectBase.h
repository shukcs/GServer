#ifndef __COMMON_H__
#define __COMMON_H__

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

/*************************************************************************
���Ǹ�����ʵ��֮��ͨѶ��Ϣ�����࣬ObjectManagers::SendMsg()���͵�����ʵ��
IMessage(sender, id, rcv, tpMsg)
sender:���Ͷ��󣬿�����IObject��IObjectManager,����������һ���߳�
id:����IObject ID�����Կգ��մ�����ն�����IObjectManager
rcv:���ն�������
tpMsg:��Ϣ����
**************************************************************************/
class IMessage
{
public:
    SHARED_DECL IMessage(IObject *sender, const std::string &id, int rcv, int tpMsg);

    virtual ~IMessage() {}
    virtual void *GetContent()const = 0;
    virtual int GetContentLength()const = 0;

    SHARED_DECL int GetReceiverType()const;
    SHARED_DECL int GetMessgeType()const;
    SHARED_DECL const std::string &GetReceiverID()const;
    SHARED_DECL void SetSenderType(int tp);
    SHARED_DECL int GetSenderType()const;
    SHARED_DECL const std::string &GetSenderID()const;
    SHARED_DECL IObject *GetSender()const;

    SHARED_DECL bool IsValid()const;
    SHARED_DECL void Release();
protected:
    std::string m_idRcv;
    int         m_tpRcv;
    int         m_tpMsg;
    int         m_tpSender;
    IObject     *m_sender;
    bool        m_bRelease;
};

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
    SHARED_DECL bool SendMsg(IMessage *msg);
    SHARED_DECL bool Receive(const void *buf, int len);
    bool RcvMassage(IMessage *msg);
    void RemoveRcvMsg(IMessage *);
    void AddRelease(IMessage *);
    bool PrcsBussiness();
    int GetThreadId()const;
    void SetThreadId(int id);
public:
    virtual int GetObjectType()const = 0;
    virtual void OnConnected(bool bConnected) = 0;
    SHARED_DECL virtual void OnClose(ISocket *);    //���ǿ������صģ������ж������
    SHARED_DECL virtual void SetSocket(ISocket *);
    SHARED_DECL virtual int GetSenLength()const;
    SHARED_DECL virtual int CopySend(char *buf, int sz, unsigned form = 0);
    SHARED_DECL virtual void SetSended(int sended=-1);//-1,������
protected:
    virtual void ProcessMassage(const IMessage &msg) = 0;
    virtual int ProcessReceive(void *buf, int len) = 0;
protected:
/*******************************************************************************************
���Ǹ�����ʵ�������
ProcessMassage():�������֮����Ϣ��
ProcessReceive():�����������ݣ�ret:�Ѵ������ݳ���
GetObjectType():����ʵ�����ͣ�����ֵ��Ҫ���Ӧ��IObjectManager::GetObjectType()��ͬ
OnConnected(bConnected):���ӶϿ�����
IObject(sock, id)��sock:socket;id:����ʵ���ʶ
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
    IMutex                  *m_mtxSend;
    std::list<IMessage*>    m_lsMsg;
    std::list<IMessage*>    m_lsMsgRelease;
    int                     m_idThread;
};

class IObjectManager
{
    typedef std::map<int, std::list<IObject*> > ObjectMap;
public:
    virtual int GetObjectType()const = 0;
    SHARED_DECL bool AddObject(IObject *obj);
    SHARED_DECL IObject *GetObjectByID(const std::string &id)const;
    SHARED_DECL bool SendMsg(IMessage *msg);
    SHARED_DECL bool Receive(ISocket *s, const BaseBuff &buff);
    void ReceiveMessage(IMessage *);
    void RemoveRcvMsg(IMessage *);
    void AddRelease(IMessage *);
    bool HasIndependThread()const;
    bool ProcessBussiness();
    void RemoveObject(IObject *);
    const std::list<IObject*> &GetThreadObject(int t)const;
protected:
    SHARED_DECL IObjectManager(uint16_t nThread = 1);
    SHARED_DECL virtual ~IObjectManager();

    void ProcessMassage(const IMessage &msg);
    void InitThread(uint16_t nThread = 1);
    int GetPropertyThread()const;
    void AddMessage(IMessage *msg);
    void PrcsRelease();
    SHARED_DECL virtual bool PrcsRemainMsg(const IMessage &msg);
protected:
    virtual IObject *ProcessReceive(ISocket *s, const char *buf, int &len) = 0;
private:
    friend class ObjectManagers;
    std::list<IMessage*>            m_lsMsg;
    std::list<IMessage*>            m_lsMsgRelease;
    std::list<BussinessThread*>     m_lsThread;
    ObjectMap                       m_mapThreadObject;
    IMutex                          *m_mtx;
};

#ifdef SOCKETS_NAMESPACE
}
#endif
#endif //__COMMON_H__