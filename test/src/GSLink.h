#ifndef __GS_LINK_H__
#define __GS_LINK_H__
#include "AbsLink.h"

class GSLink : public AbsLink
{
public:
    GSLink(const QString &id, const QString &pswd, QObject *p = NULL);
    ~GSLink();
    void SendBind(const QString &uav, bool bind);
protected:
    void parse(const QString &name, const QByteArray &a);
    void CheckSndData();
private:
    void _parseAckGSAuth(const QByteArray &a);
    void _parseHBAck(const QByteArray &a);
    void _parseOperationInformation(const QByteArray &a);

    void _login();
    void _sendHeartBeat();
private:
    int     m_seq;
    QString m_pswd;
    QString m_uav;
};

#endif//__GS_LINK_H__
