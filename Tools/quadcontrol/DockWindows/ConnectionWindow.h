#ifndef CONNECTIONWINDOW_H
#define CONNECTIONWINDOW_H

#include "WiFlyListener.h"

#include <QMainWindow>
#include <QTreeWidgetItem>

namespace Ui {
class ConnectionWindow;
}

class ConnectionWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ConnectionWindow(class MainWindow *parent = 0);
    ~ConnectionWindow();

private:
    Ui::ConnectionWindow *ui;  
    class MainWindow *mainWindow;

    WiFlyListener wiFlyListener;

    void actionScan_triggered();
    void actionConnect_triggered();
    void actionDisconnect_triggered();
    void treewidget_currentItemChanged();
    void connectionChanged();

    QTreeWidgetItem *getSelectedItem();
};

#endif // CONNECTIONWINDOW_H
