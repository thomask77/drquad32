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

#include <QSettings>
#include <QFileSystemModel>
#include <QCompleter>
#include <QMimeData>
#include <QFileDialog>
#include <QDebug>
#include <QProgressDialog>
#include <QMessageBox>
#include <QByteArray>
#include <QTime>

#include "MainWindow.h"
#include "IntelHexFile.h"

UpdateWindow::UpdateWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::UpdateWindow)
{
    ui->setupUi(this);

    ui->lineEdit->setText( QSettings().value("update_filename").toString() );

    connect(ui->lineEdit, &QLineEdit::textChanged, this, &UpdateWindow::lineEdit_textChanged);
    connect(ui->browseButton, &QPushButton::clicked, this, &UpdateWindow::browseButton_clicked);
    connect(ui->updateButton, &QPushButton::clicked, this, &UpdateWindow::updateButton_clicked);
    connect(&mainWindow->connection, &Connection::connectionChanged, this, &UpdateWindow::connectionChanged);

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
}

UpdateWindow::~UpdateWindow()
{
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
    for (;;) {
        BootProtocol bp(mainWindow->connection, mainWindow);

        if (bp.sendHexFile(ui->lineEdit->text()))
            return;

        if (QMessageBox::critical(
                mainWindow, "Error",
                QString("Firmware update failed\n%1\n%2")
                    .arg(ui->lineEdit->text())
                    .arg(bp.errorString()),
                QMessageBox::Abort | QMessageBox::Retry
        ) != QMessageBox::Retry) {
            return;
        }
    }
}

void UpdateWindow::lineEdit_textChanged()
{
    QSettings().setValue("update_filename", ui->lineEdit->text());
}

void UpdateWindow::connectionChanged()
{
    auto c = (Connection*)sender();
    ui->updateButton->setEnabled( c->isOpen() );
}
