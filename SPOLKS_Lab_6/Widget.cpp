#include "Widget.h"
#include "ui_Widget.h"

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    QStringList slInterfaces;
    foreach (auto item, QNetworkInterface::allAddresses()) {
        slInterfaces << item.toString();
    }
    ui->cbInterfaces->addItems(slInterfaces);
    ui->cbMulticastGroup->addItems({"239.255.1.1"});
    model.setStringList(slMembers);
    ui->lvMembers->setModel(&model);
    ui->leInput->setEnabled(false);

    connect(ui->pbBindResume, SIGNAL(clicked()), this, SLOT(slotBindResume()));
    connect(ui->pbJoinLeave, SIGNAL(clicked()), this, SLOT(slotJoinLeaveGroup()));
    connect(ui->leInput, SIGNAL(returnPressed()), this, SLOT(slotParseInput()));
    connect(ui->lvMembers, SIGNAL(clicked(QModelIndex)), this, SLOT(slotToogleIgnore(QModelIndex)));
    connect(ui->pbMulticastBroadcast, SIGNAL(clicked()), this, SLOT(slotMulticastBroadcast()));
}

Widget::~Widget()
{
    delete ui;
}

void Widget::slotBindResume()
{
    if(!isBind) {
        myAddress = QHostAddress(ui->cbInterfaces->currentText());
        pUdpSocket = new QUdpSocket;
        pUdpSocket->bind(myAddress, port, QUdpSocket::ShareAddress);
        connect(pUdpSocket, SIGNAL(readyRead()), this, SLOT(slotReadUdpSocket()));
        isBind = true;
        ui->pbBindResume->setText("Resume");
        ui->cbInterfaces->setEnabled(false);
    } else {
        if(isJoined)
            slotJoinLeaveGroup();
        disconnect(pUdpSocket, SIGNAL(readyRead()), this, SLOT(slotReadUdpSocket()));
        pUdpSocket->deleteLater();
        ui->pbBindResume->setText("Bind");
        ui->cbInterfaces->setEnabled(true);
        isBind = false;
    }
}

void Widget::slotJoinLeaveGroup()
{
    if(!isJoined) {
        groupAddress = QHostAddress(ui->cbMulticastGroup->currentText());
//        groupInterface = QNetworkInterface::interfaceFromIndex(ui->cbInterfaces->currentIndex());
//        pUdpSocket->joinMulticastGroup(groupAddress, groupInterface);
        pUdpSocket->joinMulticastGroup(groupAddress);
        sendMsg(MsgType::Hello);
        ui->pbJoinLeave->setText("Leave");
        isJoined = true;
        ui->cbMulticastGroup->setEnabled(false);
        ui->leInput->setEnabled(true);
    } else {
        sendMsg(MsgType::Leave);
//        pUdpSocket->leaveMulticastGroup(groupAddress, groupInterface);
        pUdpSocket->leaveMulticastGroup(groupAddress);
        ui->pbJoinLeave->setText("Join");
        isJoined = false;
        ui->cbMulticastGroup->setEnabled(true);
        ui->leInput->setEnabled(true);
        slMembers.clear();
        model.setStringList(slMembers);
        ignoreList.clear();
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
        {
            QByteArray ba;
            QDataStream out(&ba, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_5_9);
            out << qint8(MsgType::HelloAck);
            // datagram.makeReply(ba);
            pUdpSocket->writeDatagram(ba, udpSenderAddress, udpSenderPort);
        }
        case MsgType::HelloAck :
            if(!slMembers.contains(senderSocket)) {
                slMembers << senderSocket;
                model.setStringList(slMembers);
            }
            break;
        case MsgType::Msg :
            in >> s;
            if(!ignoreList.contains(senderSocket))
                ui->teChat->append(senderSocket + " > " + s);
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
    if(isBroadcast){
        pUdpSocket->writeDatagram(arrBlock, QHostAddress::Broadcast, port);
    } else {
        pUdpSocket->writeDatagram(arrBlock, groupAddress, port);
    }
}

void Widget::slotParseInput()
{
    sendMsg(MsgType::Msg, {ui->leInput->text()});
    ui->leInput->clear();
}

void Widget::slotToogleIgnore(QModelIndex idx)
{
    QString postfix = " (ignore)";
    QString s = idx.data().toString();
    int i = idx.row();

    if(s.contains(postfix)) {
        s = s.left(s.size() - postfix.size());
        ignoreList.removeAll(s);
        slMembers.replace(i, s);
    } else {
        ignoreList.append(s);
        slMembers.replace(i, s + postfix);
    }
    model.setStringList(slMembers);
}

void Widget::slotMulticastBroadcast()
{
    if(!isBroadcast) {
        if(isJoined)
            slotJoinLeaveGroup();
        isBroadcast = true;
        ui->pbMulticastBroadcast->setText("Broadcast");
        ui->leInput->setEnabled(true);
        ui->cbMulticastGroup->setEnabled(false);
        ui->pbJoinLeave->setEnabled(false);
        sendMsg(MsgType::Hello);
    } else {
        isBroadcast = false;
        ui->pbMulticastBroadcast->setText("Multicast");
        ui->leInput->setEnabled(false);
        ui->cbMulticastGroup->setEnabled(true);
        ui->pbJoinLeave->setEnabled(true);
        sendMsg(MsgType::Leave);
        slMembers.clear();
        model.setStringList(slMembers);
        ignoreList.clear();
    }
}
