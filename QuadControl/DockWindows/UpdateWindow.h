#ifndef UPDATEWINDOW_H
#define UPDATEWINDOW_H

#include <QMainWindow>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QProgressDialog>
#include <QFileSystemWatcher>

#include "../BootProtocol.h"

namespace Ui {
class UpdateWindow;
}

class UpdateWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit UpdateWindow(QWidget *parent = 0);
    ~UpdateWindow();

private:
    Ui::UpdateWindow *ui;
    QFileSystemWatcher watcher;

    void watchFile(const QString &fn);

    void lineEdit_textChanged();
    void browseButton_clicked();
    void updateButton_clicked();
    void connection_changed();
    void watcher_fileChanged();

    virtual void dragEnterEvent(QDragEnterEvent *event) override;
    virtual void dropEvent(QDropEvent *event) override;
};

#endif // UPDATEWINDOW_H
