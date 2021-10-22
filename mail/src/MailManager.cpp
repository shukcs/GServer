#include "MailManager.h"
#include "MailMessages.h"
#include "Utility.h"
#include "tinyxml.h"
#include "ObjectManagers.h"
#include "mailSmtp/quickmail.h"

#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif
////////////////////////////////////////////////////////////////////////////////////////////////
//ObjectMail
////////////////////////////////////////////////////////////////////////////////////////////////
ObjectMail::ObjectMail(const std::string &id) : IObject(id), m_port(25), m_smtps(false)
{
}

ObjectMail::~ObjectMail()
{
}

bool ObjectMail::Parse(const TiXmlElement &e)
{
    if (const char *tmp =e.Attribute("user"))
        m_user = tmp;

    if (const char *tmp = e.Attribute("password"))
        m_pswd = tmp;

    if (const char *tmp = e.Attribute("host"))
        m_host = tmp;

    if (const char *tmp = e.Attribute("port"))
        m_port = Utility::str2int(tmp);

    if (const char *tmp = e.Attribute("smtps"))
        m_smtps = Utility::str2int(tmp) > 0;

    return !m_user.empty() && !m_host.empty();
}

const char *ObjectMail::SendMail(const MailMessage &msg)
{
    if (msg.GetMailTo().empty())
        return "No destinate e-mail";

    quickmail_initialize();
    auto mailobj = quickmail_create(NULL, NULL);
    quickmail_set_from(mailobj, GetObjectID().c_str());
    quickmail_add_to(mailobj, msg.GetMailTo().c_str());
#ifdef _DEBUG
    quickmail_set_debug_log(mailobj, stdout);
#endif

    quickmail_set_subject(mailobj, msg.GetTitle().c_str());
    auto body = msg.GetBody();

    //mimetype = "text /plain", body as attach file
    quickmail_add_body_memory(mailobj, NULL, &body.at(0), body.size(), 0);

      //add message_id by cdevelop@qq.com
    char message_id[256];
    snprintf(message_id, sizeof(message_id), "Message-ID: <%ld@%s>", Utility::msTimeTick(), m_id.c_str());
    quickmail_add_header(mailobj, message_id);
    if (!quickmail_get_from(mailobj))
        return "Invalid command line parameters";

    const char *errmsg = 0;
    if (!m_smtps)
        errmsg = quickmail_send(mailobj, m_host.c_str(), m_port, m_user.c_str(), m_pswd.c_str());
    else
        errmsg = quickmail_send_secure(mailobj, m_host.c_str(), m_port, m_user.c_str(), m_pswd.c_str());

    //clean up
    quickmail_destroy(mailobj);
    quickmail_cleanup();
    return errmsg;
}

IObject::TypeObject ObjectMail::MailType()
{
    return IObject::Mail;
}

ObjectMail *ObjectMail::ParseMaill(const TiXmlElement &e)
{
    const char *name = e.Attribute("name");
    ObjectMail *ret = name ? new ObjectMail(name) : NULL;

    if (ret && ret->Parse(e))
        return ret;

    delete ret;
    return NULL;
}

int ObjectMail::GetObjectType() const
{
    return MailType();
}

void ObjectMail::InitObject()
{
    if (m_stInit != Uninitial)
        return;

    m_stInit = IObject::Initialed;
}

void ObjectMail::ProcessMessage(const IMessage *msg)
{
    if (m_stInit == Uninitial)
        InitObject();

    auto mailMsg = dynamic_cast<const MailMessage *>(msg);
    if (!mailMsg)
        return;

    auto err = SendMail(*mailMsg);
    MailRsltMessage *ack = mailMsg->GenerateAck(this);
    if (ack)
    {
        if (err)
            ack->SetErrCode(err);

        SendMsg(ack);
    }
}
////////////////////////////////////////////////////////////////////////////////
//MailManager
////////////////////////////////////////////////////////////////////////////////
MailManager::MailManager():IObjectManager()
{
}

MailManager::~MailManager()
{
}

int MailManager::GetObjectType() const
{
    return ObjectMail::MailType();
}

bool MailManager::PrcsPublicMsg(const IMessage &msg)
{
    if (auto o = GetFirstObject())
        o->ProcessMessage(&msg);

    return true;
}

IObject *MailManager::PrcsNotObjectReceive(ISocket *, const char *, int)
{
    return NULL;
}

void MailManager::LoadConfig(const TiXmlElement *root)
{
    TiXmlDocument doc;
    doc.LoadFile("MailManager.xml");

    const TiXmlElement *cfg = root ? root->FirstChildElement("MailManager") : NULL;
    if (!cfg)
        return;

    InitThread(1, 0);
    const TiXmlNode *dbNode = cfg->FirstChildElement("Object");
    while (dbNode)
    {
        if (ObjectMail *db = ObjectMail::ParseMaill(*dbNode->ToElement()))
            AddObject(db);

        dbNode = dbNode->NextSiblingElement("Object");
    }
}

bool MailManager::IsReceiveData() const
{
    return false;
}

DECLARE_MANAGER_ITEM(MailManager)