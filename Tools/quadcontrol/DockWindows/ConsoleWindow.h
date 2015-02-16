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

    void action_clear_triggered();
    void action_save_triggered();
    void action_wrap_triggered();

    void timer_timeout();

    void ansi_attrib_changed(const AnsiParser::Attributes &attr);
    void ansi_print_text(const QString &text);

    QTimer timer;
    QTextCursor cursor;
    QTextCharFormat ansi_format;
    AnsiParser ansi_parser;
};

#endif // CONSOLEWINDOW_H
