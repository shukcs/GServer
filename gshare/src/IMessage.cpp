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
MessageData::MessageData(IObject *sender, int tpMs)
    : m_countRef(1), m_tpMsg(tpMs), m_tpSender(IObject::UnKnow)
{
    if (sender)
    {
        m_tpSender = sender->GetObjectType();
        m_idSnd = sender->GetObjectID();
    }
}

MessageData::MessageData(IObjectManager *sender, int tpMs)
: m_countRef(1), m_tpMsg(tpMs), m_tpSender(IObject::UnKnow)
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
    return m_tpSender > IObject::UnKnow && m_countRef > 0;
}
////////////////////////////////////////////////////////////////////////////////////////
//IMessage
////////////////////////////////////////////////////////////////////////////////////////
IMessage::IMessage(MessageData *data, const std::string &rcv, int tpRc)
: m_data(data), m_tpRcv(tpRc), m_idRcv(rcv), m_clone(false)
{
}

IMessage::IMessage(const IMessage &oth) : m_data(oth.m_data)
, m_tpRcv(IObject::UnKnow), m_clone(true)
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
    return m_tpRcv;
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
    return m_idRcv;
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
    return m_data && m_data->IsValid() && m_tpRcv > IObject::UnKnow;
}

int IMessage::CountDataRef() const
{
    return m_data ? m_data->m_countRef : 0;
}

IMessage *IMessage::Clone(const std::string &, int) const
{
    return NULL;
}

int IMessage::IsClone() const
{
    return m_clone;
}
