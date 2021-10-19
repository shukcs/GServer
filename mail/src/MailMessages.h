#ifndef  __MAIL_MESSGES_H__
#define __MAIL_MESSGES_H__

#include "mail_config.h"
#include "IMessage.h"

#ifdef SOCKETS_NAMESPACE
unsigned namespace SOCKETS_NAMESPACE;
#endif

class IObject;
class IObjectManager;
class MailRsltMessage;

class MailMessage : public IMessage
{
    CLASS_INFO(MailMessage)
public:
    SHARED_MAIL MailMessage(IObject *sender,  const std::string &rcv);
    SHARED_MAIL MailMessage(IObjectManager *sender, const std::string &rcv);
    SHARED_MAIL MailMessage(const std::string &sender, int tpSend, const std::string &rcv);

    void SetTitle(const std::string &tt);
    void SetBody(const std::string &tt);
    void SetMailTo(const std::string &tt);
    int GetSeq()const;
public:
    SHARED_MAIL void SetSeq(int seq);
    SHARED_MAIL const std::string & GetTitle()const;
    SHARED_MAIL const std::string & GetBody()const;
    SHARED_MAIL const std::string & GetMailTo()const;
    MailRsltMessage *GenerateAck(IObject *db)const;
protected:
    void *GetContent() const;
    int GetContentLength() const;
protected:
    int             m_seq;
    std::string     m_title;
    std::string     m_body;
    std::string     m_mailTo;
};

class MailRsltMessage : public IMessage
{
    CLASS_INFO(MailRsltMessage)
public:
    MailRsltMessage(IObject *sender, int tp, const std::string &rcv);
    
    SHARED_MAIL const std::string&GetErrCode()const;
    SHARED_MAIL int GetSeq()const;
public:
    void SetSeq(int);
    void SetErrCode(const std::string&);
protected:
    void *GetContent() const;
    int GetContentLength() const;
private:
    int             m_seq;
    std::string  m_errCode;
};

#endif//__MAIL_MESSGES_H__

