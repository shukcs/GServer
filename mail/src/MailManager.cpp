#include "MailManager.h"
#include "MailMessages.h"
#include "Utility.h"
#include "tinyxml.h"
#include "ObjectManagers.h"
#include "mailSmtp/quickmail.h"
#if defined _WIN32 || defined _WIN64
#include <winsock2.h>
#endif
#ifdef _MSC_VER
#pragma comment(lib, "wsock32.lib")
#pragma comment(lib, "ws2_32.lib")
#endif

#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif

class WSAInitializer // Winsock Initializer
{
public:
    WSAInitializer()
    {
#if defined _WIN32 || defined _WIN64
        WSADATA wsadata;
        if (WSAStartup(MAKEWORD(2, 2), &wsadata))
        {
            exit(-1);
        }
#endif //defined _WIN32 || defined _WIN64
    }
    ~WSAInitializer()
    {
#if defined _WIN32 || defined _WIN64
        WSACleanup();
#endif //defined _WIN32 || defined _WIN64
    }
private:
};
////////////////////////////////////////////////////////////////////////////////////////////////
//ObjectMail
////////////////////////////////////////////////////////////////////////////////////////////////
ObjectMail::ObjectMail(const std::string &id) : IObject(id), m_port(25), m_smtps(false)
{
    static WSAInitializer s;
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

const char *ObjectMail::SendMail(const MailMessage &msg, bool bLog)
{
    if (msg.GetMailTo().empty())
        return "No destinate e-mail";

    quickmail_initialize();
    auto mailobj = quickmail_create(NULL, NULL);
    quickmail_set_from(mailobj, GetObjectID().c_str());
    quickmail_add_to(mailobj, msg.GetMailTo().c_str());
    quickmail_set_subject(mailobj, msg.GetTitle().c_str());
    auto body = msg.GetBody();

    //mimetype = "text /plain", body as attach file
    quickmail_add_body_memory(mailobj, NULL, &body.at(0), body.size(), 0);

      //add message_id by cdevelop@qq.com
    auto message_id = "Message-ID: <" + Utility::bigint2string(Utility::msTimeTick()) + "@" + m_id + ">";
    quickmail_add_header(mailobj, message_id.c_str());
    if (bLog)
        quickmail_set_debug_log(mailobj, stdout);
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