#include <QtWidgets>
#include "MyServer.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    MyServer     server(2323);

    server.show();

    return app.exec();
}
