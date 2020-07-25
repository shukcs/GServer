#ifndef __UAV3RD_LINK_H__
#define __UAV3RD_LINK_H__
#include "AbsLink.h"

class Uav3rdLink : public AbsLink
{
public:
    Uav3rdLink(const QString &id, QObject *p = NULL);
    ~Uav3rdLink();
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

#endif//__UAV3RD_LINK_H__
