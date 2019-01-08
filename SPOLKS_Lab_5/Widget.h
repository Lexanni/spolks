#ifndef WIDGET_H
#define WIDGET_H

#include "winsock2.h"
#include "iphlpapi.h"
#include "icmpapi.h"
#include <QWidget>
#include <QtConcurrent/QtConcurrent>
#include <QDebug>
#include <QThread>

void multithreadPing(QString ip_address);
QString getErrorDescription(DWORD);
static QString resultString;
static int threadCounter = 0;

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget();

private:
    Ui::Widget *ui;

public slots:
    void on_pbPing_clicked();
    void on_pbTraceroute_clicked();
    void on_pbSmurf_clicked();
//    void multithreadPing(QString);
};

#endif // WIDGET_H
