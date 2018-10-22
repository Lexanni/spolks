#include <QtWidgets>
#include <QtGui>
#include <QFile>
#include <QDebug>
#include "MyClient.h"

// ----------------------------------------------------------------------
MyClient::MyClient(QWidget*       pwgt /*=0*/
                  ) : QWidget(pwgt)
                    , nextBlockSize(0)
{
    pTxtInfo  = new QTextEdit;
    pTxtInput = new QLineEdit;
    pTxtInput->setEnabled(false);

    connect(pTxtInput, SIGNAL(returnPressed()),
            this,        SLOT(parseInput())
           );
    pTxtInfo->setReadOnly(true);

    QPushButton* pcmd = new QPushButton("&Send");
    connect(pcmd, SIGNAL(clicked()), SLOT(parseInput()));

    labelSpeed = new QLabel("<H3>Speed</H3>");
    progressBar = new QProgressBar();
    progressBar->setMaximum(100);
    progressBar->setMinimum(0);
    progressBar->setValue(0);

    pTxtIp   = new QLineEdit("localhost");
    pTxtPort = new QLineEdit("2323");
    bConnect    = new QPushButton("Connect");
    bDisconnect = new QPushButton("Disconnect");
    bDisconnect->setEnabled(false);

    connect(bConnect, SIGNAL(clicked()), this, SLOT(slotConnectToHost()));
    connect(bDisconnect, SIGNAL(clicked()), this, SLOT(slotDisconnectFromHost()));

    QHBoxLayout* addressLayout = new QHBoxLayout();
    addressLayout->addWidget(new QLabel("IP: "));
    addressLayout->addWidget(pTxtIp);
    addressLayout->addWidget(new QLabel("Port: "));
    addressLayout->addWidget(pTxtPort);
    addressLayout->addWidget(bConnect);
    addressLayout->addWidget(bDisconnect);


    //Layout setup
    QVBoxLayout* pvbxLayout = new QVBoxLayout;
    pvbxLayout->addWidget(new QLabel("<H1>Client</H1>"));
    pvbxLayout->addLayout(addressLayout);
    pvbxLayout->addWidget(pTxtInfo);
    pvbxLayout->addWidget(labelSpeed);
    pvbxLayout->addWidget(progressBar);
    pvbxLayout->addWidget(pTxtInput);
    pvbxLayout->addWidget(pcmd);
    setLayout(pvbxLayout);

    QFile file(options_file_name);
    if(file.open(QIODevice::ReadOnly)) {
        pTxtInfo->append("Client ID was readed from options file.");
        file.read((char *)&id, sizeof(id));
        file.close();
    }

    aliveTimer = new QTimer(this);
    connect(aliveTimer, SIGNAL(timeout()), this, SLOT(slotAlive()));

}

MyClient::~MyClient()
{
    QFile::remove(options_file_name);
}
// ----------------------------------------------------------------------
void MyClient::slotReadyRead()
{

    QTcpSocket* pSocket = (QTcpSocket*)sender();
    disconnect(pSocket, SIGNAL(readyRead()), this, SLOT(slotReadyRead()));
    QDataStream in(pSocket);
    in.setVersion(QDataStream::Qt_5_9);
    //qDebug () << "read buffer size = " << pSocket->readBufferSize() << endl;
    forever {
        if (!nextBlockSize) {
            if (pTcpSocket->bytesAvailable() < (int)sizeof(quint16)) {
                break;
            }
            in >> nextBlockSize;
        }
        if (pTcpSocket->bytesAvailable() < nextBlockSize) {
            break;
        }

    QString s;
    qint8 respType;
    in >> respType;
    switch (respType)
    {
        case MsgType::Sync :
            {
                int resiveId;
                in >> resiveId;
                qDebug () << "resived Id = " << resiveId << endl;
                if(id != resiveId) {
                    id = resiveId;
                    pTxtInfo->append("Server has assigned you new client ID: " + QString::number(id));
                }
                else
                    pTxtInfo->append("The server has confirmed your client ID: " + QString::number(id));

                QFile file(options_file_name);
                if(!file.open(QIODevice::WriteOnly)) {
                    pTxtInfo->append("Cannot write client id to options file. File open error.");
                }

                file.write((char *)&id, sizeof(id));
                file.close();
            }
            break;
        case MsgType::Echo :
            in >> s;
            pTxtInfo->append("Server: " + s);
            pTxtInput->clear();
            break;
        case MsgType::Time :
            {
            QTime time;
            in >> time;
            pTxtInfo->append("Server: " + time.toString());
            pTxtInput->clear();
            break;
            }
        case MsgType::Msg :
            in >> s;
            pTxtInfo->append("Server: " + s);
            pTxtInput->clear();
            break;
        case MsgType::DataAnonce :
            {
                in >> fileName;
                in >> fileSize;
                // qDebug() << "Recived DataAnonce. FileName: " << fileName << " size: " << fileSize << endl;
                pTxtInfo->append("Server: " + s + " " + QString::number(fileSize) + " bytes");
                pTxtInput->clear();
                slotSendToServer(MsgType::DataRequest, {fileName, 0, qMin(fileSize, blockSize)});
                time.start();
            }
            break;
        case MsgType::Data :
            {
                qint64 offset;
                qint64 size;
                in >> offset >> size;
                // qDebug() << "Recived data. Size: " << size << endl;



                QBuffer bufferStream(&buffer);
                bufferStream.open(QIODevice::Append);
                // bufferStream.seek(offset);
                QByteArray b;
                in >> b;
                bufferStream.write(b);
                progressBar->setValue((offset + size) * 100 / fileSize);
                if(buffer.size() == fileSize) {
                    int t = time.elapsed();
                    labelSpeed->setText("<H3>Speed: " + QString::number(fileSize/t) + " KB/s</H3>");
                    QFile file(fileName);
                    file.open(QIODevice::WriteOnly);
                    file.write(buffer);
                    file.close();
                    buffer.clear();
                    fileName.clear();
                    fileSize = 0;
                    slotSendToServer(MsgType::DownloadAck);

                    break;
                }
                slotSendToServer(MsgType::DataRequest, {fileName,
                                                        buffer.size(),
                                                        qMin(fileSize - buffer.size(),
                                                        blockSize)});
            }
            break;
        case MsgType::Alive :
            slotSendToServer(MsgType::AckAlive);
            break;
        case MsgType::AckAlive :
            aliveCounter = 0;
            break;
        default:
            pTxtInfo->append("Unknown response");
    }

    nextBlockSize = 0;

    break;
    }
    connect(pSocket, SIGNAL(readyRead()), this, SLOT(slotReadyRead()));
}
// ----------------------------------------------------------------------
void MyClient::slotError(QAbstractSocket::SocketError err)
{
    QString strError = 
        "Error: " + (err == QAbstractSocket::HostNotFoundError ? 
                     "The host was not found." :
                     err == QAbstractSocket::RemoteHostClosedError ? 
                     "The remote host is closed." :
                     err == QAbstractSocket::ConnectionRefusedError ? 
                     "The connection was refused." :
                     err == QAbstractSocket::NetworkError ?
                     "Network Error" :
                     QString(pTcpSocket->errorString())
                    );
    pTxtInfo->append(strError);
}

void MyClient::slotSendToServer(MyClient::MsgType type, QList<QVariant> args)
{
    QByteArray  arrBlock;
    QDataStream out(&arrBlock, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_9);
    out << quint16(0);

    switch(type)
    {
        case MsgType::Sync :
            out << qint8(MsgType::Sync) << id;
            break;
        case MsgType::Echo :
            out << qint8(MsgType::Echo) << args.first().toString();
            break;
        case MsgType::Time :
            out << qint8(MsgType::Time);
            break;
        case MsgType::Close :
            out << qint8(MsgType::Close);
            break;
        case MsgType::Download :
            out << qint8(MsgType::Download) << args.first().toString();
            break;
        case MsgType::DataRequest :
            {
                QString a1 = args.first().toString();
                qint64  a2 = args.at(1).toLongLong();
                qint64  a3 = args.at(2).toLongLong();

                out << qint8(MsgType::DataRequest) << a1 << a2 << a3;
            }
            break;
        case MsgType::Alive :
            out << qint8(MsgType::Alive);
            break;
        case MsgType::AckAlive :
            out << qint8(MsgType::AckAlive);
            break;
    }

    out.device()->seek(0);
    out << quint16(arrBlock.size() - sizeof(quint16));

    pTcpSocket->write(arrBlock);
    pTxtInput->setText("");
}
// ----------------------------------------------------------------------
void MyClient::parseInput()
{
    QList<QVariant> args;
    QString s = pTxtInput->text();
    int i = s.indexOf(" ");
    QString cmd = s.mid(0, i);
    args.append(s.mid(i + 1));


    if(cmd == "ECHO")
        slotSendToServer(MsgType::Echo, args);
    else if (cmd == "TIME")
        slotSendToServer(MsgType::Time);
    else if (cmd == "CLOSE")
        slotSendToServer(MsgType::Close);
    else if (cmd == "DOWNLOAD")
        slotSendToServer(MsgType::Download, args);
    else
        pTxtInfo->append("Unknown command!");
}

// ------------------------------------------------------------------
void MyClient::slotConnected()
{
    // m_ptxtInfo->append("Received the connected() signal");
    slotSendToServer(MsgType::Sync);
    aliveTimer->start(2000);
}

void MyClient::slotConnectToHost()
{
    pTcpSocket = new QTcpSocket(this);
    QString strHost = pTxtIp->text();
    int nPort = pTxtPort->text().toInt();
//    qDebug () << "strHost: " << strHost << ", port: " << nPort;
    pTcpSocket->connectToHost(strHost, nPort);
    connect(pTcpSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
            this, SLOT(slotConnectionStateChanged(QAbstractSocket::SocketState)));
    connect(pTcpSocket, SIGNAL(connected()), SLOT(slotConnected()));
    connect(pTcpSocket, SIGNAL(readyRead()), SLOT(slotReadyRead()));
    connect(pTcpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
            this,         SLOT(slotError(QAbstractSocket::SocketError))
           );
    bConnect->setEnabled(false);
    pTxtIp->setEnabled(false);
    pTxtPort->setEnabled(false);
    bDisconnect->setEnabled(true);
    pTxtInput->setEnabled(true);
}

void MyClient::slotDisconnectFromHost()
{
    pTcpSocket->disconnectFromHost();
    if (pTcpSocket->state() == QAbstractSocket::ConnectedState) {
        pTcpSocket->waitForDisconnected();
    }
    delete pTcpSocket;
    bConnect->setEnabled(true);
    bDisconnect->setEnabled(false);
    pTxtInput->setEnabled(false);
    pTxtIp->setEnabled(true);
    pTxtPort->setEnabled(true);
}

void MyClient::slotConnectionStateChanged(QAbstractSocket::SocketState state)
{
    switch(state){
    case  QAbstractSocket::UnconnectedState :
        pTxtInfo->append("The socket is not connected.");
//        if (pTcpSocket != nullptr)
//            slotDisconnectFromHost();
        break;
    case QAbstractSocket::HostLookupState :
        pTxtInfo->append("The socket is performing a host name lookup.");
        break;
    case QAbstractSocket::ConnectingState :
        pTxtInfo->append("The socket has started establishing a connection.");
        break;
    case QAbstractSocket::ConnectedState :
        pTxtInfo->append("A connection is established.");
        break;
    case QAbstractSocket::BoundState :
        pTxtInfo->append("The socket is bound to an address and port.");
        break;
    case QAbstractSocket::ClosingState :
        pTxtInfo->append("The socket is about to close (data may still be waiting to be written).");
        break;
    case QAbstractSocket::ListeningState :
        pTxtInfo->append("For internal use only.");
    }
}

void MyClient::slotAlive()
{
    aliveCounter++;
    if(aliveCounter > 3)
    {
        aliveCounter = 0;
        aliveTimer->stop();
        slotDisconnectFromHost();
        return;
    }
    slotSendToServer(MsgType::Alive);
}
