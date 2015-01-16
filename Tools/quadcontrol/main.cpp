#include "MainWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setApplicationName("QuadControl");
    a.setOrganizationName("t-kindler");
    a.setOrganizationDomain("t-kindler.de");

    MainWindow w;
    w.show();

    return a.exec();
}
