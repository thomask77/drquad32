#ifndef CONNECTIONWINDOW_H
#define CONNECTIONWINDOW_H

#include "WiFlyListener.h"
#include "InterleavedFuture.h"

#include <QMainWindow>
#include <QTreeWidgetItem>
#include <QSerialPortInfo>
#include <QTimer>
#include <QMap>


namespace Ui {
class ConnectionWindow;
}


class ConnectionWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ConnectionWindow(QWidget *parent = 0);
    ~ConnectionWindow();

private:
    Ui::ConnectionWindow *ui;  

    QTimer  timer;
    QFont   boldFont;

    WiFlyListener   wiFlyListener;

    // availablePorts takes fucking ages on windows,
    // so we execute it in a background thread.
    //
    InterleavedFuture<
        QList<QSerialPortInfo>,
        &QSerialPortInfo::availablePorts
    > availablePortsAsync;

    QTreeWidgetItem *serialItems;
    QTreeWidgetItem *wiFlyItems;
    QTreeWidgetItem *favoriteItems;

    void timer_timeout();

    void actionAdd_triggered();
    void actionRemove_triggered();

    void tryConnect(const  QUrl &url);
    void actionConnect_triggered();
    void actionDisconnect_triggered();
    void actionTerminal_triggered();

    void treewidget_currentItemChanged();
    void connectionChanged();

    QTreeWidgetItem *getSelectedItem();
};

#endif // CONNECTIONWINDOW_H
