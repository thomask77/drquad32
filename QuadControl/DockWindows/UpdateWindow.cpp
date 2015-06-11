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

#include "UpdateWindow.h"

#include "ui_UpdateWindow.h"
#include <QFileSystemModel>
#include <QSettings>
#include <QCompleter>
#include <QMimeData>
#include <QFileDialog>
#include <QDebug>
#include <QProgressDialog>
#include <QMessageBox>
#include <QByteArray>
#include <QTime>

#include "IntelHexFile.h"
#include "TryAction.h"
#include "MainWindow.h"


UpdateWindow::UpdateWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::UpdateWindow)
{
    ui->setupUi(this);

    connect(ui->browseButton, &QPushButton::clicked, this, &UpdateWindow::browseButton_clicked);
    connect(ui->updateButton, &QPushButton::clicked, this, &UpdateWindow::updateButton_clicked);
    connect(ui->rebootButton, &QPushButton::clicked, this, &UpdateWindow::rebootButton_clicked);
    connect(ui->lineEdit, &QLineEdit::textChanged, this, &UpdateWindow::lineEdit_textChanged);
    connect(&mainWindow->connection, &Connection::connectionChanged, this, &UpdateWindow::connection_changed);

    auto model = new QFileSystemModel(this);
    model->setRootPath(QDir::currentPath());
    model->setNameFilters(QStringList { "*.hex" });
    model->setNameFilterDisables(true);

    auto completer = new QCompleter(this);
    completer->setModel(model);
    completer->setCaseSensitivity(Qt::CaseInsensitive);

    // Note: This call may be slow on some windows machines.
    // Check if there are any dead network drives or CD-ROMs
    // that take a long time to load.
    //
    ui->lineEdit->setCompleter(completer);

    QSettings s;
    s.beginGroup(objectName());
    ui->lineEdit->setText(s.value("filename").toString());
    ui->cbUpdateOnFileChange->setChecked(s.value("onChange").toBool());
    ui->sbAckWindow->setValue( s.value("ackWindow", ui->sbAckWindow->value() ).toInt());

    connect(&timer, &QTimer::timeout, this, &UpdateWindow::timer_timeout);
    timer.start(1000);
}


UpdateWindow::~UpdateWindow()
{
    QSettings s;
    s.beginGroup(objectName());
    s.setValue("filename", ui->lineEdit->text());
    s.setValue("onChange", ui->cbUpdateOnFileChange->isChecked());
    s.setValue("ackWindow", ui->sbAckWindow->value());
    delete ui;
}


void UpdateWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->urls().size() == 1 &&
        event->mimeData()->urls()[0].isLocalFile())
        event->acceptProposedAction();
}


void UpdateWindow::dropEvent(QDropEvent *event)
{
    ui->lineEdit->setText(
        event->mimeData()->urls()[0].toLocalFile()
    );
}


void UpdateWindow::browseButton_clicked()
{
    auto fn = QFileDialog().getOpenFileName(
        this, "Open file", ui->lineEdit->text(),
        "Intel HEX (*.hex);;All Files (*.*)"
    );

    if (fn == "")
        return;

    ui->lineEdit->setText(fn);
}


void UpdateWindow::updateButton_clicked()
{
    if (!mainWindow->connection.isOpen())
        return;

    BootProtocol bp(mainWindow->connection);

    bp.ack_window = ui->sbAckWindow->value();

    tryAction(
        [&]() { return bp.sendHexFile(ui->lineEdit->text()); },
        [&]() { return QString("Firmware update failed\n%1\n%2")
                    .arg(ui->lineEdit->text())
                    .arg(bp.errorString());
        }, Qt::ArrowCursor
    );
}


void UpdateWindow::rebootButton_clicked()
{
    BootProtocol(mainWindow->connection, mainWindow).bootReboot();
}


void UpdateWindow::connection_changed()
{
    auto en = mainWindow->connection.isOpen();
    ui->updateButton->setEnabled(en);
    ui->rebootButton->setEnabled(en);
}


void UpdateWindow::lineEdit_textChanged()
{
    QFileInfo fi(ui->lineEdit->text());
    lastModified = fi.lastModified();
}


void UpdateWindow::timer_timeout()
{
    if (!ui->cbUpdateOnFileChange->isChecked())
        return;

    QFileInfo fi(ui->lineEdit->text());
    if (fi.exists() && lastModified != fi.lastModified()) {
        updateButton_clicked();
        lastModified = fi.lastModified();
    }
}
