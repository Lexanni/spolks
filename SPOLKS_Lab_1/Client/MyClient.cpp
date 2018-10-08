#include <QtWidgets>
#include <QtGui>
#include <QFile>
#include <QDebug>
#include "MyClient.h"

// ----------------------------------------------------------------------
MyClient::MyClient(QWidget*       pwgt /*=0*/
                  ) : QWidget(pwgt)
                    , m_nNextBlockSize(0)
{
    m_pTcpSocket = new QTcpSocket(this);

    m_ptxtInfo  = new QTextEdit;
    m_ptxtInput = new QLineEdit;

    connect(m_ptxtInput, SIGNAL(returnPressed()), 
            this,        SLOT(parseInput())
           );
    m_ptxtInfo->setReadOnly(true);

    QPushButton* pcmd = new QPushButton("&Send");
    connect(pcmd, SIGNAL(clicked()), SLOT(parseInput()));

    labelSpeed = new QLabel("<H3>Speed</H3>");
    progressBar = new QProgressBar();
    progressBar->setMaximum(100);
    progressBar->setMinimum(0);
    progressBar->setValue(0);

    m_ptxtIp   = new QLineEdit("localhost");
    m_ptxtPort = new QLineEdit("2323");
    QPushButton* bConnect = new QPushButton("Connect");

    connect(bConnect, SIGNAL(clicked()), this, SLOT(slotConnectToHost()));

    QHBoxLayout* addressLayout = new QHBoxLayout();
    addressLayout->addWidget(new QLabel("IP: "));
    addressLayout->addWidget(m_ptxtIp);
    addressLayout->addWidget(new QLabel("Port: "));
    addressLayout->addWidget(m_ptxtPort);
    addressLayout->addWidget(bConnect);

    //Layout setup
    QVBoxLayout* pvbxLayout = new QVBoxLayout;
    pvbxLayout->addWidget(new QLabel("<H1>Client</H1>"));
    pvbxLayout->addLayout(addressLayout);
    pvbxLayout->addWidget(m_ptxtInfo);
    pvbxLayout->addWidget(labelSpeed);
    pvbxLayout->addWidget(progressBar);
    pvbxLayout->addWidget(m_ptxtInput);
    pvbxLayout->addWidget(pcmd);
    setLayout(pvbxLayout);
}
// ----------------------------------------------------------------------
void MyClient::slotReadyRead()
{

    QTcpSocket* pSocket = (QTcpSocket*)sender();
    disconnect(pSocket, SIGNAL(readyRead()), this, SLOT(slotReadyRead()));
    QDataStream in(pSocket);
    in.setVersion(QDataStream::Qt_5_9);
    qDebug () << "read buffer size = " << pSocket->readBufferSize() << endl;
    forever {
        if (!m_nNextBlockSize) {
            if (m_pTcpSocket->bytesAvailable() < (int)sizeof(quint16)) {
                break;
            }
            in >> m_nNextBlockSize;
        }
        if (m_pTcpSocket->bytesAvailable() < m_nNextBlockSize) {
            break;
        }

    QString s;
    qint8 respType;
    in >> respType;
    switch (respType)
    {
        case MsgType::Sync :
            int resiveId;
            in >> resiveId;
            qDebug () << "resived Id = " << resiveId << endl;
            if(id != resiveId)
                id = resiveId;
            break;
        case MsgType::Echo :
            in >> s;
            m_ptxtInfo->append("Server: " + s);
            m_ptxtInput->clear();
            break;
        case MsgType::Time :
            {
            QTime time;
            in >> time;
            m_ptxtInfo->append("Server: " + time.toString());
            m_ptxtInput->clear();
            break;
            }
        case MsgType::Msg :
            in >> s;
            m_ptxtInfo->append("Server: " + s);
            m_ptxtInput->clear();
            break;
        case MsgType::DataAnonce :
            {
                in >> fileName;
                in >> fileSize;
                qDebug() << "Recived DataAnonce. FileName: " << fileName << " size: " << fileSize << endl;
                m_ptxtInfo->append("Server: " + s + " " + QString::number(fileSize) + " bytes");
                m_ptxtInput->clear();
                slotSendToServer(MsgType::DataRequest, {fileName, 0, qMin(fileSize, blockSize)});
                time.start();
            }
            break;
        case MsgType::Data :
            {
                qint64 offset;
                qint64 size;
                in >> offset >> size;
                qDebug() << "Recived data. Size: " << size << endl;



                QBuffer bufferStream(&buffer);
                bufferStream.open(QIODevice::Append);
                // bufferStream.seek(offset);
                QByteArray b;
                in >> b;
                bufferStream.write(b);
                progressBar->setValue(buffer.size() * 100 / fileSize);
                if(buffer.size() == fileSize) {
                    int t = time.elapsed();
                    labelSpeed->setText("<H3>Speed: " + QString::number(fileSize/t) + " KB/s</H3>");
                    QFile file("recive");
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
        default:
            m_ptxtInfo->append("Unknown response");
    }

    m_nNextBlockSize = 0;

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
                     QString(m_pTcpSocket->errorString())
                    );
    m_ptxtInfo->append(strError);
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
    }

    out.device()->seek(0);
    out << quint16(arrBlock.size() - sizeof(quint16));

    m_pTcpSocket->write(arrBlock);
    m_ptxtInput->setText("");
}
// ----------------------------------------------------------------------
void MyClient::parseInput()
{
    QList<QVariant> args;
    QString s = m_ptxtInput->text();
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
}

// ------------------------------------------------------------------
void MyClient::slotConnected()
{
    m_ptxtInfo->append("Received the connected() signal");
    slotSendToServer(MsgType::Sync);
}

void MyClient::slotConnectToHost()
{
    QString strHost = m_ptxtIp->text();
    int nPort = m_ptxtPort->text().toInt();
    qDebug () << "strHost: " << strHost << ", port: " << nPort;
    m_pTcpSocket->connectToHost(strHost, nPort);
    connect(m_pTcpSocket, SIGNAL(connected()), SLOT(slotConnected()));
    connect(m_pTcpSocket, SIGNAL(readyRead()), SLOT(slotReadyRead()));
    connect(m_pTcpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
            this,         SLOT(slotError(QAbstractSocket::SocketError))
           );
    QPushButton * pButton = (QPushButton *)sender();
    pButton->setEnabled(false);
}
