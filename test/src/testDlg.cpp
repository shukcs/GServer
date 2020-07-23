#include "testDlg.h"
#include <QtWidgets/QFileDialog>
#include <QtCore/QTextStream>
#include "ui_testDlg.h"
#include "VGMysql.h"
#include <QtCore/QDateTime>
#include "DBExecItem.h"
#include "LinkManager.h"
#include "UavLink.h"
#include "GSLink.h"
#include "3rdLink.h"
#include "TrackerLink.h"

/*-----------------------------------------------------------------------
TestCloud
-----------------------------------------------------------------------*/
TestDlg::TestDlg(QWidget *parent): QDialog(parent), m_ui(new Ui::testDlg)
, m_sql(new VGMySql), m_idTm(-1)
{
    if(m_ui)
    {
        m_ui->setupUi(this);
        connect(m_ui->btn_ok, &QPushButton::clicked, this, &TestDlg::onBtnOk);
        connect(m_ui->btn_uav, &QPushButton::clicked, this, &TestDlg::onBtnUav);
        connect(m_ui->btn_gs, &QPushButton::clicked, this, &TestDlg::onBtnGs);
        connect(m_ui->btn_db, &QPushButton::clicked, this, &TestDlg::onBtnDB);
        connect(m_ui->btn_test, &QPushButton::clicked, this, &TestDlg::onBtnTest);
    }
}

void TestDlg::onBtnOk()
{
    close();
}

void TestDlg::onBtnGs()
{
    QString file = QFileDialog::getOpenFileName(this, "Open GS file", m_ui->edit_gs->text(), "*.txt");
    if (!file.isEmpty())
        m_ui->edit_gs->setText(file);
}

void TestDlg::onBtnUav()
{
    QString file = QFileDialog::getOpenFileName(this, "Open GS file", m_ui->edit_uav->text(), "*.txt");
    if (!file.isEmpty())
        m_ui->edit_uav->setText(file);
}

void TestDlg::onBtnDB()
{
    QString file = m_ui->edit_uav->text();
    if (!file.isEmpty())
        parseUav(file);
    file = m_ui->edit_gs->text();
    if (!file.isEmpty())
        parseGS(file);
}

void TestDlg::onBtnTest()
{
    if (m_ui->cb_3rd->isChecked())
        connect3rds();
    if (m_ui->cb_tracker->isChecked())
        connectTrackers();

    QString file = m_ui->edit_uav->text();
    if (!file.isEmpty() && m_ui->cb_uav->isChecked())
        parseUav(file, false);
    file = m_ui->edit_gs->text();
    if (!file.isEmpty() && m_ui->cb_gs->isChecked())
        parseGS(file, false);

    if (m_idTm < 0)
        m_idTm = startTimer(100);
}

void TestDlg::timerEvent(QTimerEvent *e)
{
    if (e->timerId() != m_idTm)
        return QDialog::timerEvent(e);

    for (AbsLink *l : m_users)
    {
        l->CheckSndData();
    }
    for (AbsLink *l : m_uavs)
    {
        l->CheckSndData();
    }
}

LinkManager *TestDlg::getPropertyMgr()
{
    auto ret = m_mgrs.count() > 0 ? m_mgrs.last() : NULL;
    if (ret && ret->CountSock() < 64)
        return ret;

    ret = new LinkManager();
    if (ret)
        m_mgrs.append(ret);

    return ret;
}

void TestDlg::parseGS(const QString &file, bool insert)
{
    QFile f(file);
    if (!f.open(QIODevice::ReadOnly))
        return;

    QTextStream stream(&f);
    while (!stream.atEnd())
    {
        QString line = stream.readLine(256);
        QStringList strLs = line.split("=", QString::SkipEmptyParts);
        if (strLs.size() != 2)
            continue;

        if (insert)
            insertGS(strLs.first(), strLs.last());
        else
            connectGS(strLs.first(), strLs.last());
    }
}

void TestDlg::parseUav(const QString &file, bool insert)
{
    QFile f(file);
    if (!f.open(QIODevice::ReadOnly))
        return;

    QTextStream stream(&f);
    bool bSuc;
    while (!stream.atEnd())
    {
        QString line = stream.readLine(256);
        QStringList strLs = line.split(":", QString::SkipEmptyParts);
        if (strLs.size() != 2 || strLs.first() != "VIGAU" || strLs.last().length() != 8 || strLs.last().toInt(&bSuc, 16) == 0)
            continue;

        if (insert)
            insertUav(line);
        else
            connectUav(line);
    }
}

void TestDlg::insertGS(const QString &gs, const QString &pswd)
{
    QString str = "insert GSInfo(user, pswd, auth, regTm) value(\'%1\', \'%2\', 1, %3)";
    str = str.arg(gs).arg(pswd).arg(QDateTime::currentSecsSinceEpoch());
    ensureMysql();
    m_sql->Execut(str.toStdString());
}

void TestDlg::insertUav(const QString &uav)
{
    QString str = "insert UavInfo(id, authCheck) value('%1\', \'%2\')";
    str = str.arg(uav).arg(ExecutItem::GenCheckString(8).c_str());
    ensureMysql();
    m_sql->Execut(str.toStdString());
}

void TestDlg::connectGS(const QString &user, const QString &pswd)
{
    if (auto mgr = getPropertyMgr())
    {
        if (auto link = new GSLink(user, pswd, this))
        {
            link->ConnectTo(m_ui->edit_host->text(), m_ui->edit_port->text().toInt(), mgr);
            m_uavs.append(link);
        }
    }
}

void TestDlg::connectUav(const QString &uav)
{
    if (auto mgr = getPropertyMgr())
    {
        if (auto link = new UavLink(uav, this))
        {
            link->ConnectTo(m_ui->edit_host->text(), m_ui->edit_port->text().toInt(), mgr);
            m_uavs.append(link);
        }
    }
}

void TestDlg::connectTrackers()
{
    QFile f("trackers.txt");
    if (!f.open(QIODevice::ReadOnly))
        return;

    QTextStream stream(&f);
    bool bSuc;
    while (!stream.atEnd())
    {
        QString line = stream.readLine(256);
        QStringList strLs = line.split(":", QString::SkipEmptyParts);
        if (strLs.size() != 2 || strLs.first() != "VIGAT" || strLs.last().length() != 8 || strLs.last().toInt(&bSuc, 16) == 0)
            continue;

        if (auto mgr = getPropertyMgr())
        {
            if (auto link = new TrackerLink(line, this))
            {
                link->ConnectTo(m_ui->edit_host->text(), m_ui->edit_port->text().toInt(), mgr);
                m_trackers.append(link);
            }
        }
    }
}

void TestDlg::connect3rds()
{
    QFile f("3rd.txt");
    if (!f.open(QIODevice::ReadOnly))
        return;

    QTextStream stream(&f);
    bool bSuc;
    while (!stream.atEnd())
    {
        QString line = stream.readLine(256);
        if (auto mgr = getPropertyMgr())
        {
            if (auto link = new Uav3rdLink(line, this))
            {
                link->ConnectTo(m_ui->edit_host->text(), m_ui->edit_port->text().toInt(), mgr);
                m_3rds.append(link);
            }
        }
    }
}

void TestDlg::ensureMysql()
{
    if (!m_sql)
        m_sql = new VGMySql();

    if (m_sql && !m_sql->IsValid())
    {
        m_sql->ConnectMySql(m_ui->edit_ip->text().toUtf8().data()
            , m_ui->spinBox->value()
            , m_ui->edit_user->text().toUtf8().data()
            , m_ui->edit_pswd->text().toUtf8().data()
            , m_ui->edit_db->text().toUtf8().data());
    }
}
