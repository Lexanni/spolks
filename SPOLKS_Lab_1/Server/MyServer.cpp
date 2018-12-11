#include <QtNetwork>
#include <QtWidgets>
#include <QFile>
#include "MyServer.h"

MyServer::MyServer(QWidget* pwgt /*=0*/) : QWidget(pwgt)
{
    pTcpServer = new QTcpServer(this);
    connect(pTcpServer, SIGNAL(newConnection()), this, SLOT(slotNewConnection()));

    pTxt = new QTextEdit;
    pTxt->setReadOnly(true);
    pTxtTcpPort = new QLineEdit("2323");
    pTxtUdpPort = new QLineEdit("12345");
    bListen = new QPushButton("Listen");
    bResume = new QPushButton("Resume");
    bResume->setEnabled(false);
    bBind   = new QPushButton("Bind");
    bUnbind = new QPushButton("Unbind");
    bUnbind->setEnabled(false);

    connect(bListen, SIGNAL(clicked()), this, SLOT(slotListen()));
    connect(bResume, SIGNAL(clicked()), this, SLOT(slotResume()));
    connect(bBind  , SIGNAL(clicked()), this, SLOT(slotBind())  );
    connect(bUnbind, SIGNAL(clicked()), this, SLOT(slotUnbind()));

    QHBoxLayout * tcpPortLayout = new QHBoxLayout();
    tcpPortLayout->addWidget(new QLabel("TCP port: "));
    tcpPortLayout->addWidget(pTxtTcpPort);
    tcpPortLayout->addWidget(bListen);
    tcpPortLayout->addWidget(bResume);

    QHBoxLayout * udpPortLayout = new QHBoxLayout();
    udpPortLayout->addWidget(new QLabel("UDP port: "));
    udpPortLayout->addWidget(pTxtUdpPort);
    udpPortLayout->addWidget(bBind);
    udpPortLayout->addWidget(bUnbind);

    //Layout setup
    QVBoxLayout* pvbxLayout = new QVBoxLayout;    
    pvbxLayout->addWidget(new QLabel("<H1>Server</H1>"));
    pvbxLayout->addLayout(tcpPortLayout);
    pvbxLayout->addLayout(udpPortLayout);
    pvbxLayout->addWidget(pTxt);
    setLayout(pvbxLayout);

    aliveTimer = new QTimer();
    aliveTimer->setInterval(1777);
    sendingTimer = new QTimer();
    sendLostTimer = new QTimer();
    connect(aliveTimer, SIGNAL(timeout()), this, SLOT(slotAlive()));
    connect(sendingTimer, SIGNAL(timeout()), this, SLOT(slotSendData()));
    connect(sendLostTimer, SIGNAL(timeout()), this, SLOT(slotSendLost()));
}
void MyServer::slotNewConnection()
{
    if(countClients == 0){
        pTcpSocket = pTcpServer->nextPendingConnection();
        connect(pTcpSocket, SIGNAL(disconnected()),
                this,       SLOT(slotDisconnected())
               );
        connect(pTcpSocket, SIGNAL(readyRead()),
                this,       SLOT(slotReadTcpSocket())
               );
        connect(pTcpSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
                this,       SLOT(slotConnectionStateChanged(QAbstractSocket::SocketState)));
        connect(pTcpSocket, SIGNAL(aboutToClose()), this, SLOT(slotAboutToClose()));
        countClients++;
        // aliveTimer->start();
        //sendToClient(pClientSocket, "Server Response: Connected!");
    }
    else {
        QTcpSocket * pDiscardSocket = pTcpServer->nextPendingConnection();
        //sendToClient(pDiscardSocket, "Server Response: Connection rejected.");

        pDiscardSocket->disconnectFromHost();
        if (pDiscardSocket->state() == QAbstractSocket::ConnectedState) {
            pDiscardSocket->waitForDisconnected();
        }
        delete pDiscardSocket;
    }
}
void MyServer::slotReadTcpSocket()
{
    QTcpSocket* pSocket = (QTcpSocket*)sender();
    disconnect(pSocket, SIGNAL(readyRead()), this, SLOT(slotReadTcpSocket()));
    QDataStream in(pSocket);
    in.setVersion(QDataStream::Qt_5_9);
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
void MyServer::sendMsg(SocketType socketType, MsgType type, QList<QVariant> args)
{
    QByteArray  arrBlock;
    QDataStream out(&arrBlock, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_9);
    out << quint16(0);

    switch(type)
    {
        case MsgType::Sync :
            out << qint8(MsgType::Sync) << curClientId;
            break;
        case MsgType::Echo :
            out << qint8(MsgType::Echo) << args.first().toString();
            break;
        case MsgType::Time :
            out << qint8(MsgType::Time) << QTime::currentTime();
            break;
        case MsgType::Close :
            out << qint8(MsgType::Close);
            break;
        case MsgType::Msg :
            out << qint8(MsgType::Msg) << args.first().toString();
            break;
        case MsgType::DownloadFin :
            out << qint8(MsgType::DownloadFin);
            break;
        case MsgType::DataAnonce :
            buffer.clear();
            file.setFileName(args.first().toString());
            if(!file.open(QIODevice::ReadOnly)){
                qDebug () << "File don't open!" << endl;
                return;
            }
            qDebug () << "File opend. Send file info..." << endl;
            out << qint8(MsgType::DataAnonce) << file.fileName() << file.size();
            file.close();
            break;
        case MsgType::Data :
            {
                QBuffer buffStream(&buffer);
                buffStream.open(QIODevice::ReadOnly);
                qint64 offset = args.at(0).toLongLong();
                qint64 size = args.at(1).toLongLong();
                qint64 blockSize = baseBlockSize;
                qint64 blockNumber = (args.size() == 3) ? args.at(2).toLongLong() : 0;
                // qDebug() << "blockNumber = " << blockNumber << "args.size() = " << args.size();
                qint64 end = qMin(offset + size, (qint64)buffer.size());
                while(offset < end)
                {
                    blockSize = (end - offset > blockSize) ? blockSize : (end - offset);
                    QByteArray  arrBlock;
                    QDataStream out(&arrBlock, QIODevice::WriteOnly);
                    out.setVersion(QDataStream::Qt_5_9);
                    out << quint16(0);
                    out << qint8(MsgType::Data) << offset << blockSize << blockNumber;

                    buffStream.seek(offset);
                    out << buffStream.read(blockSize);
                    out.device()->seek(0);
                    out << quint16(arrBlock.size() - sizeof(quint16));
                    if(socketType == SocketType::TCP) {
                        pTcpSocket->write(arrBlock);
                    } else {
                        pUdpSocket->writeDatagram(arrBlock, udpSenderAddress, udpSenderPort);
                    }
                    offset += baseBlockSize;
                }
                // pTcpSocket->flush();
                return;
            }
            break;
        case MsgType::Alive :
            out << qint8(MsgType::Alive);
            break;
        case MsgType::AckAlive :
            out << qint8(MsgType::AckAlive);
            break;
        // default:
    }

    out.device()->seek(0);
    out << quint16(arrBlock.size() - sizeof(quint16));
    if(socketType == SocketType::TCP) {
        pTcpSocket->write(arrBlock);
        pTcpSocket->flush();
    } else {
        pUdpSocket->writeDatagram(arrBlock, udpSenderAddress, udpSenderPort);
    }
}
void MyServer::slotDisconnected()
{
    if(sender() == pTcpSocket){
        // qDebug () << "hDisconnected()" << endl;
        countClients--;
        pTcpSocket->disconnectFromHost();
    }
}
void MyServer::slotListen()
{
    int nPort = pTxtTcpPort->text().toInt();
    if (!pTcpServer->listen(QHostAddress::Any, nPort)) {
        QMessageBox::critical(0,
                              "Server Error",
                              "Unable to start the server:"
                              + pTcpServer->errorString()
                             );
        pTcpServer->close();
        return;
    }
    pTxtTcpPort->setEnabled(false);
    bListen->setEnabled(false);
    bResume->setEnabled(true);
    pTxt->append("Listening port " + pTxtTcpPort->text());
}
void MyServer::slotResume()
{
//    if (pClientSocket != nullptr) {
//        delete pClientSocket;
//        pClientSocket = nullptr;
//        countClients--;
//    }
    pTcpServer->close();
    bListen->setEnabled(true);
    bResume->setEnabled(false);
    pTxtTcpPort->setEnabled(true);
    pTxt->append("Resume port " + pTxtTcpPort->text());
}
void MyServer::slotBind()
{
    int nPort = pTxtUdpPort->text().toInt();
    pUdpSocket = new QUdpSocket(this);
    pUdpSocket->bind(QHostAddress::Any, nPort);

    connect(pUdpSocket, SIGNAL(readyRead()),
            this, SLOT(slotReadUdpSocket()));

    bBind->setEnabled(false);
    bUnbind->setEnabled(true);
    pTxtUdpPort->setEnabled(false);
}
void MyServer::slotUnbind()
{
    disconnect(pUdpSocket, SIGNAL(readyRead()),
            this, SLOT(slotReadUdpSocket()));
    pUdpSocket->deleteLater();
    bBind->setEnabled(true);
    bUnbind->setEnabled(false);
    pTxtUdpPort->setEnabled(true);
}
void MyServer::slotReadUdpSocket()
{
//    qDebug () << "slotReadUdpSocket";
    while (pUdpSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = pUdpSocket->receiveDatagram();
        udpSenderAddress = datagram.senderAddress();
        udpSenderPort    = datagram.senderPort();
//        qDebug() << "datagram.destinationAddress()" << udpSenderAddress;
//        qDebug() << "datagram.destinationPort()"    << udpSenderPort;
        QDataStream in(datagram.data());
        in.setVersion(QDataStream::Qt_5_9);
        qint16 size;
        in >> size;
        processRecivedData(SocketType::UDP, in);
    }
}
void MyServer::processRecivedData(SocketType soketType, QDataStream &in)
{
    QString s;
    qint8 reqType;
    in >> reqType;
    switch(reqType)
    {
        case MsgType::Sync :
//            qDebug () << "Sync" << endl;
            pTxt->append("Sync");
            int clientId;
            in >> clientId;
            if(clientId != curClientId)
                curClientId++;
            sendMsg(soketType, MsgType::Sync);
            break;
         case MsgType::Echo :
//            qDebug () << "Echo" << endl;
            in >> s;
            pTxt->append("ECHO " + s);
            sendMsg(soketType, MsgType::Echo, {s});
            break;
        case MsgType::Time :
//            qDebug () << "Time" << endl;
            in >> s;
            pTxt->append("TIME");
            sendMsg(soketType, MsgType::Time);
            break;
        case MsgType::Close :
            slotDisconnected();
            break;
        case MsgType::Download :
//            qDebug () << "Download" << endl;
            in >> s;
            pTxt->append("DOWNLOAD " + s);
            if(QFile::exists(s)){
//                qDebug () << "file " + s + " exists" << endl;
                fileName = s;
                time.restart();
                sendMsg(soketType, MsgType::DataAnonce, {s});
                QFile file(s);
                file.open(QIODevice::ReadOnly);
                buffer = file.readAll();
                file.close();
            }
            else
            {
//                qDebug () << "file " + s + " don't exits!" << endl;
                sendMsg(soketType, MsgType::Msg, {"File don't exist."});
            }
            break;
         case MsgType::ContinueDownloading :
           {
//        qDebug () << "ContinueDownloading";
                QString recivedFileName;
                in >> recivedFileName;
                if(recivedFileName != fileName) {
                    sendMsg(soketType, MsgType::Msg, {"Cannot continue downloading."});
                    break;
                }
                in >> offset >> blockSize;
                int blockNumber = offset/blockSize;
                for(int i = blockNumber; i < acks.size(); i++)
                    acks[i] = 0;
//                if(ackPartLast > blockNumber)
//                    ackPartLast = blockNumber;
                ackPartLast = 0;
                windowSize = 8;
                attemptCounter = 0;
                sendingTimer->start(1);
           }
            break;
         case MsgType::DataRequest :
            {
//               qDebug () << "Reciver DataRequest" << endl;
               rrt_time = time.elapsed();
               // qDebug () << "rrt_time = " << rrt_time << "msec";
               in >> fileName >> offset >> dataSize >> baseBlockSize;
               if(soketType == SocketType::UDP){
                  blockNumber = 0;
                  ackPartLast = 0;
                  acks.clear();
                  acks.resize(dataSize / baseBlockSize + 1);
                  acks.fill(0);
                  slotSendData();
             }
               else
                  sendMsg(soketType, MsgType::Data, {offset, dataSize});
            }
            break;
         case MsgType::DataAck :
            {
                qint64 blockNumber;
                in >> blockNumber;
                acks[blockNumber] = 0;
                ackCounter--;
                attemptCounter = 0;
//                qDebug () << "DataAck: blockNumber = " << blockNumber << "ackCounter" << ackCounter;
            }
            break;
         case MsgType::DownloadAck :
            sendLostTimer->stop();
            fileName.clear();
            buffer.clear();
            break;
        case MsgType::Alive :
            sendMsg(soketType, MsgType::AckAlive);
            break;
        case MsgType::AckAlive :
            aliveCounter = 0;
            break;
        // default:
    }
}
void MyServer::slotSendData()
{
//    qDebug () << "slotSendData()";
    sendingTimer->stop();
    if(attemptCounter++ == 100)
        return;
    if(ackCounter > 0) {
        boost = false;
        windowSize = windowSize >> 1;
        // windowSize = windowSize - ackCounter;
    } else if (boost)
        windowSize = windowSize << 1;
    else
        windowSize += 1;
//    qDebug () << "windowSize = " << windowSize;
    QBuffer buffStream(&buffer);
    buffStream.open(QIODevice::ReadOnly);
    qint64 blockSize = baseBlockSize;
    int i = windowSize;

    while(offset < dataSize && i-- > 0)
    {
        ackCounter = windowSize;
        blockSize = (dataSize - offset > blockSize) ? blockSize : (dataSize - offset);
        QByteArray  arrBlock;
        QDataStream out(&arrBlock, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_9);
        out << quint16(0);
        out << qint8(MsgType::Data) << offset << blockSize << blockNumber;

        buffStream.seek(offset);
        out << buffStream.read(blockSize);
        out.device()->seek(0);
        out << quint16(arrBlock.size() - sizeof(quint16));
        pUdpSocket->writeDatagram(arrBlock, udpSenderAddress, udpSenderPort);
//                    qDebug() << "offset = " << offset << ", blockSize = " << blockSize;
        pUdpSocket->waitForBytesWritten();
        acks[blockNumber] = 1;
        blockNumber++;
        offset += baseBlockSize;
    }
    if(offset < dataSize) {
        sendingTimer->start(1);
    }
    else {
        sendingTimer->stop();
        slotSendLost();
    }
}
void MyServer::slotSendLost()
{
//    qDebug () << "slotSendLost()";
    sendLostTimer->stop();
    if(attemptCounter++ == 100)
        return;
    while(ackPartLast < blockNumber) {
        if(acks[ackPartLast] > 0)
            break;
        ackPartLast++;
    }
    if(ackPartLast < blockNumber){
        qint64 offset = ackPartLast * baseBlockSize;
        sendMsg(SocketType::UDP, MsgType::Data, {offset, baseBlockSize, ackPartLast});
//        qDebug () << "send lost parts: offset = " << offset << "baseBlockSize = " << baseBlockSize << "ackPartLast" << ackPartLast;
        sendLostTimer->start(1);
    } else {
        sendMsg(SocketType::UDP, MsgType::DownloadFin);
        sendLostTimer->start(1);
    }
}
void MyServer::slotConnectionStateChanged(QAbstractSocket::SocketState state)
{
    switch(state){
    case  QAbstractSocket::UnconnectedState :
        pTxt->append("The socket is not connected.");
//        m_ptcpServer->close();
//        m_ptcpServer->listen(QHostAddress::Any, nPort);
//        pClientSocket->reset();
        break;
    case QAbstractSocket::HostLookupState :
        pTxt->append("The socket is performing a host name lookup.");
        break;
    case QAbstractSocket::ConnectingState :
        pTxt->append("The socket has started establishing a connection.");
        break;
    case QAbstractSocket::ConnectedState :
        pTxt->append("A connection is established.");
        break;
    case QAbstractSocket::BoundState :
        pTxt->append("The socket is bound to an address and port.");
        break;
    case QAbstractSocket::ClosingState :
        pTxt->append("The socket is about to close (data may still be waiting to be written).");
        break;
    case QAbstractSocket::ListeningState :
        pTxt->append("For internal use only.");
    }
}
void MyServer::slotAboutToClose()
{
    pTxt->append("Device about to close.");
}
void MyServer::slotAlive()
{
    aliveCounter++;
    if(aliveCounter > 3)
    {
        aliveCounter = 0;
        aliveTimer->stop();
        slotResume();
        return;
    }
    sendMsg(SocketType::TCP, MsgType::Alive);
}

