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
}

Widget::~Widget()
{
    delete ui;
}

void Widget::on_pbPing_clicked()
{
    QString strInput = ui->cbInput->currentText();
    QStringList listAddresses = strInput.split(';');
//    qDebug () << listAddresses;
    resultString.clear();
    auto future = QtConcurrent::map(listAddresses.begin(), listAddresses.end(), multithreadPing);
    future.waitForFinished();
    ui->teOutput->append(resultString);
    threadCounter = 0;
}

void Widget::on_pbTraceroute_clicked()
{
    QString ip_address = ui->cbInput->currentText();
    QStringList list_ip_addr = ip_address.split(';');
    ip_address = list_ip_addr.first();
    QString senderIp;
    HANDLE hIcmpFile;                       // Обработчик
    unsigned long ipaddr = INADDR_NONE;     // Адрес назначения
    DWORD dwRetVal = 0;                     // Количество ответов
    char SendData[32] = "Data Buffer";      // Буффер отсылаемых данных
    LPVOID ReplyBuffer = NULL;              // Буффер ответов
    DWORD ReplySize = 0;                    // Размер буффера ответов
    IP_OPTION_INFORMATION ip_opt_info;

    ZeroMemory(&ip_opt_info, sizeof (IP_OPTION_INFORMATION));
    ui->teOutput->append("Traceroute to " + ip_address + " (30 hops max):");
    ipaddr = inet_addr(ip_address.toStdString().c_str());
    hIcmpFile = IcmpCreateFile();

    for(uchar i = 1; i <= 30; i++){
        ip_opt_info.Ttl = i;

        ReplySize = sizeof(ICMP_ECHO_REPLY) + sizeof(SendData);
        ReplyBuffer = (VOID*) malloc(ReplySize);

        dwRetVal = IcmpSendEcho(hIcmpFile, ipaddr, SendData, sizeof(SendData),
                    &ip_opt_info, ReplyBuffer, ReplySize, 2000);

        QString strMessage = QString::number(i) + ". \t";

        if (dwRetVal != 0) {
            PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY)ReplyBuffer;
            struct in_addr ReplyAddr;
            ReplyAddr.S_un.S_addr = pEchoReply->Address;
            strMessage += QString::number(pEchoReply->RoundTripTime) + " ms \t";
            senderIp = inet_ntoa( ReplyAddr );
            strMessage += senderIp;
        } else {
            DWORD error = GetLastError();
            strMessage += getErrorDescription(error);
        }
        ui->teOutput->append(strMessage);
        free(ReplyBuffer); // Освобождаем память
        if(senderIp == ip_address)
            break;
    }
}

void Widget::on_pbSmurf_clicked()
{
    QString dst_address = ui->cbInput->currentText();
    QString victim_address = ui->cbVictimAddress->currentText();

    HANDLE hIcmpFile;
    unsigned long dst_ip_addr = INADDR_NONE;
    unsigned long victim_ip_addr = INADDR_NONE;
    DWORD dwRetVal = 0;
    char SendData[32] = "Data Buffer";
    LPVOID ReplyBuffer = NULL;
    DWORD ReplySize = 0;

    dst_ip_addr = inet_addr(dst_address.toStdString().c_str());
    victim_ip_addr = inet_addr(victim_address.toStdString().c_str());
    hIcmpFile = IcmpCreateFile();

    ReplySize = sizeof(ICMP_ECHO_REPLY) + sizeof(SendData);
    ReplyBuffer = (VOID*) malloc(ReplySize);
    dwRetVal = IcmpSendEcho2Ex(hIcmpFile, NULL, NULL, NULL, victim_ip_addr, dst_ip_addr,
                               SendData, sizeof(SendData), NULL, ReplyBuffer, ReplySize, 1000);
    free(ReplyBuffer);
}

void multithreadPing(QString ip_address)
{
    QString threadName = "T-" + QString::number(++threadCounter) + ": ";
    HANDLE hIcmpFile;                       // Обработчик
    unsigned long ipaddr = INADDR_NONE;     // Адрес назначения
    DWORD dwRetVal = 0;                     // Количество ответов
    char SendData[32] = "Data Buffer";      // Буффер отсылаемых данных
    LPVOID ReplyBuffer = NULL;              // Буффер ответов
    DWORD ReplySize = 0;                    // Размер буффера ответов

    ipaddr = inet_addr(ip_address.toStdString().c_str());
    hIcmpFile = IcmpCreateFile();

    for(int i = 0; i < 3; i++)
    {
        ReplySize = sizeof(ICMP_ECHO_REPLY) + sizeof(SendData);
        ReplyBuffer = (VOID*) malloc(ReplySize);
        dwRetVal = IcmpSendEcho(hIcmpFile, ipaddr, SendData, sizeof(SendData),
                        NULL, ReplyBuffer, ReplySize, 1000);

        QString strMessage = threadName;

        if (dwRetVal != 0) {
            PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY)ReplyBuffer;
            struct in_addr ReplyAddr;
            ReplyAddr.S_un.S_addr = pEchoReply->Address;
            strMessage += "Reply from ";
            strMessage += inet_ntoa( ReplyAddr );
//            strMessage += " Status = " + QString::number(pEchoReply->Status);
            strMessage += "\tRTT = " + QString::number(pEchoReply->RoundTripTime) + " ms \n";
        } else {
            DWORD error = GetLastError();
            strMessage += "\tICMP error " + QString::number(error) + ": ";
            strMessage += getErrorDescription(error) + "\n";
        }
        resultString.append(strMessage);
        free(ReplyBuffer);
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
        s = "TTL expired transit";
        break;
    case IP_TTL_EXPIRED_REASSEM      :
        s = "TTL expired reassem";
        break;
    case IP_PARAM_PROBLEM            :
        s = "Param problem";
        break;
    case IP_SOURCE_QUENCH            :
        s = "Source quench";
        break;
    case IP_OPTION_TOO_BIG           :
        s = "Option too big";
        break;
    case IP_BAD_DESTINATION          :
        s = "Bad destination";
        break;
    }
    return s;
}
