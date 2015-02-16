/**
 * Copyright (C)2015 Thomas Kindler <mail_drquad@t-kindler.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ConsoleWindow.h"
#include "ui_ConsoleWindow.h"

#include <QSerialPort>
#include <QSerialPortInfo>
#include <QFileDialog>
#include <QDebug>
#include <QTime>
#include <QString>
#include <QTextStream>
#include <QMessageBox>
#include <QScrollBar>

#include "MainWindow.h"
#include "TryAction.h"

static const QBrush ansiPalette[] = {
    QBrush(QColor("#222")), QBrush(QColor("#C00")),
    QBrush(QColor("#2C0")), QBrush(QColor("#CC0")),
    QBrush(QColor("#00C")), QBrush(QColor("#C0C")),
    QBrush(QColor("#0CC")), QBrush(QColor("#CCC")),
    QBrush(QColor("#444")), QBrush(QColor("#F44")),
    QBrush(QColor("#4F4")), QBrush(QColor("#FF4")),
    QBrush(QColor("#44F")), QBrush(QColor("#F4F")),
    QBrush(QColor("#4FF")), QBrush(QColor("#FFF"))
};


ConsoleWindow::ConsoleWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ConsoleWindow)
{
    ui->setupUi(this);

    connect(ui->actionClear, &QAction::triggered, this, &ConsoleWindow::actionClear_triggered);
    connect(ui->actionSave, &QAction::triggered, this, &ConsoleWindow::actionSave_triggered);
    connect(ui->actionWrap, &QAction::triggered, this, &ConsoleWindow::actionWrap_triggered);

    connect(&ansiParser, &AnsiParser::attributesChanged, this, &ConsoleWindow::ansi_attributesChanged);
    connect(&ansiParser, &AnsiParser::printText, this, &ConsoleWindow::ansi_print_text);

    connect(&mainWindow->connection, &Connection::messageReceived, this, &ConsoleWindow::connection_messageReceived);
    connect(&timer, &QTimer::timeout, this, &ConsoleWindow::timer_timeout);

    cursor = QTextCursor(ui->plainTextEdit->document());

    auto p = ui->plainTextEdit->palette();
    p.setColor(QPalette::All, QPalette::Base, ansiPalette[0].color());
    p.setColor(QPalette::All, QPalette::Text, ansiPalette[7].color());
    ui->plainTextEdit->setPalette(p);

    auto font = ui->plainTextEdit->font();

#ifdef Q_OS_WIN
    font.setFamily("Lucida Console");
#else
    font.setFamily("Liberation Mono");
#endif

    ui->plainTextEdit->setFont(font);

    actionClear_triggered();
    timer.start(100);
}


ConsoleWindow::~ConsoleWindow()
{
    delete ui;
}


void ConsoleWindow::actionClear_triggered()
{
    ui->plainTextEdit->clear();
    ansi_attributesChanged(ansiParser.attributes);
}


void ConsoleWindow::actionSave_triggered()
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

    tryAction(
        [&]() { return f.open(QIODevice::WriteOnly); },
        [&]() { return QString("Can't save\n%1\n%2")
                        .arg(fn) .arg(f.errorString());
        }
    );

    f.write(ui->plainTextEdit->toPlainText().toLatin1());
    f.close();
}


void ConsoleWindow::actionWrap_triggered()
{
    if (ui->actionWrap->isChecked())
        ui->plainTextEdit->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    else
        ui->plainTextEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
}


void ConsoleWindow::connection_messageReceived(const msg_generic &msg)
{
    if (msg.h.id == MSG_ID_SHELL_TO_PC)
        rx_buf.append( QByteArray((char*)msg.data, msg.h.data_len) );
}


void ConsoleWindow::timer_timeout()
{
    if (ui->actionPause->isChecked())
        return;

    if (rx_buf.isEmpty())
        return;

    auto pte = ui->plainTextEdit;
    auto vsb = pte->verticalScrollBar();
    bool at_bottom = vsb->value() == vsb->maximum();

    pte->setUpdatesEnabled(false);

    ansiParser.parse(rx_buf);
    if (at_bottom)
        vsb->setValue(vsb->maximum());

    pte->setUpdatesEnabled(true);

    rx_buf.clear();
}


void ConsoleWindow::ansi_attributesChanged(const AnsiParser::Attributes &attr)
{
    unsigned fg = attr.foreground + (attr.bold ? 8 : 0);
    unsigned bg = attr.background;

    Q_ASSERT(fg < 16);
    Q_ASSERT(bg < 16);

    ansiFormat.setForeground(ansiPalette[fg]);
    ansiFormat.setBackground(ansiPalette[bg]);

    cursor.setCharFormat(ansiFormat);
}


void ConsoleWindow::ansi_print_text(const QString &text)
{
    cursor.insertText(text);
}
