#ifndef PLOTWINDOW_H
#define PLOTWINDOW_H

#include <QMainWindow>
#include <QTimer>

#include "../../Source/msg_structs.h"

namespace Ui {
class PlotWindow;
}

class PlotWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit PlotWindow(QWidget *parent = 0);
    ~PlotWindow();

    QSize sizeHint() const override { return QSize(480, 320); }

private:
    Ui::PlotWindow *ui;

    QTimer timer;

    void timer_timeout();
    void connection_messageReceived(const msg_generic &msg);
};

#endif // PLOTWINDOW_H
