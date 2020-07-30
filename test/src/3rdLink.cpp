#include "3rdLink.h"
#include <QtCore/QDateTime>
#include "UavLink.h"
#include "das.pb.h"
#include "common/mavlink.h"

using namespace das::proto;

Uav3rdLink::Uav3rdLink(const QString &id, QObject *p):AbsLink(id, p)
, m_seq(0)
{
}

Uav3rdLink::~Uav3rdLink()
{
}

void Uav3rdLink::parse(const QString &name, const QByteArray &a)
{
    if (name == d_p_ClassName(Ack3rdIdentityAuthentication))
        _parseAckUavAuth(a);
    else if (name == d_p_ClassName(AckOperationInformation))
        _parseAckInformation(a);
}

void Uav3rdLink::CheckSndData()
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
            _posInfo();
        break;
    case AbsLink::St_InfoSending:
        if (m_count == 5)
            _posInfo();
        break;
    case AbsLink::St_InfoSended:
        if (m_count == 10)
            _posInfo();
        break;
    default:
        break;
    }
}

void Uav3rdLink::_parseAckUavAuth(const QByteArray &a)
{
    Ack3rdIdentityAuthentication ack;
    if (ack.ParseFromArray(a.data(), a.size()))
    {
        m_stat = ack.result() == 1 ? St_Logined : St_LoginFail;
        m_count = 0;
    }
}

void Uav3rdLink::_parseAckInformation(const QByteArray &a)
{
    AckOperationInformation ack;
    if (ack.ParseFromArray(a.data(), a.size()))
        m_stat = St_InfoSended;
}

void Uav3rdLink::_login()
{
    Request3rdIdentityAuthentication req;
    req.set_seqno(++m_seq);
    req.set_identification("YIFEI:SERVER");
    req.set_secretkey("123");
    Send(req);
    m_stat = AbsLink::St_Logining;
}

void Uav3rdLink::_posInfo()
{
    PostOperationInformation poi;
    poi.set_seqno(++m_seq);
    if (auto info = poi.add_oi())
    {
        info->set_uavid(m_id.toStdString());
        info->set_timestamp(QDateTime::currentMSecsSinceEpoch());
        if (auto gps = UavLink::GenGps(28.243140, 112.992856, 10))
            info->set_allocated_gps(gps);
    }
    m_stat = AbsLink::St_InfoSending;
    m_count = 1;
    Send(poi);
}
