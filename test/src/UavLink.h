#ifndef __UAV_LINK_H__
#define __UAV_LINK_H__
#include "AbsLink.h"

namespace das {
    namespace proto {
        class GpsInformation;
        class UavAttitude;
        class OperationParams;
    }
}
class UavLink : public AbsLink
{
public:
    UavLink(const QString &id, QObject *p = NULL);
    ~UavLink();
public:
    static das::proto::GpsInformation *GenGps(double lat, double lon, double alt);
protected:
    void parse(const QString &name, const QByteArray &a);
    void CheckSndData();
private:
    void _parseAckUavAuth(const QByteArray &a);
    void _parseAckInformation(const QByteArray &a);

    void _login();
    void _posInfo();
private:
    int m_seq;
};

#endif//__UAV_LINK_H__
