#pragma once

#include <QWidget>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QLabel>
#include <QList>
#include <QBuffer>
#include <QtMath>
#include <QTime>
#include <QTimer>
#include <QProgressBar>
#include <QPushButton>
#include <QShortcut>
#include <QNetworkDatagram>
#include <QGroupBox>
#include <QComboBox>
#include <QSpinBox>

class QTextEdit;
class QLineEdit;

// ======================================================================
class MyClient : public QWidget {
Q_OBJECT
private:
    QTcpSocket* pTcpSocket;
    QUdpSocket* pUdpSocket;
    QTextEdit*  pTxtInfo;
    QComboBox*  pTxtInput;
    QComboBox*  pTxtTcpIp;
    QComboBox*  pTxtTcpPort;
    QComboBox*  pTxtUdpIp;
    QComboBox*  pTxtUdpPort;
    QComboBox*  pTxtUdpMyPort;
    QSpinBox*   pMTU;
    quint16     nextBlockSize;
    qint32      id = 0;
    qint64      fileSize;
    qint64      recivedBytes;
    int         progressBarValue = 0;
    QString     fileName;
    QByteArray  buffer;
    const qint64 blockSize = 65000;
    QHostAddress udpServerAddress;
    quint16      udpServerPort;
    QHostAddress udpMyAddress;
    quint16      udpMyPort;

    QLabel *    labelSpeed;
    QProgressBar * progressBar;
    QTime       time;
    QTimer      * aliveTimer;
    int         aliveCounter = 0;

    QPushButton* bConnect;
    QPushButton* bDisconnect;
    QPushButton * bBind;
    QPushButton * bUnbind;
    QPushButton * bProtToogle;

    QString     options_file_name = "client_options";
//    QList<QString> listLastComands;
//    int         cur_command;
//    QShortcut   *pKeyUp;
//    QShortcut   *pKeyDown;
//    QShortcut    *pKeyEnter;


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
    SocketType   currentSocketType = SocketType::UDP;
public:
    MyClient(QWidget* pwgt = 0);
    ~MyClient();

signals:
    void downloadBegin();
    void downloadFinished();

private slots:
    void slotReadTcpSocket();
    void slotReadUdpSocket();
    void slotBind();
    void slotUnbind();
    void slotError(QAbstractSocket::SocketError);
    void sendMsg(SocketType socketType, MsgType type, QList<QVariant> args = QList<QVariant>());
    void processRecivedData(SocketType soketType, QDataStream &in);
    void parseInput();
    void slotConnected();
    void slotConnectToHost();
    void slotDisconnectFromHost();
    void slotConnectionStateChanged(QAbstractSocket::SocketState state);
    void slotAlive();
//    void slotListLastCommandsStepUp();
//    void slotListLastCommandsStepDown();
    void slotToogleProt();
};
