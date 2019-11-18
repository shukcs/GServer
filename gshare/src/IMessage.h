#ifndef __IMESSAGE_H__
#define __IMESSAGE_H__

#include <string>
#include "stdconfig.h"

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

class IObject;
class IObjectManager;

class MessageData
{
public:
    SHARED_DECL MessageData(IObject *sender, const std::string &id, int rcv, int tpMs);
    SHARED_DECL MessageData(IObjectManager *sender, const std::string &id, int rcv, int tpMs);
    SHARED_DECL virtual ~MessageData();
private:
    void AddRef();
    bool Release();
    bool IsValid() const;
private:
    friend class IMessage;
    int         m_countRef;
    int         m_tpRcv;
    int         m_tpMsg;
    int         m_tpSender;
    std::string m_idRcv;
    std::string m_idSnd;
};

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
    SHARED_DECL IMessage(MessageData *data);
    SHARED_DECL IMessage(const IMessage &oth);
    SHARED_DECL virtual ~IMessage();

    virtual void *GetContent()const = 0;
    virtual int GetContentLength()const = 0;

    SHARED_DECL int GetReceiverType()const;
    SHARED_DECL void SetMessgeType(int tp);
    SHARED_DECL int GetMessgeType()const;
    SHARED_DECL const std::string &GetReceiverID()const;
    SHARED_DECL int GetSenderType()const;
    SHARED_DECL const std::string &GetSenderID()const;
    SHARED_DECL IObject *GetSender()const;

    SHARED_DECL bool IsValid()const;
    SHARED_DECL void Release();
    SHARED_DECL int CountDataRef()const;
    SHARED_DECL virtual IMessage *Clone()const;
protected:
    MessageData *m_data;
};

#ifdef SOCKETS_NAMESPACE
}
#endif
#endif //__IMESSAGE_H__