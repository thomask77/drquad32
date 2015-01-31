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

#include "ConnectionWindow.h"
#include "ui_ConnectionWindow.h"

#include <QSerialPort>
#include <QMessageBox>
#include <QDebug>

#include "MainWindow.h"
#include "PuTTYLauncher.h"


ConnectionWindow::ConnectionWindow(MainWindow *parent)
    : QMainWindow(parent)
    , mainWindow(parent)
    , ui(new Ui::ConnectionWindow)
{
    ui->setupUi(this);
    ui->treeWidget->sortByColumn(0, Qt::AscendingOrder);
    ui->treeWidget->setColumnWidth(0, 120);
    ui->treeWidget->setColumnWidth(1, 0);

    connect(ui->actionConnect, &QAction::triggered, this, &ConnectionWindow::actionConnect_triggered);
    connect(ui->actionDisconnect, &QAction::triggered, this, &ConnectionWindow::actionDisconnect_triggered);
    connect(ui->actionTerminal, &QAction::triggered, this, &ConnectionWindow::actionTerminal_triggered);

    connect(ui->treeWidget, &QTreeWidget::currentItemChanged, this, &ConnectionWindow::treewidget_currentItemChanged);
    connect(ui->treeWidget, &QTreeWidget::itemActivated, this, &ConnectionWindow::actionConnect_triggered);

    connect(&mainWindow->connection, &Connection::connectionChanged, this, &ConnectionWindow::connectionChanged);
    connect(&timer, &QTimer::timeout, this, &ConnectionWindow::timer_timeout);

    boldFont.setBold(true);

    ui->treeWidget->addTopLevelItem(&serialItems);
    serialItems.setText(0, "Serial Ports");
    serialItems.setFont(0, boldFont);
    serialItems.setFlags(Qt::ItemIsEnabled);
    serialItems.setFirstColumnSpanned(true);
    serialItems.setExpanded(true);

    ui->treeWidget->addTopLevelItem(&wiFlyItems);
    wiFlyItems.setText(0, "WiFly Modules");
    wiFlyItems.setFont(0, boldFont);
    wiFlyItems.setFlags(Qt::ItemIsEnabled);
    wiFlyItems.setFirstColumnSpanned(true);
    wiFlyItems.setExpanded(true);

    ui->treeWidget->addTopLevelItem(&favoriteItems);
    favoriteItems.setText(0, "Favorites");
    favoriteItems.setFont(0, boldFont);
    favoriteItems.setFlags(Qt::ItemIsEnabled);
    favoriteItems.setFirstColumnSpanned(true);
    favoriteItems.setExpanded(true);

    timer.setInterval(1000);
    timer.start();
}


ConnectionWindow::~ConnectionWindow()
{
    delete ui;
}


static void setItemColor(QTreeWidgetItem *item, const QColor &color)
{
    for (int i=0; i<item->columnCount(); i++)
        item->setTextColor(i, color);
}


static QTreeWidgetItem *findChildOrNew(QTreeWidgetItem *item, const QString &text)
{
    for (int i=0; i<item->childCount(); i++)
        if (item->child(i)->text(0) == text)
            return item->child(i);

    auto newItem = new QTreeWidgetItem();
    item->addChild(newItem);

    return newItem;
}


void ConnectionWindow::timer_timeout()
{
    ui->treeWidget->setUpdatesEnabled(false);

    // Update serial ports
    //
    auto ports = availablePortsAsync();

    for (const auto &p: ports) {
        auto item = findChildOrNew(&serialItems, p.portName());

        item->setText(0, p.portName());
        item->setText(1, p.systemLocation());
        item->setText(2, p.description());
    }

    for (int i=0; i<serialItems.childCount(); i++) {
        bool found = false;
        for (const auto &p: ports) {
            if (p.portName() == serialItems.child(i)->text(0)) {
                found = true;
                break;
            }
        }

        setItemColor(serialItems.child(i), found ? Qt::black : Qt::gray);
    }

    // Update WiFly modules
    //
    for (const auto &ci: wiFlyListener.clients) {
        auto item = findChildOrNew(&wiFlyItems, ci.address.toString());

        item->setText(0, ci.address.toString());
        item->setText(1, QString("RSSI: %1").arg(ci.rssi));
        item->setText(2, ci.deviceId);

        auto age = ci.lastSeen.secsTo(QDateTime::currentDateTime());
        setItemColor(item, age < 10 ? Qt::black : Qt::gray);
    }

    ui->treeWidget->setUpdatesEnabled(true);
}


QTreeWidgetItem *ConnectionWindow::getSelectedItem()
{
    if (!ui->treeWidget)
        return NULL;

    auto item = ui->treeWidget->currentItem();

    // ignore top-level items
    //
    if (!item || !item->parent())
        return NULL;

    return item;
}


void ConnectionWindow::actionConnect_triggered()
{
    auto item = getSelectedItem();
    if (!item)
        return;

    bool res = false;
    while (!res) {
        auto portName = item->text(0);

        QApplication::setOverrideCursor(Qt::WaitCursor);
        res = mainWindow->connection.open( QSerialPortInfo(portName) );
        QApplication::restoreOverrideCursor();

        if (!res && QMessageBox::critical(
            mainWindow, "Error",
            QString("Can't open\n%1\n%2")
                .arg(portName)
                .arg(mainWindow->connection.errorString()),
            QMessageBox::Abort | QMessageBox::Retry
        ) != QMessageBox::Retry) {
            break;
        }
    }
}


void ConnectionWindow::actionTerminal_triggered()
{
    // TODO: disconnect if connected to current item
    //

    auto putty = new PuTTYLauncher(mainWindow);
    auto qmb   = new QMessageBox(mainWindow);

    connect( putty, &PuTTYLauncher::finished, [=](){ qmb->accept(); } );

    qmb->setModal(true);
    qmb->setText("Waiting for PuTTY...");
    qmb->setStandardButtons(QMessageBox::Cancel);
    qmb->setIcon(QMessageBox::Information);

    bool res = false;

    while (!res) {
        QApplication::setOverrideCursor(Qt::WaitCursor);
        res = putty->startSerial("COM26", 115200);
        QApplication::restoreOverrideCursor();

        if (!res && QMessageBox::critical(
            mainWindow, "Error",
            QString("Can't launch\n%1\n%2")
                .arg(putty->puttyFilename)
                .arg(putty->errorString()),
            QMessageBox::Abort | QMessageBox::Retry
        ) != QMessageBox::Retry) {
            break;
        }
    }

    if (!res || (qmb->exec() != QMessageBox::Cancel)) {
        // reconnect if disconnected
        //
    }
}


void ConnectionWindow::actionDisconnect_triggered()
{
    mainWindow->connection.close();
}


void ConnectionWindow::treewidget_currentItemChanged()
{
    auto item = getSelectedItem();
    ui->actionConnect->setEnabled(!!item);
    ui->actionTerminal->setEnabled(!!item);
}


void ConnectionWindow::connectionChanged()
{
    auto c = (Connection*)sender();
    ui->actionDisconnect->setEnabled( c->isOpen() );
}

