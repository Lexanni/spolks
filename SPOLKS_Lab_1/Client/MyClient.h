#pragma once

#include <QWidget>
#include <QTcpSocket>
#include <QLabel>
#include <QList>
#include <QBuffer>
#include <QtMath>
#include <QTime>
#include <QProgressBar>

class QTextEdit;
class QLineEdit;

// ======================================================================
class MyClient : public QWidget {
Q_OBJECT
private:
    QTcpSocket* m_pTcpSocket;
    QTextEdit*  m_ptxtInfo;
    QLineEdit*  m_ptxtInput;
    QLineEdit*  m_ptxtIp;
    QLineEdit*  m_ptxtPort;
    quint16     m_nNextBlockSize;
    qint32      id = 0;
    qint64      fileSize;
//    qint64      resiveBytes;
    QString     fileName;
    QByteArray  buffer;
    const qint64 blockSize = 64000;

    QLabel *    labelSpeed;
    QProgressBar * progressBar;
    QTime       time;


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
        DownloadAck
    };


public:
    MyClient(QWidget* pwgt = 0) ;

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
};
