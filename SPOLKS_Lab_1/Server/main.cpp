#include <QtWidgets>
#include "MyServer.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    MyServer     server;

    server.show();

    return app.exec();
}
