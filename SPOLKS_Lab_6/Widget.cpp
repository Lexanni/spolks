#include "Widget.h"
#include "ui_Widget.h"

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    ui->cbMulticastGroup->addItems({"239.255.1.1"});
    model.setStringList(slMembers);
    ui->lvMembers->setModel(&model);

    pUdpSocket = new QUdpSocket;
    pUdpSocket->bind(QHostAddress::AnyIPv4, port, QUdpSocket::ShareAddress);

    connect(ui->pbJoinLeave, SIGNAL(clicked()), this, SLOT(slotJoinLeaveGroup()));
    connect(pUdpSocket, SIGNAL(readyRead()), this, SLOT(slotReadUdpSocket()));
}

Widget::~Widget()
{
    delete ui;
}

void Widget::slotJoinLeaveGroup()
{
    if(!isJoined) {
        groupAddress = QHostAddress(ui->cbMulticastGroup->currentText());
        pUdpSocket->joinMulticastGroup(groupAddress);
        ui->pbJoinLeave->setText("Leave");
        isJoined = true;
    } else {
        pUdpSocket->leaveMulticastGroup(groupAddress);
        ui->pbJoinLeave->setText("Join");
        isJoined = false;
    }
}

void Widget::slotReadUdpSocket()
{
    qDebug () << "slotReadUdpSocket()";
    disconnect(pUdpSocket, SIGNAL(readyRead()), this, SLOT(slotReadUdpSocket()));
    while (pUdpSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = pUdpSocket->receiveDatagram();
        processRecivedData(datagram);
    }
    connect(pUdpSocket, SIGNAL(readyRead()), this, SLOT(slotReadUdpSocket()));
}

void Widget::processRecivedData(QNetworkDatagram &datagram)
{
    QHostAddress udpSenderAddress = datagram.senderAddress();
    int          udpSenderPort    = datagram.senderPort();
    QString      senderSocket = udpSenderAddress.toString() + ":" + QString::number(udpSenderPort);
    qDebug () << "sender: " << senderSocket;
    QDataStream in(datagram.data());
    in.setVersion(QDataStream::Qt_5_9);
    QString s;
    qint8 respType;
    in >> respType;
    switch (respType)
    {
        case MsgType::Hello :
            if(!slMembers.contains(senderSocket)) {
                slMembers << senderSocket;
                model.setStringList(slMembers);
            }
            break;
        case MsgType::Msg :
            in >> s;
            ui->teChat->append(senderSocket + ":" + s);
            break;
        case MsgType::Leave :
            slMembers.removeAll(senderSocket);
            model.setStringList(slMembers);
            break;
        default:
            ui->teChat->append("Unknown MsgType");
    }
}
void Widget::sendMsg(MsgType type, QList<QVariant> args)
{
    QByteArray  arrBlock;
    QDataStream out(&arrBlock, QIODevice::WriteOnly);

    switch(type)
    {
        case MsgType::Hello :
            out << qint8(MsgType::Hello);
            break;
        case MsgType::Msg :
            out << qint8(MsgType::Msg) << args.first().toString();
            break;
        case MsgType::Leave :
            out << qint8(MsgType::Leave);
            break;
        default:
            qDebug () << "Unknown MsgType!";
    }
    pUdpSocket->writeDatagram(arrBlock, groupAddress, port);
}
