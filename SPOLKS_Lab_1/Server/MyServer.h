#pragma once

#include <QWidget>
#include <QFile>
#include <QtMath>
#include <QAbstractSocket>

#include <QTcpServer>
#include <QTextEdit>
#include <QTcpSocket>
#include <QFile>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>
#include <QUdpSocket>
#include <QTime>

class MyServer : public QWidget {
Q_OBJECT
private:
    QTcpServer* pTcpServer;
    QTcpSocket* pTcpSocket;
    QUdpSocket* pUdpSocket;
    QTextEdit*  pTxt;
    QLineEdit*  pTxtTcpPort;
    QLineEdit*  pTxtUdpPort;
    quint16     nextBlockSize;
    qint64      blockNumber = 0;
    int         countClients = 0;
    int         curClientId  = 100500;
    QFile       file;
    QString     fileName;
    qint64      offset;
    qint64      blockSize;
    qint64      dataSize;
    QByteArray  buffer;
    QPushButton * bListen;
    QPushButton * bResume;
    QPushButton * bBind;
    QPushButton * bUnbind;
    QTimer      * aliveTimer;
    int         aliveCounter = 0;
    QHostAddress udpSenderAddress;
    quint16      udpSenderPort;
    qint64      baseBlockSize = 64000;
    QVector<qint64>     acks;
    int         windowSize = 4;
    int         ackCounter = 0;
    int         rrt_time;
    QTime       time;
    QTimer      * sendingTimer;
    bool        boost = true;

    enum MsgType {
        Sync,
        Echo,
        Time,
        Close,
        Download,
        Msg,
        Data,
        DataAck,
        DataAnonce,
        DataRequest,
        DownloadAck,
        Alive,
        AckAlive
    };
    enum SocketType {
        TCP,
        UDP
    };

private:
    void sendMsg(SocketType socketType, MsgType type, QList<QVariant> args = QList<QVariant>());
    void processRecivedData(SocketType soketType, QDataStream &in);
public:
    MyServer(QWidget* pwgt = 0);

public slots:
    virtual void slotNewConnection();
            void slotDisconnected();
            void slotListen();
            void slotResume();
            void slotConnectionStateChanged(QAbstractSocket::SocketState state);
            void slotAboutToClose();
            void slotAlive();
            void slotBind();
            void slotUnbind();
            void slotReadTcpSocket();
            void slotReadUdpSocket();
            void slotSendData();
};

