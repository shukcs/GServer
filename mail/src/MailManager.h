#ifndef  __MAIL_MANAGER_H__
#define __MAIL_MANAGER_H__

#include "ObjectBase.h"

#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif

class MailMessage;
class ObjectMail : public IObject
{
public:
    ObjectMail(const std::string &id);
    ~ObjectMail();

    bool Parse(const TiXmlElement &e);
    const char *SendMail(const MailMessage &msg);
    static ObjectMail *ParseMaill(const TiXmlElement &e);
    static TypeObject MailType();
protected:
    int GetObjectType()const;
    void InitObject();

    void ProcessMessage(const IMessage *msg);
private:
    int            m_port;
    bool         m_smtps;
    std::string m_host;
    std::string  m_user;
    std::string  m_pswd;
};

class MailManager : public IObjectManager
{
public:
    MailManager();
    ~MailManager();

protected:
    int GetObjectType()const;
    IObject *PrcsNotObjectReceive(ISocket *s, const char *buf, int len);
    void LoadConfig(const TiXmlElement *root);
    bool IsReceiveData()const;
    bool PrcsPublicMsg(const IMessage &msg);
private:
};

#endif//__MAIL_MANAGER_H__

