#include <QtNetwork>
#include <QtWidgets>
#include <QFile>
#include "MyServer.h"

// ----------------------------------------------------------------------
MyServer::MyServer(QWidget* pwgt /*=0*/) : QWidget(pwgt)
                                                    , m_nNextBlockSize(0)
{
    m_ptcpServer = new QTcpServer(this); 
    connect(m_ptcpServer, SIGNAL(newConnection()), 
            this,         SLOT(slotNewConnection())
           );

    m_ptxt = new QTextEdit;
    m_ptxt->setReadOnly(true);
    m_ptxtPort = new QLineEdit("2323");
    bListen = new QPushButton("Listen");
    bResume = new QPushButton("Resume");
    bResume->setEnabled(false);

    connect(bListen, SIGNAL(clicked()), this, SLOT(slotListen()));
    connect(bResume, SIGNAL(clicked()), this, SLOT(slotResume()));

    QHBoxLayout * portLayout = new QHBoxLayout();
    portLayout->addWidget(new QLabel("Port: "));
    portLayout->addWidget(m_ptxtPort);
    portLayout->addWidget(bListen);
    portLayout->addWidget(bResume);

    //Layout setup
    QVBoxLayout* pvbxLayout = new QVBoxLayout;    
    pvbxLayout->addWidget(new QLabel("<H1>Server</H1>"));
    pvbxLayout->addLayout(portLayout);
    pvbxLayout->addWidget(m_ptxt);
    setLayout(pvbxLayout);
}

// ----------------------------------------------------------------------
/*virtual*/ void MyServer::slotNewConnection()
{
    if(countClients == 0){
        pClientSocket = m_ptcpServer->nextPendingConnection();
        connect(pClientSocket, SIGNAL(disconnected()),
                this, SLOT(hDisconnected())
               );
        connect(pClientSocket, SIGNAL(readyRead()),
                this,          SLOT(slotReadClient())
               );
        connect(pClientSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
                this,          SLOT(slotConnectionStateChanged(QAbstractSocket::SocketState)));
        connect(pClientSocket, SIGNAL(aboutToClose()), this, SLOT(slotAboutToClose()));
        countClients++;
        //sendToClient(pClientSocket, "Server Response: Connected!");
    }
    else {
        QTcpSocket * pDiscardSocket = m_ptcpServer->nextPendingConnection();
        //sendToClient(pDiscardSocket, "Server Response: Connection rejected.");

        pDiscardSocket->disconnectFromHost();
        if (pDiscardSocket->state() == QAbstractSocket::ConnectedState) {
            pDiscardSocket->waitForDisconnected();
        }
        delete pDiscardSocket;
    }
}

// ----------------------------------------------------------------------
void MyServer::slotReadClient()
{
    QTcpSocket* pClientSocket = (QTcpSocket*)sender();
    QDataStream in(pClientSocket);
    in.setVersion(QDataStream::Qt_5_9);

    if (!m_nNextBlockSize) {
        if (pClientSocket->bytesAvailable() < (int)sizeof(quint16)) {
            return;
        }
        in >> m_nNextBlockSize;
    }
    if (pClientSocket->bytesAvailable() < m_nNextBlockSize) {
        return;
    }

    QString s;
    qint8 reqType;
    in >> reqType;
    switch(reqType)
    {
        case MsgType::Sync :
            qDebug () << "recive request Sync" << endl;
            m_ptxt->append("Recive request Sync");
            int clientId;
            in >> clientId;
            if(clientId != curClientId)
                curClientId++;
            sendToClient(MsgType::Sync);
            break;
         case MsgType::Echo :
            qDebug () << "recive request Echo" << endl;
            in >> s;
            m_ptxt->append("ECHO " + s);
            sendToClient(MsgType::Echo, {s});
            break;
        case MsgType::Time :
            qDebug () << "recive request Time" << endl;
            in >> s;
            m_ptxt->append("TIME");
            sendToClient(MsgType::Time);
            break;
        case MsgType::Close :
            hDisconnected();
            break;
        case MsgType::Download :
            qDebug () << "recive request Download" << endl;
            in >> s;
            m_ptxt->append("DOWNLOAD " + s);
            if(QFile::exists(s)){
                qDebug () << "file " + s + " exists" << endl;
                fileName = s;
                sendToClient(MsgType::DataAnonce, {s});
                QFile file(s);
                file.open(QIODevice::ReadOnly);
                buffer = file.readAll();
                file.close();
            }
            else
            {
                qDebug () << "file " + s + " don't exits!" << endl;
                sendToClient(MsgType::Msg, {"File don't exist."});
            }
            break;
         case MsgType::DataRequest :
            {
//               qDebug () << "Reciver DataRequest" << endl;
               in >> s;
               qint64 offset;
               qint64 blockSize;
               in >> offset >> blockSize;
               sendToClient(MsgType::Data, {offset, blockSize});
            }
            break;
         case MsgType::DownloadAck :
            fileName.clear();
            buffer.clear();
            break;
        // default:
    }
    m_nNextBlockSize = 0;
}

// ----------------------------------------------------------------------
void MyServer::sendToClient(MsgType type, QList<QVariant> args)
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
        case MsgType::DataAnonce :
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
                out << qint8(MsgType::Data) << offset << size;
                buffStream.seek(args.first().toLongLong());
                out << buffStream.read(size);
            }
            break;
        // default:
    }

    out.device()->seek(0);
    out << quint16(arrBlock.size() - sizeof(quint16));

    pClientSocket->write(arrBlock);
    pClientSocket->flush();
}
// ----------------------------------------------------------------------
void MyServer::hDisconnected()
{
    if(sender() == pClientSocket){
        // qDebug () << "hDisconnected()" << endl;
        countClients--;
        pClientSocket->disconnectFromHost();
    }
}

void MyServer::slotListen()
{
    nPort = m_ptxtPort->text().toInt();
    if (!m_ptcpServer->listen(QHostAddress::Any, nPort)) {
        QMessageBox::critical(0,
                              "Server Error",
                              "Unable to start the server:"
                              + m_ptcpServer->errorString()
                             );
        m_ptcpServer->close();
        return;
    }
    m_ptxtPort->setEnabled(false);
    bListen->setEnabled(false);
    bResume->setEnabled(true);
    m_ptxt->append("Listening port " + m_ptxtPort->text());
}

void MyServer::slotResume()
{
//    if (pClientSocket != nullptr) {
//        delete pClientSocket;
//        pClientSocket = nullptr;
//        countClients--;
//    }
    m_ptcpServer->close();
    bListen->setEnabled(true);
    bResume->setEnabled(false);
    m_ptxtPort->setEnabled(true);
    m_ptxt->append("Resume port " + m_ptxtPort->text());
}

void MyServer::slotConnectionStateChanged(QAbstractSocket::SocketState state)
{
    switch(state){
    case  QAbstractSocket::UnconnectedState :
        m_ptxt->append("The socket is not connected.");
        m_ptcpServer->close();
        m_ptcpServer->listen(QHostAddress::Any, nPort);
        break;
    case QAbstractSocket::HostLookupState :
        m_ptxt->append("The socket is performing a host name lookup.");
        break;
    case QAbstractSocket::ConnectingState :
        m_ptxt->append("The socket has started establishing a connection.");
        break;
    case QAbstractSocket::ConnectedState :
        m_ptxt->append("A connection is established.");
        break;
    case QAbstractSocket::BoundState :
        m_ptxt->append("The socket is bound to an address and port.");
        break;
    case QAbstractSocket::ClosingState :
        m_ptxt->append("The socket is about to close (data may still be waiting to be written).");
        break;
    case QAbstractSocket::ListeningState :
        m_ptxt->append("For internal use only.");
    }
}

void MyServer::slotAboutToClose()
{
    m_ptxt->append("Device about to close.");
}
