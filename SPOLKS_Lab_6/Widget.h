#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QStringListModel>
#include <QDebug>
#include <QNetworkInterface>
#include <QList>

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

    enum MsgType {
        Hello,
        HelloAck,
        Leave,
        Msg
    };

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();

private:
    Ui::Widget *ui;
    QUdpSocket *pUdpSocket;
    bool isJoined = false;
    bool isBind   = false;
    QStringListModel model;
    QStringList      slMembers;
    int port = 45454;
    QHostAddress groupAddress;
    QHostAddress myAddress;

public slots:
    void slotBindResume();
    void slotJoinLeaveGroup();
    void slotReadUdpSocket();
    void processRecivedData(QNetworkDatagram &datagram);
    void sendMsg(MsgType type, QList<QVariant> args = QList<QVariant>());
    void slotParseInput();
};

#endif // WIDGET_H
