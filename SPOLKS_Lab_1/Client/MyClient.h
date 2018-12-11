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
    quint16     nextBlockSize = 0;
    qint32      id = 0;
    QFile       file;
    qint64      fileSize;
    qint64      recivedBytes;
    qint64      savePoint;
    qint64      savePointStep = (1 << 22);
    qint64      lastRecivedOffset = 0;
    QVector<qint64> recivedBlocks;
    int         progressBarValue = 0;
    QString     fileName;
    QByteArray  buffer;
    qint64      blockSize = 65000;
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
    QPushButton * bContinueDownloading;

    QString     options_file_name = "client_options";
    QString     downloading_options = "d_";
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
        ContinueDownloading,
        Msg,
        Data,
        DataAck,
        DataAnonce,
        DataRequest,
        DownloadAck,
        DownloadFin,
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
    void processRecivedData(SocketType socketType, QDataStream &in);
    void parseInput();
    void slotConnected();
    void slotConnectToHost();
    void slotDisconnectFromHost();
    void slotConnectionStateChanged(QAbstractSocket::SocketState state);
    void slotAlive();
//    void slotListLastCommandsStepUp();
//    void slotListLastCommandsStepDown();
    void slotToogleProt();
    void slotContinueDownloading();
};
