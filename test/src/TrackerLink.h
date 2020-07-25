#ifndef __TRACKER_LINK_H__
#define __TRACKER_LINK_H__
#include "AbsLink.h"

class TrackerLink : public AbsLink
{
public:
    TrackerLink(const QString &id, QObject *p = NULL);
    ~TrackerLink();
protected:
    void parse(const QString &name, const QByteArray &a);
    void CheckSndData();
private:
    void _parseAckUavAuth(const QByteArray &a);
    void _parseAckInformation(const QByteArray &a);
    void _parseQueryParameters(const QByteArray &a);

    void _login();
    void _posInfo();
private:
    int m_seq;
};

#endif//__UAV_LINK_H__
