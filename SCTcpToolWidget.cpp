#include "SCTcpToolWidget.h"
#include "ui_SCTcpToolWidget.h"
#include <QDateTime>
#include <QFileDialog>
#include <QHostInfo>
#include <QDesktopServices>

SCTcpToolWidget::SCTcpToolWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SCTcpToolWidget)
{
    ui->setupUi(this);

    ui->pushButton_clearInfo->setObjectName("pushButton_green");
    ui->pushButton_connect->setObjectName("pushButton_green");
    ui->pushButton_send->setObjectName("pushButton_green");
    ui->pushButton_zipFile->setObjectName("pushButton_green");
    //自动滚动.
    connect(ui->textEdit_info,SIGNAL(textChanged()),this,SLOT(slotAutomaticallyScroll()));
    //tcp
    _scStatusTcp = new SCStatusTcp(this);
    connect(_scStatusTcp,SIGNAL(sigPrintInfo(QString)),this,SLOT(slotPrintInfo(QString)));
    connect(_scStatusTcp,SIGNAL(sigChangedText(bool,int,QByteArray,QByteArray,int,int)),
            this,SLOT(slotChangedText(bool,int,QByteArray,QByteArray,int,int)));
    //ip正则.
#if (QT_VERSION >= QT_VERSION_CHECK(6,0,0))
    QRegularExpression regExp("\\b(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\b");
    auto ev = new QRegularExpressionValidator(regExp,ui->lineEdit_ip);
    ui->lineEdit_ip->setValidator(ev);
#else
    QRegExp regExp("\\b(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\b");
    QRegExpValidator *ev = new QRegExpValidator(regExp);
    ui->lineEdit_ip->setValidator(ev);
#endif
    //0-65535,已在.ui中约束
    // ui->spinBox_number
    ui->textEdit_info->document()->setMaximumBlockCount(1000);
    on_checkBox_timeOut_clicked(true);
}

SCTcpToolWidget::~SCTcpToolWidget()
{
    if(_queryTimer){
        _queryTimer->deleteLater();
    }
    //_scStatusTcp类不用手动释放，qt会自动释放SCTcpToolWidget的子类.
    delete ui;
}

/** socket连接/断开.
 * @brief SCTcpToolWidget::on_pushButton_connect_clicked
 * @param checked
 */
void SCTcpToolWidget::on_pushButton_connect_clicked()
{
    switch (_scStatusTcp->connectHost(ui->lineEdit_ip->text(),ui->comboBox_port->currentText().toInt())) {
    case 1:
        ui->pushButton_connect->setText(tr("Connect"));
        break;

    default:
        break;
    }
}

//TODO---------------tcp----------------------
/** tcp槽实时监测tcp状态.
 * @brief SCTcpToolWidget::stateChanged
 * @param state
 */
void SCTcpToolWidget::stateChanged(QAbstractSocket::SocketState state)
{
    QString info;
    switch (state) {
    case QAbstractSocket::UnconnectedState:
        info = "QAbstractSocket::UnconnectedState";
        ui->comboBox_port->setEnabled(true);
        ui->pushButton_connect->setText(tr("Connect"));
        break;
    case QAbstractSocket::HostLookupState:
        info = "QAbstractSocket::HostLookupState";
        break;

    case QAbstractSocket::ConnectingState:
        info = "QAbstractSocket::ConnectingState";
        ui->pushButton_connect->setText(tr("Connecting..."));
        ui->comboBox_port->setEnabled(false);
        break;
    case QAbstractSocket::ConnectedState:
    {
        info = "QAbstractSocket::ConnectedState \n";
        ui->pushButton_connect->setText(tr("Disconnect"));
    }
    break;
    case QAbstractSocket::BoundState:
        info = "QAbstractSocket::BoundState";
        break;
    case QAbstractSocket::ListeningState:
        info = "QAbstractSocket::ListeningState";
        break;
    case QAbstractSocket::ClosingState:
        info = "QAbstractSocket::ClosingState";
        ui->comboBox_port->setEnabled(true);
        ui->pushButton_connect->setText(tr("Connect"));
        break;
    }
    ui->textEdit_info->append(QString("%1 IP:%2:%3 %4")
                                  .arg(_scStatusTcp->getCurrentDateTime())
                                  .arg(ui->lineEdit_ip->text())
                                  .arg(ui->comboBox_port->currentText())
                                  .arg(info));
}
/** tcp槽 返回tcp错误.
 * @brief SCTcpToolWidget::receiveTcpError
 * @param error
 */
void SCTcpToolWidget::receiveTcpError(QAbstractSocket::SocketError error)
{
    ui->textEdit_info->append(QString("%1  connect error[%2]: IP:%3:%4")
                                  .arg(_scStatusTcp->getCurrentDateTime())
                                  .arg(error)
                                  .arg(ui->lineEdit_ip->text())
                                  .arg(ui->comboBox_port->currentText()));
    ui->comboBox_port->setEnabled(true);
    ui->pushButton_connect->setText(tr("Connect"));
}

/** 发送.
 * @brief SCTcpToolWidget::on_pushButton_send_clicked
 */
void SCTcpToolWidget::on_pushButton_send_clicked()
{
    if(_scStatusTcp->tcpSocket()
        && _scStatusTcp->tcpSocket()->state()==QAbstractSocket::ConnectedState)
    {
        //报头数据类型.
        uint16_t sendCommand = ui->spinBox_sendCommand->text().toInt();
        //数据区数据.
        QString sendDataStr = ui->textEdit_sendData->toPlainText();
        QByteArray sendData = sendDataStr.toLatin1();
        //Json 区数据
        QByteArray jsonData = ui->textEdit_JSON->toPlainText().toLocal8Bit();
        //发送数据size.
        quint64 sendDataSize = sendData.size();
        //如果数据区是文件直接打开读取发送.
        if(!sendDataStr.isEmpty()){
            QFile file(sendDataStr);
            if(file.exists()){//如果数据区是文件.
                if(file.open(QIODevice::ReadOnly)){
                    sendData = file.readAll();
                    sendDataSize = sendData.size();
                    qDebug()<<"sendData(zip file): size"<<sendDataSize;
                }
                file.close();
            }
        }
        //序号.
        uint16_t number = ui->spinBox_number->value();
        uint8_t byte15 = ui->spinBox_byte15->value();
        //清理接收数据区域.
        ui->textEdit_revData->clear();
        //发送数据.
        if(!_scStatusTcp->writeTcpData(sendCommand,jsonData,sendData,number,byte15)){
            slotPrintInfo(tr("<font color=\"red\">"
                             "%1--------- Send error----------\n"
                             "Type:%2  \n"
                             "Error: %3"
                             "</font> ")
                              .arg(_scStatusTcp->getCurrentDateTime())
                              .arg(sendCommand)
                              .arg(_scStatusTcp->lastError()));
        }
    }else{
        //FIX </font> 后面的空格是一定要的，不然会串色.
        ui->textEdit_info->append(tr(" <font color=\"red\">Unconnected:%1</font> ").arg(ui->lineEdit_ip->text()));
    }
}
/** 发送后，响应.
 * @brief SCTcpToolWidget::slotChangedText
 * @param isOk 是否正常返回.
 * @param revCommand 返回的数据类型.
 * @param revData 返回的数据.
 * @param revHex 返回hex.
 * @param number 序号.
 * @param msTime 发送->返回时间 单位：ms.
 */
void SCTcpToolWidget::slotChangedText(bool isOk,int revCommand,
                                      QByteArray revData,QByteArray revHex,
                                      int number,int msTime)
{
    if(isOk){
        int dataSize = 0;
        dataSize = revData.size();
        if(ui->checkBox_revHex->isChecked()){//16进制显示.
            //            dataSize = revHex.size();
            ui->textEdit_revData->setText(_scStatusTcp->hexToQString(revHex));
        }else{//文本显示.
            //            dataSize = revData.size();
            ui->textEdit_revData->setText(QString(revData));
        }
        ui->label_revText->setText(QString("Receive type: %1 (0x%2) \t\n\n"
                                           "Number: %4 (0x%5)\t\n\n"
                                           "Waste time: %6 ms \t\n\n"
                                           "Data Size: %7")
                                       .arg(revCommand)
                                       .arg(QString::number(revCommand,16))
                                       .arg(number)
                                       .arg(QString::number(number,16))
                                       .arg(msTime)
                                       .arg(dataSize));
        //保存到SeerReceive.temp文件.
        if(ui->checkBox_saveFile->isChecked()){
            QFile file("./SeerReceive.temp");
            if(file.open(QIODevice::WriteOnly)){
                file.write(revData);
            }else{
                slotPrintInfo(tr(" <font color=\"red\">Open SeerReceive.temp failed</font> "));
            }
            file.close();
        }
    }else{

        slotPrintInfo(tr(" <font color=\"red\">"
                         "%1--------- Receive error----------\n"
                         "Type:%2  \n"
                         "Error: %3"
                         "</font> ")
                          .arg(_scStatusTcp->getCurrentDateTime())
                          .arg(revCommand)
                          .arg(_scStatusTcp->lastError()));

        ui->textEdit_revData->setText(QString(revData));
        ui->label_revText->setText(QString("Receive error: %1 \t\n")
                                       .arg(_scStatusTcp->lastError()));
    }
}
/** 打印信息.
 * @brief SCTcpToolWidget::slotPrintInfo
 * @param info
 */
void SCTcpToolWidget::slotPrintInfo(QString info)
{
    ui->textEdit_info->append(info);
}

/** 清空textEdit_info数据.
 * @brief SCTcpToolWidget::on_pushButton_clearInfo_clicked
 */
void SCTcpToolWidget::on_pushButton_clearInfo_clicked()
{
    //    if(ui->textEdit_info->document()){
    //        ui->textEdit_info->document()->clear();
    //    }
    ui->textEdit_info->clear();
}

/** 自动滚动.
 * @brief SCTcpToolWidget::slotAutomaticallyScroll
 */
void SCTcpToolWidget::slotAutomaticallyScroll()
{
    if(ui->checkBox_automatically->isChecked()){
        QTextEdit *textedit = (QTextEdit*)sender();
        if(textedit){
            QTextCursor cursor = textedit->textCursor();
            cursor.movePosition(QTextCursor::End);
            textedit->setTextCursor(cursor);
        }
    }
}

void SCTcpToolWidget::on_pushButton_zipFile_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("File"), ".", tr("File(*.*)"));
    if (filePath.isEmpty()){
        return;
    }
    ui->textEdit_sendData->setText(filePath);
}
//是否开启超时.
void SCTcpToolWidget::on_checkBox_timeOut_clicked(bool checked)
{
    if(checked){
        _scStatusTcp->setTimeOut(ui->spinBox_timeOut->value());
    }else{
        _scStatusTcp->setTimeOut(0);
    }
}

void SCTcpToolWidget::on_checkBox_queryTime_clicked(bool checked)
{
    if(checked){
        if(!_queryTimer){
            _queryTimer = new QTimer(this);
            connect(_queryTimer,&QTimer::timeout,this,[=](){
                on_pushButton_send_clicked();
            });
        }
        _queryTimer->stop();
        _queryTimer->start(ui->spinBox_queryTime->value());
    }else{
        if(_queryTimer && _queryTimer->isActive()){
            _queryTimer->stop();
        }
    }
}

void SCTcpToolWidget::on_pushButton_openFolder_clicked()
{
    QString filePath = QCoreApplication::applicationDirPath();
    QDesktopServices::openUrl(QString("file:///%1").arg(filePath));
}

void SCTcpToolWidget::on_toolButton_en_clicked()
{
    if(_translator){
        qApp->removeTranslator(_translator);
        delete _translator;
        _translator = Q_NULLPTR;
    }
    ui->retranslateUi(this);
}

void SCTcpToolWidget::on_toolButton_ch_clicked()
{
    if(!_translator){
        _translator = new QTranslator(this);
        if(_translator->load(QString(":/resource/Ch.qm")))
            qApp->installTranslator(_translator);
    }
    ui->retranslateUi(this);
}
