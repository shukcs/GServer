﻿#include "IMessage.h"
#include "ObjectBase.h"
#include "ObjectManagers.h"
#include "common/Utility.h"

using namespace std;
#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif

////////////////////////////////////////////////////////////////////////////////////////
//MessageData
////////////////////////////////////////////////////////////////////////////////////////
MessageData::MessageData(const IObject &sender, int16_t tpMs)
: m_threadID(Utility::ThreadID()), m_tpMsg(tpMs)
, m_tpSender(IObject::UnKnow)
{
    m_tpSender = sender.GetObjectType();
    m_idSnd = sender.GetObjectID();
}

MessageData::MessageData(const IObjectManager *sender, int16_t tpMs)
: m_threadID(Utility::ThreadID()), m_tpMsg(tpMs)
, m_tpSender(IObject::UnKnow)
{
    if (sender)
        m_tpSender = sender->GetObjectType();
}

MessageData::MessageData(const string &sender, int tpSnd, int16_t tpMs)
    : m_threadID(Utility::ThreadID()), m_tpMsg(tpMs)
    , m_tpSender(tpSnd), m_idSnd(sender)
{
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
IMessage::IMessage(MessageData *data, const std::string &rcv, int32_t tpRc)
: m_data(data), m_tpRcv(tpRc), m_idRcv(rcv)
{
}

IMessage::~IMessage()
{
    delete m_data;
    m_data = NULL;
}

int32_t IMessage::GetReceiverType() const
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

int32_t IMessage::GetSenderType() const
{
    return  m_data->m_tpSender;
}

const std::string &IMessage::GetSenderID() const
{
    return m_data->m_idSnd;
}

bool IMessage::IsValid() const
{
    return m_data && m_data->IsValid() && m_tpRcv > IObject::UnKnow;
}

 std::string IMessage::ClassName() const
{
     return _className();
}

int IMessage::CreateThreadID() const
{
    return m_data ? m_data->m_threadID : -1;
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
ObjectSignal::ObjectSignal(const IObject *sender, int32_t rcvTp, int e, const string &rcv)
:IMessage(new MessageData(*sender, e), rcv, rcvTp)
{
}

void *ObjectSignal::GetContent() const
{
    return NULL;
}

int ObjectSignal::GetContentLength() const
{
    return 0;
}
