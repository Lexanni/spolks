#pragma once

#include <QWidget>
#include <QTcpSocket>
#include <QLabel>
#include <QList>
#include <QBuffer>
#include <QtMath>
#include <QTime>
#include <QTimer>
#include <QProgressBar>
#include <QPushButton>
#include <QShortcut>

class QTextEdit;
class QLineEdit;

// ======================================================================
class MyClient : public QWidget {
Q_OBJECT
private:
    QTcpSocket* pTcpSocket;
    QTextEdit*  pTxtInfo;
    QLineEdit*  pTxtInput;
    QLineEdit*  pTxtIp;
    QLineEdit*  pTxtPort;
    quint16     nextBlockSize;
    qint32      id = 0;
    qint64      fileSize;
//    qint64      resiveBytes;
    QString     fileName;
    QByteArray  buffer;
    const qint64 blockSize = 64000;

    QLabel *    labelSpeed;
    QProgressBar * progressBar;
    QTime       time;
    QTimer      * aliveTimer;
    int         aliveCounter = 0;

    QPushButton* bConnect;
    QPushButton* bDisconnect;

    QString     options_file_name = "client_options";
    QList<QString> listLastComands;
    int         cur_command;
    QShortcut   *pKeyUp;
    QShortcut   *pKeyDown;


    enum MsgType {
        Sync,
        Echo,
        Time,
        Close,
        Download,
        Msg,
        Data,
        DataAnonce,
        DataRequest,
        DownloadAck,
        Alive,
        AckAlive
    };


public:
    MyClient(QWidget* pwgt = 0);
    ~MyClient();

signals:
    void downloadBegin();
    void downloadFinished();

private slots:
    void slotReadyRead();
    void slotError(QAbstractSocket::SocketError);
    void slotSendToServer(MsgType type, QList<QVariant> args = QList<QVariant>());
    void parseInput();
    void slotConnected();
    void slotConnectToHost();
    void slotDisconnectFromHost();
    void slotConnectionStateChanged(QAbstractSocket::SocketState state);
    void slotAlive();
    void slotListLastCommandsStepUp();
    void slotListLastCommandsStepDown();
};
