#ifndef __IMESSAGE_H__
#define __IMESSAGE_H__

#include <string>
#include "stdconfig.h"

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

class IObject;
class IObjectManager;

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
    SHARED_DECL IMessage(IObjectManager *sender, const std::string &id, int rcv, int tpMsg);
    SHARED_DECL virtual ~IMessage() {}

    virtual void *GetContent()const = 0;
    virtual int GetContentLength()const = 0;

    SHARED_DECL int GetReceiverType()const;
    SHARED_DECL int GetMessgeType()const;
    SHARED_DECL const std::string &GetReceiverID()const;
    SHARED_DECL int GetSenderType()const;
    SHARED_DECL const std::string &GetSenderID()const;
    SHARED_DECL IObject *GetSender()const;

    SHARED_DECL bool IsValid()const;
    SHARED_DECL void Release();
protected:
    int         m_tpRcv;
    int         m_tpMsg;
    int         m_tpSender;
    bool        m_bRelease;
    std::string m_idRcv;
    std::string m_idSnd;
};

#ifdef SOCKETS_NAMESPACE
}
#endif
#endif //__IMESSAGE_H__