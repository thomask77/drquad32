#include "MainWindow.h"
#include <QApplication>


MainWindow *mainWindow;


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setApplicationName("QuadControl");
    app.setOrganizationName("t-kindler");
    app.setOrganizationDomain("t-kindler.de");

    mainWindow = new MainWindow();
    mainWindow->show();

    return app.exec();
}
