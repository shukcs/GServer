#include "IMessage.h"
#include "ObjectBase.h"
#include "ObjectManagers.h"

using namespace std;

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif
////////////////////////////////////////////////////////////////////////////////////////
//IMessage
///////////////////////////////////////////////////////////////////////////////////////
IMessage::IMessage(IObject *sender, const std::string &id, int rcv, int tpMsg)
: m_tpRcv(rcv), m_tpMsg(tpMsg), m_tpSender(IObject::UnKnow)
, m_bRelease(false), m_idRcv(id)
{
    if (sender)
    {
        m_tpSender = sender->GetObjectType();
        m_idSnd = sender->GetObjectID();
    }
}

IMessage::IMessage(IObjectManager *sender, const std::string &id, int rcv, int tpMsg)
: m_idRcv(id), m_tpRcv(rcv), m_tpMsg(tpMsg), m_tpSender(IObject::UnKnow)
, m_bRelease(false)
{
    if (sender)
        m_tpSender = sender->GetObjectType();
}

int IMessage::GetReceiverType() const
{
    return m_tpRcv;
}

int IMessage::GetMessgeType() const
{
    return m_tpMsg;
}

const std::string &IMessage::GetReceiverID() const
{
    return m_idRcv;
}

int IMessage::GetSenderType() const
{
    return  m_tpSender;
}

const std::string &IMessage::GetSenderID() const
{
    static const string def;
    return m_idSnd;
}

IObject *IMessage::GetSender() const
{
    if (m_idSnd.empty())
        return NULL;

    IObjectManager *om = ObjectManagers::Instance().GetManagerByType(GetSenderType());
    return om->GetObjectByID(m_idSnd);
}

bool IMessage::IsValid() const
{
    return m_tpSender>IObject::UnKnow && m_tpRcv>IObject::UnKnow && !m_bRelease;
}

void IMessage::Release()
{
    IObjectManager *om = ObjectManagers::Instance().GetManagerByType(GetSenderType());
    if (!om)
    {
        delete this;
        return;
    }
    IObject *obj = om->GetObjectByID(GetSenderID());
    if (obj)
        obj->RemoveRcvMsg(this);
    else
        om->RemoveRcvMsg(this);

    if (IObject *obj = GetSender())
        obj->AddRelease(this);
    else if (IObjectManager *m = ObjectManagers::Instance().GetManagerByType(m_tpSender))
        m->AddRelease(this);
}

#ifdef SOCKETS_NAMESPACE
}
#endif