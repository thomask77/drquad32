#include "ConsoleWindow.h"
#include "ui_ConsoleWindow.h"

#include <QSerialPort>
#include <QSerialPortInfo>
#include <QFileDialog>
#include <QTime>
#include <QString>
#include <QTextStream>
#include <QMessageBox>
#include <QScrollBar>

#include "MainWindow.h"

ConsoleWindow::ConsoleWindow(MainWindow *parent) :
    QMainWindow(parent),
    ui(new Ui::ConsoleWindow),
    mainWindow(parent)
{
    ui->setupUi(this);

    connect(ui->action_clear, &QAction::triggered, this, &ConsoleWindow::action_clear_triggered);
    connect(ui->action_save, &QAction::triggered, this, &ConsoleWindow::action_save_triggered);
    connect(ui->action_wrap, &QAction::triggered, this, &ConsoleWindow::action_wrap_triggered);

    cursor = QTextCursor(ui->plainTextEdit->document());

    connect(&ansi_parser, &AnsiParser::attributesChanged, this, &ConsoleWindow::ansi_attrib_changed);
    connect(&ansi_parser, &AnsiParser::printText, this, &ConsoleWindow::ansi_print_text);

/*
      self.queue = Queue()
      dispatcher.subscribe(self.queue, msg_shell_to_pc.MSG_ID)
*/

    connect(&timer, &QTimer::timeout, this, &ConsoleWindow::timer_timeout);
    timer.start(100);
}


void ConsoleWindow::action_clear_triggered()
{
    ui->plainTextEdit->clear();
}


void ConsoleWindow::action_save_triggered()
{
    auto t = QDateTime::currentDateTime();

    auto fn = QString().sprintf(
        "log_%02d%02d%02d_%02d%02d%02d.txt",
        t.date().year() % 100, t.date().month(), t.date().day(),
        t.time().hour(), t.time().minute(), t.time().second()
    );

    auto modifiers = QApplication::keyboardModifiers();

    if (modifiers != Qt::ShiftModifier) {
        fn = QFileDialog::getSaveFileName(
            this, "Save console output", fn,
            "Text files (*.txt);;All Files (*.*)"
        );
    }

    if (fn == "")
        return;

    QFile f(fn);

    while (!f.open(QIODevice::WriteOnly)) {
        if (QMessageBox::critical(
            this, "Can't save log file", f.errorString(),
            QMessageBox::Abort | QMessageBox::Retry
        ) != QMessageBox::Retry) {
            return;
        }
    }

    f.write(ui->plainTextEdit->toPlainText().toLatin1());
    f.close();
}


void ConsoleWindow::action_wrap_triggered()
{
    if (ui->action_wrap->isChecked())
        ui->plainTextEdit->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    else
        ui->plainTextEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
}


void ConsoleWindow::timer_timeout()
{
    if (ui->action_pause->isChecked())
        return;

    // Combine messages
    //
    QString text;

    // while not self.queue.empty():
    //     msg = self.queue.get()
    //     text += msg[2:]

    if (text == "")
        return;

    auto pte = ui->plainTextEdit;
    auto vsb = pte->verticalScrollBar();

    bool at_bottom = vsb->value() == vsb->maximum();

    pte->setUpdatesEnabled(false);

    ansi_parser.parse(text);

    if (at_bottom)
        vsb->setValue(vsb->maximum());

    pte->setUpdatesEnabled(true);
}


void ConsoleWindow::ansi_attrib_changed(const AnsiParser::Attributes &attr)
{
    static const QBrush ansi_palette[] = {
        QBrush(QColor("#000")), QBrush(QColor("#C00")),
        QBrush(QColor("#0C0")), QBrush(QColor("#CC0")),
        QBrush(QColor("#00C")), QBrush(QColor("#C0C")),
        QBrush(QColor("#0CC")), QBrush(QColor("#CCC")),
        QBrush(QColor("#444")), QBrush(QColor("#F44")),
        QBrush(QColor("#4F4")), QBrush(QColor("#FF4")),
        QBrush(QColor("#44F")), QBrush(QColor("#F4F")),
        QBrush(QColor("#4FF")), QBrush(QColor("#FFF"))
    };

    unsigned fg = attr.foreground + (attr.bold ? 8 : 0);
    unsigned bg = attr.background;

    Q_ASSERT(fg < 16);
    Q_ASSERT(bg < 16);

    ansi_format.setForeground(ansi_palette[fg]);
    ansi_format.setBackground(ansi_palette[bg]);

    cursor.setCharFormat(ansi_format);
}


void ConsoleWindow::ansi_print_text(const QString &text)
{
    cursor.insertText(text);
}


ConsoleWindow::~ConsoleWindow()
{
    delete ui;
}

