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
#include <QInputDialog>
#include <QDebug>

#include "MainWindow.h"
#include "PuTTYLauncher.h"


ConnectionWindow::ConnectionWindow(MainWindow *parent)
    : QMainWindow(parent)
    , ui(new Ui::ConnectionWindow)
    , mainWindow(parent)
{
    ui->setupUi(this);
    ui->treeWidget->sortByColumn(0, Qt::AscendingOrder);
    ui->treeWidget->setColumnWidth(0, 120);
    ui->treeWidget->setColumnWidth(1, 0);

    connect(ui->actionAdd, &QAction::triggered, this, &ConnectionWindow::actionAdd_triggered);
    connect(ui->actionRemove, &QAction::triggered, this, &ConnectionWindow::actionRemove_triggered);
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


QTreeWidgetItem *ConnectionWindow::getSelectedItem()
{
    auto item = ui->treeWidget->currentItem();

    // ignore top-level items
    //
    if (!item || !item->parent())
        return NULL;

    return item;
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

        auto path = QString("serial://%1?115200").arg(p.portName());
        item->setData(0, Qt::UserRole, path);
        item->setToolTip(0, path);
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

        auto path = QString("wifly://%1:%2") .arg(ci.address.toString()) .arg(ci.localPort);

        item->setData(0, Qt::UserRole, path);
        item->setToolTip(0, path);
    }

    ui->treeWidget->setUpdatesEnabled(true);
}


void ConnectionWindow::actionAdd_triggered()
{
    QInputDialog qid;

    if (getSelectedItem())
        qid.setTextValue(getSelectedItem()->data(0, Qt::UserRole).toString());

    if (qid.exec() != QDialog::Accepted)
        return;

    auto item = new QTreeWidgetItem();
    item->setText(0, qid.textValue());
    item->setData(0,  Qt::UserRole, qid.textValue());
    favoriteItems.addChild(item);

    item->setFirstColumnSpanned(true);
}


void ConnectionWindow::actionRemove_triggered()
{
    auto item = getSelectedItem();
    if (item)
        item->parent()->removeChild(item);
}


void ConnectionWindow::actionConnect_triggered()
{
    auto item = getSelectedItem();
    if (!item)
        return;

    bool res = false;
    while (!res) {

        QApplication::setOverrideCursor(Qt::WaitCursor);
        QApplication::processEvents();

        auto path = item->data(0, Qt::UserRole).toString();
        res = mainWindow->connection.open(path);

        QApplication::restoreOverrideCursor();

        if (!res && QMessageBox::critical(
            mainWindow, "Error",
            QString("Can't open\n%1\n%2")
                .arg(path)
                .arg(mainWindow->connection.errorString()),
            QMessageBox::Abort | QMessageBox::Retry
        ) != QMessageBox::Retry) {
            break;
        }
    }
}


void ConnectionWindow::actionTerminal_triggered()
{
    auto item = getSelectedItem();
    if (!item)
        return;

    // TODO: disconnect if connected to current item
    //
    auto putty = new PuTTYLauncher(mainWindow);

    bool res = false;
    while (!res) {
        auto portName = item->text(0);

        QApplication::setOverrideCursor(Qt::WaitCursor);
        QApplication::processEvents();

        auto path = item->data(0, Qt::UserRole).toString();
        res = putty->openUrl(path);

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

    QMessageBox qmb;
    qmb.setText("Waiting for PuTTY...");
    qmb.setStandardButtons(QMessageBox::Cancel);
    qmb.setIcon(QMessageBox::Information);

    connect( putty, &PuTTYLauncher::finished, &qmb, &QMessageBox::accept );

    if (!res || (qmb.exec() != QMessageBox::Cancel)) {
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
    ui->actionConnect->setEnabled(item);
    ui->actionTerminal->setEnabled(item);
    ui->actionRemove->setEnabled(item && item->parent() == &favoriteItems);
}


void ConnectionWindow::connectionChanged()
{
    auto c = (Connection*)sender();
    ui->actionDisconnect->setEnabled( c->isOpen() );
}

