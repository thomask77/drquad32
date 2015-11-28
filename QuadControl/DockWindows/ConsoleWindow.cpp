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
#include "MainWindow.h"
#include "TryAction.h"

#include <QFileDialog>

ConsoleWindow::ConsoleWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ConsoleWindow)
{
    ui->setupUi(this);
    this->setContextMenuPolicy(Qt::NoContextMenu);

    connect(ui->actionClear, &QAction::triggered, this, &ConsoleWindow::actionClear_triggered);
    connect(ui->actionSave, &QAction::triggered, this, &ConsoleWindow::actionSave_triggered);
    connect(ui->actionWrap, &QAction::triggered, this, &ConsoleWindow::actionWrap_triggered);

    connect(&mainWindow->connection, &Connection::messageReceived, this, &ConsoleWindow::connection_messageReceived);
    connect(&timer, &QTimer::timeout, this, &ConsoleWindow::timer_timeout);

    timer.start(30);
}


ConsoleWindow::~ConsoleWindow()
{
    delete ui;
}


void ConsoleWindow::actionClear_triggered()
{
    ui->terminal->clear();
    ui->terminal->reset();
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
        [&] { return f.open(QIODevice::WriteOnly); },
        [&] { return QString("Can't save\n%1\n%2")
                        .arg(fn) .arg(f.errorString());
        }
    );

    f.write(ui->terminal->toPlainText().toLatin1());
    f.close();
}


void ConsoleWindow::actionWrap_triggered()
{
    if (ui->actionWrap->isChecked())
        ui->terminal->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    else
        ui->terminal->setLineWrapMode(QPlainTextEdit::NoWrap);
}


void ConsoleWindow::connection_messageReceived(const msg_generic &msg)
{
    if (msg.h.id == MSG_ID_SHELL_TO_PC) {
        auto m = (const msg_shell_to_pc&)msg;
        rxBuf += QLatin1String(m.data, m.h.data_len);
    }
}


void ConsoleWindow::timer_timeout()
{
    for (;;) {
        msg_shell_from_pc msg;

        auto buf = ui->terminal->read( sizeof(msg.data) );
        if (buf.isEmpty())
            break;

        msg.h.id = MSG_ID_SHELL_FROM_PC;
        msg.h.data_len = buf.length();
        memcpy(msg.data, buf.toLatin1(), msg.h.data_len);

        mainWindow->connection.sendMessage(&msg.h);
    }

    if (!rxBuf.isEmpty() && !ui->actionPause->isChecked())  {
        ui->terminal->write(rxBuf);
        rxBuf.clear();
    }
}

