#ifndef __LINK_MANAGER_H__
#define __LINK_MANAGER_H__
#include <QtCore/QObject>
#include <QtCore/QList>

class QTcpSocket;
class QThread;
class LinkManager:public QObject
{
    Q_OBJECT
public:
    LinkManager(QObject *p=NULL);
    ~LinkManager();

    QTcpSocket *CreateSocket();
    int CountSock()const;
public slots:
    void onConnectTo(const QString &host, int port, QTcpSocket *sock);
    void onWriteSock(const QByteArray &a, QTcpSocket *sock);
private slots:
    void onDistoied();
private:
    QThread             *m_thread;
    QList<QTcpSocket*>  m_socks;
};

#endif //__LINK_MANAGER_H__