#ifndef CONSOLEWINDOW_H
#define CONSOLEWINDOW_H

#include "msg_structs.h"

#include <QTimer>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QMainWindow>

namespace Ui {
class ConsoleWindow;
}

class ConsoleWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ConsoleWindow(class QWidget *parent = 0);
    ~ConsoleWindow();

    QSize sizeHint() const override { return QSize(640, 80); }

private:
    Ui::ConsoleWindow *ui;

    QTimer  timer;
    QString rxBuf;

    void actionClear_triggered();
    void actionSave_triggered();
    void actionWrap_triggered();

    void connection_messageReceived(const msg_generic &msg);
    void timer_timeout();
};

#endif // CONSOLEWINDOW_H
