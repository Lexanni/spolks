#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QStringListModel>
#include <QDebug>

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

    enum MsgType {
        Hello,
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
    QStringListModel model;
    QStringList      slMembers;
    int port = 45454;
    QHostAddress groupAddress;

public slots:
    void slotJoinLeaveGroup();
    void slotReadUdpSocket();
    void processRecivedData(QNetworkDatagram &datagram);
    void sendMsg(MsgType type, QList<QVariant> args);
};

#endif // WIDGET_H
