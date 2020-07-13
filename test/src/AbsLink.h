#ifndef __ABS_LINK_H__
#define __ABS_LINK_H__
#include <QtCore/QObject>

#define d_p_ClassName(_cls) QString(_cls::descriptor()->full_name().c_str())

class QTcpSocket;
class LinkManager;
namespace google {
    namespace protobuf {
        class Message;
    }
}
class AbsLink:public QObject
{
    Q_OBJECT
protected:
    enum LinkStat {
        St_Unknow,
        St_Connecting,
        St_Connected,
        St_LoginFail,
        St_Logining,
        St_Logined,
        St_InfoSending,
        St_InfoSended,
    };
public:
    AbsLink(const QString &id, QObject *p=NULL);
    ~AbsLink();

    bool ConnectTo(const QString &host, int port, LinkManager *mgr);
    virtual void CheckSndData() = 0;
protected slots:
    virtual void parse(const QString &name, const QByteArray &a) = 0;
    void onSockConnect();
    void onSockDisconnect();
    void onReadReady();
    void Send(const google::protobuf::Message &pb);
    void Reconnect();
signals:
    void connectTo(const QString &host, int port, QTcpSocket *sock);
    void writeSock(const QByteArray &a, QTcpSocket *sock);
protected:
    QTcpSocket  *m_sock;
    QString     m_id;
    LinkStat    m_stat;
    uint32_t    m_count;
    QByteArray  m_read;
    QString     m_sever;
    int         m_port;
};

#endif //__ABS_LINK_H__