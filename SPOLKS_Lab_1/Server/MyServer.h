#pragma once

#include <QWidget>
#include <QFile>
#include <QtMath>

class QTcpServer;
class QTextEdit;
class QTcpSocket;
class QFile;
class QLineEdit;

class MyServer : public QWidget {
Q_OBJECT
private:
    QTcpServer* m_ptcpServer;
    QTcpSocket* pClientSocket;
    QTextEdit*  m_ptxt;
    QLineEdit*  m_ptxtPort;
    quint16     m_nNextBlockSize;
    int         countClients = 0;
    int         curClientId  = 100500;
    QFile       file;
    QString     fileName;
    QByteArray  buffer;

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

private:
    void sendToClient(MsgType type, QList<QVariant> args = QList<QVariant>());

public:
    MyServer(QWidget* pwgt = 0);

public slots:
    virtual void slotNewConnection();
            void slotReadClient();
            void hDisconnected();
            void slotListen();
};

