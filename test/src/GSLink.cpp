#include "GSLink.h"
#include <QtCore/QDateTime>
#include "das.pb.h"
#include "common/mavlink.h"

using namespace das::proto;

GSLink::GSLink(const QString &id, const QString &pswd, QObject *p)
:AbsLink(id, p), m_seq(0), m_pswd(pswd)
{
}

GSLink::~GSLink()
{
}

void GSLink::SendBind(const QString &uav, bool bind)
{
    if (bind && !m_uav.isEmpty())
        SendBind(m_uav, false);

    RequestBindUav bd;
    bd.set_seqno(++m_seq);
    bd.set_binder(m_id.toStdString());
    bd.set_uavid(uav.toStdString());
    bd.set_opid(bind ? 1 : 0);
    Send(bd);
}

void GSLink::parse(const QString &name, const QByteArray &a)
{
    if (name == d_p_ClassName(AckGSIdentityAuthentication))
        _parseAckGSAuth(a);
    else if (name == d_p_ClassName(AckOperationInformation))
        _parseHBAck(a);
    else if (name == d_p_ClassName(PostOperationInformation))
        _parseOperationInformation(a);
}

void GSLink::CheckSndData()
{
    m_count = m_count % 100 + 1;
    switch (m_stat)
    {
    case AbsLink::St_Unknow:
        if (m_count == 1)
            Reconnect();
        break;
    case AbsLink::St_Connected:
        if (m_count == 1)
            _login();
        break;
    case AbsLink::St_LoginFail:
    case AbsLink::St_Logining:
        if (m_count == 10)
            _login();
        break;
    case AbsLink::St_Logined:
        if (m_count == 1)
            _sendHeartBeat();
        break;
    case AbsLink::St_InfoSending:
        if (m_count == 5)
            _sendHeartBeat();
        break;
    case AbsLink::St_InfoSended:
        if (m_count == 50)
            _sendHeartBeat();
        break;
    default:
        break;
    }

    if (!m_uav.isEmpty() && m_count == 48)
        SendBind(m_uav, false);
}

void GSLink::_parseAckGSAuth(const QByteArray &a)
{
    AckGSIdentityAuthentication ack;
    if (ack.ParseFromArray(a.data(), a.size()))
    {
        m_stat = ack.result() == 1 ? St_Logined : St_LoginFail;
        m_count = 0;
    }
}

void GSLink::_parseHBAck(const QByteArray &a)
{
    AckHeartBeat ack;
    if (ack.ParseFromArray(a.data(), a.size()))
    {
        m_stat = St_InfoSended;
        m_count = 0;
    }
}

void GSLink::_parseOperationInformation(const QByteArray &a)
{
    PostOperationInformation info;
    if (info.ParseFromArray(a.data(), a.size()))
    {
        if (info.oi_size() < 1)
            return;

        m_uav = info.oi(0).uavid().c_str();
    }
}

void GSLink::_login()
{
    RequestGSIdentityAuthentication req;
    req.set_seqno(++m_seq);
    req.set_userid(m_id.toStdString());
    req.set_password(m_pswd.toStdString());
    Send(req);
    m_stat = AbsLink::St_Logining;
}

void GSLink::_sendHeartBeat()
{
    PostHeartBeat hb;
    hb.set_seqno(++m_seq);
    m_stat = AbsLink::St_InfoSending;
    m_count = 1;
    Send(hb);
}
