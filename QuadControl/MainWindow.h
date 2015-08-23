#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QTimer>
#include <QTime>
#include "Connection.h"


namespace Ui {
class MainWindow;
}


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    Connection connection;

private:
    Ui::MainWindow *ui;

    class ConnectionWindow  *connectionWindow;
    class UpdateWindow      *updateWindow;
    class ConsoleWindow     *consoleWindow;
    class GLWindow          *glWindow;
    class PlotWindow        *plotWindow;
    class InputMappingWindow *inputMappingWindow;

    bool isInitialized = false;

    QLabel statisticsLabel;
    QLabel connectionLabel;
    QTimer timer;

    QTime old_stats_t;
    Connection::Statistics old_stats;

    QDockWidget *addDockWindow(Qt::DockWidgetArea area, QWidget *window);

    void timer_timeout();
    void actionFullscreen_triggered();

    void actionAbout_triggered();
    void actionAboutQt_triggered();

    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;
};


extern MainWindow *mainWindow;   // singleton


#endif // MAINWINDOW_H
