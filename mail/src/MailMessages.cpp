#include "MailMessages.h"
#include "MailManager.h"

using namespace std;
#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif

static const string sStrEmpty;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//MailMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MailMessage::MailMessage(const IObject &sender, const std::string &rcv)
: IMessage(new MessageData(sender, SendMail), rcv, IObject::Mail), m_seq(-1)
{
}

MailMessage::MailMessage(IObjectManager *sender, const std::string &rcv)
: IMessage(new MessageData(sender, SendMail), rcv, IObject::Mail), m_seq(-1)
{
}

MailMessage::MailMessage(const std::string &sender, int tpSend, const std::string &rcv)
: IMessage(new MessageData(sender, tpSend, SendMail), rcv, IObject::Mail), m_seq(-1)
{
}

 void MailMessage::SetSeq(int seq)
{
     m_seq = seq;
}


 void MailMessage::SetTitle(const std::string &tt)
 {
     m_title = tt;
 }

 void MailMessage::SetBody(const std::string &tt)
 {
     m_body = tt;
 }

 void MailMessage::SetMailTo(const std::string &tt)
 {
     m_mailTo = tt;
 }

 int MailMessage::GetSeq() const
 {
     return m_seq;
 }

 const std::string & MailMessage::GetTitle() const
 {
     return m_title;
 }

 const std::string &MailMessage::GetBody() const
 {
     return m_body;
 }

 const std::string &MailMessage::GetMailTo() const
 {
     return m_mailTo;
 }

void *MailMessage::GetContent() const
{
    return NULL;
}

int MailMessage::GetContentLength() const
{
    return 0;
}

MailRsltMessage *MailMessage::GenerateAck(IObject *db) const
{
    if (auto msg = db ? new MailRsltMessage(*db, GetSenderType(), GetSenderID()) : NULL)
    {
        msg->SetSeq(m_seq); 
        return msg;
    }

    return NULL;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//MailRsltMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MailRsltMessage::MailRsltMessage(const IObject &sender, int tp, const std::string &rcv)
    : IMessage(new MessageData(sender, MailRslt), rcv, tp), m_seq(-1)
{
}

void MailRsltMessage::SetSeq(int seq)
{
    m_seq = seq;
}

void MailRsltMessage::SetErrCode(const std::string &cd)
{
    m_errCode = cd;
}

const std::string & MailRsltMessage::GetErrCode() const
{
    return m_errCode;
}

int MailRsltMessage::GetSeq() const
{
    return m_seq;
}

void * MailRsltMessage::GetContent() const
{
    return NULL;
}

int MailRsltMessage::GetContentLength() const
{
    return 0;
}
