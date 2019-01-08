#include "Widget.h"
#include "ui_Widget.h"

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    ui->cbInput->addItems({
                              "8.8.8.8;5.255.255.5;192.168.0.1;178.172.160.2",
                              "5.255.255.5",
                              "192.168.0.1"
                          });
    ui->cbInput->setEditable(true);
}

Widget::~Widget()
{
    delete ui;
}

void Widget::on_pbPing_clicked()
{
    QString strInput = ui->cbInput->currentText();
    QStringList listAddresses = strInput.split(';');
    qDebug () << listAddresses;
    resultString.clear();
//    QFuture<QString> future = QtConcurrent::mapped(listAddresses.begin(), listAddresses.end(), multithreadPing);
//    future.waitForFinished();
//    // qDebug () << future.results();
//    foreach (auto item, future.results()) {
//        ui->teOutput->append(item);
//    }


//    auto future = QtConcurrent::map(listAddresses.begin(), listAddresses.end(), multithreadPing);
//    future.waitForFinished();
//     ui->teOutput->append(resultString);


//     qDebug () << resultString;

     auto future = QtConcurrent::map(listAddresses.begin(), listAddresses.end(), [this](QString & ip_address) {
             // Объявляем переменные
             HANDLE hIcmpFile;                       // Обработчик
             unsigned long ipaddr = INADDR_NONE;     // Адрес назначения
             DWORD dwRetVal = 0;                     // Количество ответов
             char SendData[32] = "Data Buffer";      // Буффер отсылаемых данных
             LPVOID ReplyBuffer = NULL;              // Буффер ответов
             DWORD ReplySize = 0;                    // Размер буффера ответов

             // Устанавливаем IP-адрес из поля lineEdit
             ipaddr = inet_addr(ip_address.toStdString().c_str());
             hIcmpFile = IcmpCreateFile();   // Создаём обработчик

             for(int i = 0; i < 3; i++){
                 // Выделяем память под буффер ответов
                 ReplySize = sizeof(ICMP_ECHO_REPLY) + sizeof(SendData);
                 ReplyBuffer = (VOID*) malloc(ReplySize);

                 // Вызываем функцию ICMP эхо запроса
                 dwRetVal = IcmpSendEcho(hIcmpFile, ipaddr, SendData, sizeof(SendData),
                             NULL, ReplyBuffer, ReplySize, 1000);

                 // создаём строку, в которою запишем сообщения ответа
                 QString strMessage = "";

                 if (dwRetVal != 0) {
                     // Структура эхо ответа
                     PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY)ReplyBuffer;
                     struct in_addr ReplyAddr;
                     ReplyAddr.S_un.S_addr = pEchoReply->Address;
                     strMessage += "Received from ";
                     strMessage += inet_ntoa( ReplyAddr );
             //        strMessage += "\n";
                     strMessage += " Status = " + pEchoReply->Status;
                     strMessage += " Roundtrip time = " + QString::number(pEchoReply->RoundTripTime) + " milliseconds";
                 } else {
                     DWORD error = GetLastError();
                     strMessage += "ICMP error " + QString::number(error) + ": ";
                     strMessage += getErrorDescription(error);
                 }

                 // resultString.append(strMessage);
                 mux.lock();
                 ui->teOutput->append(strMessage);
                 mux.unlock();

                 free(ReplyBuffer); // Освобождаем память

             }
        });
     future.waitForFinished();
      // ui->teOutput->append(resultString);
}

void Widget::on_pbTraceroute_clicked()
{

}

void multithreadPing(QString ip_address)
{
    // Объявляем переменные
    HANDLE hIcmpFile;                       // Обработчик
    unsigned long ipaddr = INADDR_NONE;     // Адрес назначения
    DWORD dwRetVal = 0;                     // Количество ответов
    char SendData[32] = "Data Buffer";      // Буффер отсылаемых данных
    LPVOID ReplyBuffer = NULL;              // Буффер ответов
    DWORD ReplySize = 0;                    // Размер буффера ответов

    // Устанавливаем IP-адрес из поля lineEdit
    ipaddr = inet_addr(ip_address.toStdString().c_str());
    hIcmpFile = IcmpCreateFile();   // Создаём обработчик

    for(int i = 0; i < 1; i++){
        // Выделяем память под буффер ответов
        ReplySize = sizeof(ICMP_ECHO_REPLY) + sizeof(SendData);
        ReplyBuffer = (VOID*) malloc(ReplySize);

        // Вызываем функцию ICMP эхо запроса
        dwRetVal = IcmpSendEcho(hIcmpFile, ipaddr, SendData, sizeof(SendData),
                    NULL, ReplyBuffer, ReplySize, 1000);

        // создаём строку, в которою запишем сообщения ответа
        QString strMessage = "";

        if (dwRetVal != 0) {
            // Структура эхо ответа
            PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY)ReplyBuffer;
            struct in_addr ReplyAddr;
            ReplyAddr.S_un.S_addr = pEchoReply->Address;
            strMessage += "Received from ";
            strMessage += inet_ntoa( ReplyAddr );
    //        strMessage += "\n";
            strMessage += " Status = " + pEchoReply->Status;
            strMessage += " Roundtrip time = " + QString::number(pEchoReply->RoundTripTime) + " milliseconds \n";
        } else {
            DWORD error = GetLastError();
            strMessage += "ICMP error " + QString::number(error) + ": ";
            strMessage += getErrorDescription(error) + "\n";
        }

        resultString.append(strMessage);

        free(ReplyBuffer); // Освобождаем память

    }
}

QString getErrorDescription(DWORD error ){
    QString s;
    switch(error) {
    case IP_BUF_TOO_SMALL            :
        s = "Buffer too small";
        break;
    case IP_DEST_NET_UNREACHABLE     :
        s = "Destitation net unreachable";
        break;
    case IP_DEST_HOST_UNREACHABLE    :
        s = "Destication host unreachable";
        break;
    case IP_DEST_PROT_UNREACHABLE    :
        s = "Destination port unreachable";
        break;
    case IP_NO_RESOURCES             :
        s = "No serources";
        break;
    case IP_BAD_OPTION               :
        s = "Bad option";
        break;
    case IP_HW_ERROR                 :
        s = "HW error";
        break;
    case IP_PACKET_TOO_BIG           :
        s = "Packet too big";
        break;
    case IP_REQ_TIMED_OUT            :
        s = "Request timed out";
        break;
    case IP_BAD_REQ                  :
        s = "Bad request";
        break;
    case IP_BAD_ROUTE                :
        s = "Bad route";
        break;
    case IP_TTL_EXPIRED_TRANSIT      :
        break;
    case IP_TTL_EXPIRED_REASSEM      :
        break;
    case IP_PARAM_PROBLEM            :
        break;
    case IP_SOURCE_QUENCH            :
        break;
    case IP_OPTION_TOO_BIG           :
        break;
    case IP_BAD_DESTINATION          :
        break;
    }
    return s;
}
