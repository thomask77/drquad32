#ifndef PLOTWINDOW_H
#define PLOTWINDOW_H

#include <QMainWindow>
#include <QTimer>

namespace Ui {
class PlotWindow;
}

class PlotWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit PlotWindow(class MainWindow *parent = 0);
    ~PlotWindow();

    QSize sizeHint() const override { return QSize(480, 320); }

private:
    Ui::PlotWindow *ui;
    class MainWindow *mainWindow;

    QTimer timer;

    void timer_timeout();
};

#endif // PLOTWINDOW_H
