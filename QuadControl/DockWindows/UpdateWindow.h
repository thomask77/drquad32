#ifndef UPDATEWINDOW_H
#define UPDATEWINDOW_H

#include <QMainWindow>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QProgressDialog>

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
    QTimer timer;
    QDateTime lastModified;

    void browseButton_clicked();
    void updateButton_clicked();
    void rebootButton_clicked();
    void connection_changed();

    void lineEdit_textChanged();
    void timer_timeout();

    virtual void dragEnterEvent(QDragEnterEvent *event) override;
    virtual void dropEvent(QDropEvent *event) override;
};

#endif // UPDATEWINDOW_H
