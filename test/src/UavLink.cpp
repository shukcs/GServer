#include "UavLink.h"
#include <QtCore/QDateTime>
#include "das.pb.h"
#include "common/mavlink.h"

typedef union {
    float tmp[7];
    MAVPACKED(struct {
        float velocity[3];
        uint16_t precision;     //航线精度
        uint16_t gndHeight;     //地面高度
        uint16_t gpsVSpeed;     //垂直速度
        uint16_t curMs;         //当前任务点
        uint8_t fixType;        //定位模式及模块状态
        uint8_t baseMode;       //飞控模块状态
        uint8_t satellites;     //卫星数
        uint8_t sysStat;        //飞控状态
        uint8_t missionRes : 4; //任务状态
        uint8_t voltageErr : 4; //电压报警
        uint8_t sprayState : 4; //喷洒报警
        uint8_t magneErr : 2;   //校磁报警
        uint8_t gpsJam : 1;     //GPS干扰
        uint8_t stDown : 1;     //下载状态 0:没有或者完成，1:正下载
        uint8_t sysType;        //飞控类型
    });
} GpsAdtionValue;

static GpsAdtionValue sAdVal = {0};
using namespace das::proto;

UavLink::UavLink(const QString &id, QObject *p):AbsLink(id, p)
, m_seq(0)
{
    sAdVal.fixType = 238;
    sAdVal.satellites = 12;
}

UavLink::~UavLink()
{
}

GpsInformation *UavLink::GenGps(double lat, double lon, double alt)
{
    if (auto gps = new GpsInformation())
    {
        gps->set_latitude(lat*1e7);
        gps->set_longitude(lon*1e7);
        gps->set_altitude(alt*1000);
        for (int i=0; i<3; ++i)
        {
            gps->add_velocity(0);
        }
        return gps;
    }
    return NULL;
}

void UavLink::parse(const QString &name, const QByteArray &a)
{
    if (name == d_p_ClassName(AckUavIdentityAuthentication))
        _parseAckUavAuth(a);
    else if (name == d_p_ClassName(AckOperationInformation))
        _parseAckInformation(a);
}

void UavLink::CheckSndData()
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

void UavLink::_parseAckUavAuth(const QByteArray &a)
{
    AckUavIdentityAuthentication ack;
    if (ack.ParseFromArray(a.data(), a.size()))
    {
        m_stat = ack.result() == 1 ? St_Logined : St_LoginFail;
        m_count = 0;
    }
}

void UavLink::_parseAckInformation(const QByteArray &a)
{
    AckOperationInformation ack;
    if (ack.ParseFromArray(a.data(), a.size()))
        m_stat = St_InfoSended;
}

void UavLink::_login()
{
    RequestUavIdentityAuthentication req;
    req.set_seqno(++m_seq);
    req.set_uavid(m_id.toStdString());
    Send(req);
    m_stat = AbsLink::St_Logining;
}

void UavLink::_posInfo()
{
    PostOperationInformation poi;
    poi.set_seqno(++m_seq);
    if (auto info = poi.add_oi())
    {
        info->set_uavid(m_id.toStdString());
        info->set_timestamp(QDateTime::currentMSecsSinceEpoch());
        if (auto gps = GenGps(28.243140, 112.992856, 10))
        {
            for (int i = 3; i<7; ++i)
            {
                gps->add_velocity(sAdVal.tmp[i]);
            }
            info->set_allocated_gps(gps);
        }
    }
    m_stat = AbsLink::St_InfoSending;
    m_count = 1;
    Send(poi);
}
