#ifndef SCTCPTOOLWIDGET_H
#define SCTCPTOOLWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QTranslator>
#include "SCStatusTcp.h"

namespace Ui {
class SCTcpToolWidget;
}

class SCTcpToolWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SCTcpToolWidget(QWidget *parent = 0);
    ~SCTcpToolWidget();

public slots:
    void stateChanged(QAbstractSocket::SocketState state);
    void receiveTcpError(QAbstractSocket::SocketError error);
    void slotPrintInfo(QString info);

    void slotChangedText(bool isOk, int revCommand, QByteArray revData, QByteArray revHex, int number, int msTime);
    void slotAutomaticallyScroll();
private slots:
    void on_pushButton_connect_clicked();
    void on_pushButton_send_clicked();

    void on_pushButton_clearInfo_clicked();

    void on_pushButton_zipFile_clicked();

    void on_checkBox_timeOut_clicked(bool checked);


    void on_checkBox_queryTime_clicked(bool checked);

    void on_pushButton_openFolder_clicked();

    void on_toolButton_en_clicked();

    void on_toolButton_ch_clicked();

private:
    Ui::SCTcpToolWidget *ui;
    SCStatusTcp *_scStatusTcp {Q_NULLPTR};
    QTimer *_queryTimer {Q_NULLPTR};
    QTranslator * _translator {Q_NULLPTR};
};

#endif // SCTCPTOOLWIDGET_H
