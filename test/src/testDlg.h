#ifndef __TESTCLOUD_H__
#define __TESTCLOUD_H__

#include <QtWidgets/QDialog>

namespace Ui {
    class testDlg;
}

class VGMySql;
class AbsLink;
class LinkManager;

class TestDlg : public QDialog
{
    Q_OBJECT
public:
    TestDlg(QWidget *parent = Q_NULLPTR);
protected slots:
    void onBtnOk();
    void onBtnGs();
    void onBtnUav();
    void onBtnDB();
    void onBtnTest();
protected:
    void timerEvent(QTimerEvent *event);
    LinkManager *getPropertyMgr();
private:
    void ensureMysql();
    void parseGS(const QString &file, bool insert = true);
    void parseUav(const QString &file, bool insert = true);
    void insertGS(const QString &user, const QString &pswd);
    void insertUav(const QString &uav);
    void connectGS(const QString &user, const QString &pswd);
    void connectUav(const QString &uav);
    void connectTrackers();
    void connect3rds();
private:
    Ui::testDlg         *m_ui;
    VGMySql             *m_sql;
    int                 m_idTm;
    QList<AbsLink*>     m_uavs;
    QList<AbsLink*>     m_users;
    QList<AbsLink*>     m_trackers;
    QList<AbsLink*>     m_3rds;
    QList<LinkManager*> m_mgrs;
};

#endif //__TESTCLOUD_H__