#include "IMessage.h"
#include "ObjectBase.h"
#include "ObjectManagers.h"

using namespace std;
#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif

////////////////////////////////////////////////////////////////////////////////////////
//MessageData
////////////////////////////////////////////////////////////////////////////////////////
MessageData::MessageData(IObject *sender, const std::string &id, int rcv, int tpMs)
    : m_countRef(1), m_tpRcv(rcv), m_tpMsg(tpMs), m_tpSender(IObject::UnKnow)
    , m_idRcv(id)
{
    if (sender)
    {
        m_tpSender = sender->GetObjectType();
        m_idSnd = sender->GetObjectID();
    }
}

MessageData::MessageData(IObjectManager *sender, const std::string &id, int rcv, int tpMs)
: m_countRef(1), m_tpRcv(rcv), m_tpMsg(tpMs), m_tpSender(IObject::UnKnow)
, m_idRcv(id)
{
    if (sender)
        m_tpSender = sender->GetObjectType();
}

MessageData::~MessageData()
{
}

void MessageData::AddRef()
{
    m_countRef++;
}

bool MessageData::Release()
{
    return --m_countRef < 1;
}

bool MessageData::IsValid() const
{
    return m_tpSender > IObject::UnKnow && m_tpRcv > IObject::UnKnow && m_countRef > 0;
}
////////////////////////////////////////////////////////////////////////////////////////
//IMessage
////////////////////////////////////////////////////////////////////////////////////////
IMessage::IMessage(MessageData *data)
: m_data(data)
{
}

IMessage::IMessage(const IMessage &oth): m_data(oth.m_data)
{
    if (m_data)
        m_data->AddRef();
}

IMessage::~IMessage()
{
    if (m_data && m_data->Release())
        delete m_data;
}

int IMessage::GetReceiverType() const
{
    return m_data->m_tpRcv;
}

void IMessage::SetMessgeType(int tp)
{
    if (m_data)
        m_data->m_tpMsg = tp;
}

int IMessage::GetMessgeType() const
{
    return m_data->m_tpMsg;
}

const std::string &IMessage::GetReceiverID() const
{
    return m_data->m_idRcv;
}

int IMessage::GetSenderType() const
{
    return  m_data->m_tpSender;
}

const std::string &IMessage::GetSenderID() const
{
    return m_data->m_idSnd;
}

IObject *IMessage::GetSender() const
{
    if (!m_data || m_data->m_idSnd.empty())
        return NULL;

    IObjectManager *om = ObjectManagers::Instance().GetManagerByType(GetSenderType());
    return om->GetObjectByID(m_data->m_idSnd);
}

bool IMessage::IsValid() const
{
    return m_data && m_data->IsValid();
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
        obj->RemoveMessage(this);
    else
        om->RemoveMessage(this);

    if (IObject *obj = GetSender())
        obj->PushReleaseMsg(this);
    else if (IObjectManager *m = ObjectManagers::Instance().GetManagerByType(GetSenderType()))
        m->PushReleaseMsg(this);
}

int IMessage::CountDataRef() const
{
    return m_data ? m_data->m_countRef : 0;
}

IMessage *IMessage::Clone() const
{
    return NULL;
}
