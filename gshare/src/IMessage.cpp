#include "IMessage.h"
#include "ObjectBase.h"
#include "ObjectManagers.h"
#include "Utility.h"

using namespace std;
#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif

////////////////////////////////////////////////////////////////////////////////////////
//MessageData
////////////////////////////////////////////////////////////////////////////////////////
MessageData::MessageData(IObject *sender, int16_t tpMs)
: m_threadID(Utility::ThreadID()), m_tpMsg(tpMs)
, m_tpSender(IObject::UnKnow)
{
    if (sender)
    {
        m_tpSender = sender->GetObjectType();
        m_idSnd = sender->GetObjectID();
    }
}

MessageData::MessageData(IObjectManager *sender, int16_t tpMs)
: m_threadID(Utility::ThreadID()), m_tpMsg(tpMs)
, m_tpSender(IObject::UnKnow)
{
    if (sender)
        m_tpSender = sender->GetObjectType();
}

MessageData::~MessageData()
{
}

bool MessageData::IsValid() const
{
    return m_tpSender > IObject::UnKnow;
}
////////////////////////////////////////////////////////////////////////////////////////
//IMessage
////////////////////////////////////////////////////////////////////////////////////////
IMessage::IMessage(MessageData *data, const std::string &rcv, int tpRc)
: m_data(data), m_tpRcv(tpRc), m_idRcv(rcv)
{
}

IMessage::~IMessage()
{
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

int IMessage::CreateThreadID() const
{
    return m_data ? m_data->m_threadID : -1;
}

/////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////
ObjectEvent::ObjectEvent(const string &rcv, int rcvTp, EventType e)
:IMessage(new MessageData((IObject*)NULL, (int)e), rcv, rcvTp)
{
}

void *ObjectEvent::GetContent() const
{
    return NULL;
}

int ObjectEvent::GetContentLength() const
{
    return 0;
}
