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
    int         countClients = 0;
    int         curClientId  = 100500;
    QFile       file;
    QString     fileName;
    QByteArray  buffer;
    QPushButton * bListen;
    QPushButton * bResume;
    QPushButton * bBind;
    QPushButton * bUnbind;
    QTimer      * aliveTimer;
    int         aliveCounter = 0;
    QHostAddress udpSenderAddress;
    quint16      udpSenderPort;

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
};

