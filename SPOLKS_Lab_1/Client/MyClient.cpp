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
    pTxtInput = new QComboBox;
    pTxtInput->addItems({
                            "DOWNLOAD pt.exe",
                            "DOWNLOAD linux.rar",
                            "DOWNLOAD easy.zip"
                            "ECHO Hello!",
                            "TIME",
                        });
    pTxtInput->setEditable(true);
    pTxtInput->setEnabled(false);

    connect(pTxtInput, SIGNAL(returnPressed()),
            this,        SLOT(parseInput()));
    pTxtInfo->setReadOnly(true);

    QPushButton* pcmd = new QPushButton("&Send");
    connect(pcmd, SIGNAL(clicked()), SLOT(parseInput()));
    // Speed
    labelSpeed = new QLabel("<H3>Speed</H3>");
    progressBar = new QProgressBar();
    progressBar->setMaximum(100);
    progressBar->setMinimum(0);
    progressBar->setValue(0);
    // TCP
    pTxtTcpIp   = new QComboBox;
    pTxtTcpIp->addItems({"localhost", "127.0.0.1"});
    pTxtTcpIp->setEditable(true);
    pTxtTcpPort = new QComboBox;
    pTxtTcpPort->addItems({"2323", "12345"});
    pTxtTcpPort->setEditable(true);
    bConnect    = new QPushButton("Connect");
    bDisconnect = new QPushButton("Disconnect");
    bDisconnect->setEnabled(false);

    QHBoxLayout* tcpPortLayout = new QHBoxLayout();
    tcpPortLayout->addWidget(new QLabel("TCP: Server IP:"), 1);
    tcpPortLayout->addWidget(pTxtTcpIp, 1);
    tcpPortLayout->addWidget(new QLabel("Server port:"), 1);
    tcpPortLayout->addWidget(pTxtTcpPort, 1);
    tcpPortLayout->addWidget(bConnect, 2);
    tcpPortLayout->addWidget(bDisconnect, 1);

    connect(bConnect,    SIGNAL(clicked()), this, SLOT(slotConnectToHost()));
    connect(bDisconnect, SIGNAL(clicked()), this, SLOT(slotDisconnectFromHost()));

    // UDP
    pTxtUdpIp     = new QComboBox;
    pTxtUdpIp->addItems({"127.0.0.1", "localhost", "10.0.0.1", "10.0.0.2"});
    pTxtUdpIp->setEditable(true);
    pTxtUdpPort   = new QComboBox;
    pTxtUdpPort->addItems({"12345", "10000"});
    pTxtUdpPort->setEditable(true);
    pTxtUdpMyPort = new QComboBox;
    pTxtUdpMyPort->addItems({"5454"});
    pTxtUdpMyPort->setEditable(true);
    bBind    = new QPushButton("Bind");
    bUnbind  = new QPushButton("Unbind");
    bUnbind->setEnabled(false);

    QHBoxLayout* udpPortLayout = new QHBoxLayout();
    udpPortLayout->addWidget(new QLabel("UDP: Server IP:"), 1);
    udpPortLayout->addWidget(pTxtUdpIp, 1);
    udpPortLayout->addWidget(new QLabel("Server port:"), 1);
    udpPortLayout->addWidget(pTxtUdpPort, 1);
    udpPortLayout->addWidget(new QLabel("My port: "));
    udpPortLayout->addWidget(pTxtUdpMyPort, 1);
    udpPortLayout->addWidget(bBind, 1);
    udpPortLayout->addWidget(bUnbind, 1);

    connect(bBind  , SIGNAL(clicked()), this, SLOT(slotBind())  );
    connect(bUnbind, SIGNAL(clicked()), this, SLOT(slotUnbind()));

    bProtToogle = new QPushButton("UDP");
    connect(bProtToogle, SIGNAL(clicked()), this, SLOT(slotToogleProt()));
    pMTU = new QSpinBox;
    pMTU->setRange(1000, 65000);
    pMTU->setValue(65000);
    pMTU->setSingleStep(1000);

    QHBoxLayout* paramsLayout = new QHBoxLayout;
    paramsLayout->addWidget(new QLabel("Protocol:"));
    paramsLayout->addWidget(bProtToogle, 1);
    paramsLayout->addWidget(new QLabel("MTU:"));
    paramsLayout->addWidget(pMTU, 1);

    //Layout setup
    QVBoxLayout* pvbxLayout = new QVBoxLayout;
    pvbxLayout->addWidget(new QLabel("<H1>Client</H1>"));
    pvbxLayout->addLayout(tcpPortLayout);
    pvbxLayout->addLayout(udpPortLayout);
    pvbxLayout->addLayout(paramsLayout);
    pvbxLayout->addWidget(pTxtInfo);
    pvbxLayout->addWidget(labelSpeed);
    pvbxLayout->addWidget(progressBar);
    pvbxLayout->addWidget(pTxtInput);
    pvbxLayout->addWidget(pcmd);
    setLayout(pvbxLayout);

    //Shortcut
//    pKeyUp = new QShortcut(Qt::Key_Up, pTxtInput);
//    pKeyDown = new QShortcut(Qt::Key_Down, pTxtInput);
//    pKeyEnter = new QShortcut(Qt::Key_Enter, pTxtInput);

//    connect(pKeyUp,   SIGNAL(activated()), this, SLOT(slotListLastCommandsStepUp()  ));
//    connect(pKeyDown, SIGNAL(activated()), this, SLOT(slotListLastCommandsStepDown()));
//    connect(pKeyEnter, SIGNAL(activated()), this, SLOT(parseInput()));

    QFile file(options_file_name);
    if(file.open(QIODevice::ReadOnly)) {
        pTxtInfo->append("Client ID was readed from options file.");
        file.read((char *)&id, sizeof(id));
        file.close();
    }

    aliveTimer = new QTimer();
    aliveTimer->setInterval(2000);
    connect(aliveTimer, SIGNAL(timeout()), this, SLOT(slotAlive()));
    this->adjustSize();
}
void MyClient::slotBind()
{
    int nPort = pTxtUdpMyPort->currentText().toInt();
    pUdpSocket = new QUdpSocket(this);
    pUdpSocket->bind(nPort);
    udpServerAddress = QHostAddress(pTxtUdpIp->currentText());
    udpServerPort    = pTxtUdpPort->currentText().toInt();
    connect(pUdpSocket, SIGNAL(readyRead()),
            this, SLOT(slotReadUdpSocket()));

    bBind->setEnabled(false);
    bUnbind->setEnabled(true);
    pTxtUdpPort->setEnabled(false);
    pTxtUdpMyPort->setEnabled(false);
    pTxtInput->setEnabled(true);
}
void MyClient::slotUnbind()
{
    disconnect(pUdpSocket, SIGNAL(readyRead()),
            this, SLOT(slotReadUdpSocket()));
    pUdpSocket->deleteLater();
    bBind->setEnabled(true);
    bUnbind->setEnabled(false);
    pTxtUdpPort->setEnabled(true);
    pTxtUdpMyPort->setEnabled(true);
    pTxtInput->setEnabled(false);
}
MyClient::~MyClient()
{
    QFile::remove(options_file_name);
}
void MyClient::slotReadTcpSocket()
{
    QTcpSocket* pSocket = (QTcpSocket*)sender();
    disconnect(pSocket, SIGNAL(readyRead()), this, SLOT(slotReadTcpSocket()));
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
        processRecivedData(SocketType::TCP, in);
        nextBlockSize = 0;
        if (pTcpSocket->bytesAvailable() <= 0)
            break;
    }
    connect(pSocket, SIGNAL(readyRead()), this, SLOT(slotReadTcpSocket()));
}
void MyClient::slotReadUdpSocket()
{
//    qDebug () << "slotReadUdpSocket()";
    while (pUdpSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = pUdpSocket->receiveDatagram();
//        udpSenderAddress = datagram.destinationAddress();
//        udpSenderPort    = datagram.destinationPort();
        QDataStream in(datagram.data());
        in.setVersion(QDataStream::Qt_5_9);
        quint16 size;
        in >> size;
        processRecivedData(SocketType::UDP, in);
    }
}
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
void MyClient::sendMsg(SocketType socketType, MsgType type, QList<QVariant> args)
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
                QString fileName = args.first().toString();
                qint64  offset = args.at(1).toLongLong();
                qint64  size = args.at(2).toLongLong();
                qint64  mtu = args.at(3).toLongLong();

                out << qint8(MsgType::DataRequest) << fileName << offset << size << mtu;
            }
            break;
        case MsgType::DownloadAck :
            out << qint8(MsgType::DownloadAck);
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
    if(socketType == SocketType::TCP) {
        pTcpSocket->write(arrBlock);
    } else {
        pUdpSocket->writeDatagram(arrBlock, udpServerAddress, udpServerPort);
    }
}
void MyClient::processRecivedData(SocketType soketType, QDataStream &in)
{
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
//            pTxtInput->clear();
            break;
        case MsgType::Time :
            {
            QTime time;
            in >> time;
            pTxtInfo->append("Server: " + time.toString());
//            pTxtInput->clear();
            break;
            }
        case MsgType::Msg :
            in >> s;
            pTxtInfo->append("Server: " + s);
//            pTxtInput->clear();
            break;
        case MsgType::DataAnonce :
            {
                in >> fileName;
                in >> fileSize;
                // qDebug() << "Recived DataAnonce. FileName: " << fileName << " size: " << fileSize << endl;
                pTxtInfo->append("Server: " + s + " " + QString::number(fileSize) + " bytes");
//                pTxtInput->clear();
                recivedBytes = 0;
                buffer.clear();
                buffer.resize(fileSize);
                labelSpeed->setText("<H3>" + QString::number(recivedBytes) +
                                    "/" + QString::number(fileSize) + " bytes. Speed: " +
                                    QString::number(0.0, 'f', 2) + " MB/s </H3>");
                sendMsg(soketType, MsgType::DataRequest, {fileName, 0, fileSize, (qint64)pMTU->value()});
                time.start();
            }
            break;
        case MsgType::Data :
            {
                int t = time.elapsed();
                qint64 offset;
                qint64 size;
                in >> offset >> size;
//                qDebug() << "offset = " << offset << "size = " << size << endl;
                recivedBytes += size;
                int val = recivedBytes * 100 / fileSize;
                if(val != progressBarValue){
                    progressBarValue = val;
                    progressBar->setValue(val);
                    labelSpeed->setText("<H3>" + QString::number(recivedBytes) + "/" + QString::number(fileSize) +
                                        " bytes. Speed: " + QString::number((double)recivedBytes/(t * 1000), 'f', 2) +
                                        " MB/s</H3>");
                }
                QBuffer bufferStream(&buffer);
                bufferStream.open(QIODevice::ReadWrite);
                bufferStream.seek(offset);
                QByteArray b;
                in >> b;
                bufferStream.write(b);
                if(recivedBytes == fileSize) {
//                    qDebug() << "recivedBytes == fileSize";
                    QFile file(fileName);
                    file.open(QIODevice::WriteOnly);
                    file.write(buffer);
                    file.close();
                    buffer.clear();
                    fileName.clear();
                    fileSize = 0;
                    recivedBytes = 0;
                    sendMsg(soketType, MsgType::DownloadAck);
                    break;
                }
//                sendMsg(soketType, MsgType::DataRequest, {fileName,
//                                                        buffer.size(),
//                                                        qMin(fileSize - buffer.size(),
//                                                        blockSize)});
            }
            break;
        case MsgType::Alive :
            sendMsg(SocketType::TCP, MsgType::AckAlive);
            break;
        case MsgType::AckAlive :
            aliveCounter = 0;
            break;
        default:
            pTxtInfo->append("Unknown response");
    }
}
void MyClient::parseInput()
{
    QList<QVariant> args;
    QString s = pTxtInput->currentText();
//    listLastComands.append(s);
//    cur_command = listLastComands.size() - 1;
//    pTxtInput->clear();
    int i = s.indexOf(" ");
    QString cmd = s.mid(0, i);
    args.append(s.mid(i + 1));

    if(cmd == "ECHO")
        sendMsg(currentSocketType, MsgType::Echo, args);
    else if (cmd == "TIME")
        sendMsg(currentSocketType, MsgType::Time);
    else if (cmd == "CLOSE")
        sendMsg(currentSocketType, MsgType::Close);
    else if (cmd == "DOWNLOAD")
        sendMsg(currentSocketType, MsgType::Download, args);
    else
        pTxtInfo->append("Unknown command!");
}
void MyClient::slotConnected()
{
    // m_ptxtInfo->append("Received the connected() signal");
    sendMsg(SocketType::TCP, MsgType::Sync);
    // aliveTimer->start();
}
void MyClient::slotConnectToHost()
{
    pTcpSocket = new QTcpSocket(this);
    QString strHost = pTxtTcpIp->currentText();
    int nPort = pTxtTcpPort->currentText().toInt();
//    qDebug () << "strHost: " << strHost << ", port: " << nPort;
    pTcpSocket->connectToHost(strHost, nPort);
    connect(pTcpSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
            this, SLOT(slotConnectionStateChanged(QAbstractSocket::SocketState)));
    connect(pTcpSocket, SIGNAL(connected()), SLOT(slotConnected()));
    connect(pTcpSocket, SIGNAL(readyRead()), SLOT(slotReadTcpSocket()));
    connect(pTcpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
            this,         SLOT(slotError(QAbstractSocket::SocketError))
           );
    bConnect->setEnabled(false);
    pTxtTcpIp->setEnabled(false);
    pTxtTcpPort->setEnabled(false);
    bDisconnect->setEnabled(true);
    pTxtInput->setEnabled(true);
}
void MyClient::slotDisconnectFromHost()
{
    aliveTimer->stop();
    pTcpSocket->disconnectFromHost();
    if (pTcpSocket->state() == QAbstractSocket::ConnectedState) {
        pTcpSocket->waitForDisconnected();
    }
    delete pTcpSocket;
    bConnect->setEnabled(true);
    bDisconnect->setEnabled(false);
    pTxtInput->setEnabled(false);
    pTxtTcpIp->setEnabled(true);
    pTxtTcpPort->setEnabled(true);
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
    sendMsg(SocketType::TCP, MsgType::Alive);
}
//void MyClient::slotListLastCommandsStepUp()
//{
//    if(listLastComands.empty())
//        return;
//    pTxtInput->setText(listLastComands[cur_command]);
//    if(cur_command)
//        cur_command--;
//}
//void MyClient::slotListLastCommandsStepDown()
//{
//    if(listLastComands.empty())
//        return;
//    pTxtInput->setText(listLastComands[cur_command]);
//    if(cur_command != listLastComands.size() - 1)
//        cur_command++;
//}
void MyClient::slotToogleProt()
{
    if(currentSocketType == SocketType::TCP){
        bProtToogle->setText("UDP");
        currentSocketType = SocketType::UDP;
    } else {
        bProtToogle->setText("TCP");
        currentSocketType = SocketType::TCP;
    }
}
