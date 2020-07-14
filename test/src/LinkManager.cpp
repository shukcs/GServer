#include "LinkManager.h"
#include <QtNetwork/QTcpSocket>
#include <QtCore/QThread>

#define MaxSocket 64

LinkManager::LinkManager(QObject *p/*=NULL*/) : QObject(p)
, m_thread(new QThread)
{
    if (m_thread)
    {
        moveToThread(m_thread);
        m_thread->start();
    }
}

LinkManager::~LinkManager()
{
    qDeleteAll(m_socks);
    m_thread->terminate();
    delete m_thread;
}

QTcpSocket *LinkManager::CreateSocket()
{
    if (!m_thread || m_socks.size()>=MaxSocket)
        return NULL;

    if (auto s = new QTcpSocket())
    {
        s->moveToThread(m_thread);
        m_socks << s;
        connect(s, &QObject::destroyed, this, &LinkManager::onDistoied);
        return s;
    }

    return NULL;
}

int LinkManager::CountSock() const
{
    return m_socks.count();
}

void LinkManager::onConnectTo(const QString &host, int port, QTcpSocket *sock)
{
    if (m_socks.contains(sock))
    {
        sock->connectToHost(host, port);
        if (!sock->waitForConnected(1000))
            emit sock->disconnected();
    }
}

void LinkManager::onWriteSock(const QByteArray &a, QTcpSocket *sock)
{
    if (m_socks.contains(sock))
        sock->write(a);
}

void LinkManager::onDistoied()
{
    m_socks.removeAll((QTcpSocket*)sender());
}
