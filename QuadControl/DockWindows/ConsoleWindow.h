#ifndef CONSOLEWINDOW_H
#define CONSOLEWINDOW_H

#include "Shared/msg_structs.h"

#include <QTimer>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QMainWindow>
#include "../AnsiParser.h"

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

    AnsiParser ansiParser;

    QTextCharFormat ansiFormat;
    QTextCursor cursor;

    QTimer timer;
    QString rx_buf;
    QString tx_buf;

    bool eventFilter(QObject *obj, QEvent *event) override;

    void actionClear_triggered();
    void actionSave_triggered();
    void actionWrap_triggered();
    void connection_messageReceived(const msg_generic &msg);
    void timer_timeout();

    void ansi_attributesChanged(const AnsiParser::Attributes &attr);
    void ansi_print_text(const QString &text);
};

#endif // CONSOLEWINDOW_H
