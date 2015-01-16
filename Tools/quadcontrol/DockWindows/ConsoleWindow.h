#ifndef CONSOLEWINDOW_H
#define CONSOLEWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QTextCursor>
#include <QTextCharFormat>

#include "AnsiParser.h"

namespace Ui {
class ConsoleWindow;
}

class ConsoleWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ConsoleWindow(class MainWindow *parent = 0);
    ~ConsoleWindow();

private:
    Ui::ConsoleWindow *ui;
    class MainWindow *mainWindow;

    void actionClear_triggered();
    void actionSave_triggered();
    void actionWrap_triggered();
    void connection_messageReceived(const QByteArray &message);
    void timer_timeout();

    void ansi_attributesChanged(const AnsiParser::Attributes &attr);
    void ansi_print_text(const QString &text);

    AnsiParser ansiParser;

    QTextCharFormat ansiFormat;
    QTextCursor cursor;

    QTimer timer;
    QString rx_buf;
};

#endif // CONSOLEWINDOW_H
