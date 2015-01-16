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
#include <QSerialPortInfo>
#include <QDebug>

#include "MainWindow.h"


ConnectionWindow::ConnectionWindow(MainWindow *parent)
    : QMainWindow(parent)
    , ui(new Ui::ConnectionWindow)
    , mainWindow(parent)
{
    ui->setupUi(this);
    ui->treeWidget->sortByColumn(0, Qt::AscendingOrder);
    ui->treeWidget->setColumnWidth(0, 80);
    ui->treeWidget->setColumnWidth(1, 0);

    connect(ui->actionScan, &QAction::triggered, this, &ConnectionWindow::actionScan_triggered);
    connect(ui->actionConnect, &QAction::triggered, this, &ConnectionWindow::actionConnect_triggered);
    connect(ui->actionDisconnect, &QAction::triggered, this, &ConnectionWindow::actionDisconnect_triggered);
    connect(ui->treeWidget, &QTreeWidget::currentItemChanged, this, &ConnectionWindow::treewidget_currentItemChanged);
    connect(ui->treeWidget, &QTreeWidget::itemActivated, this, &ConnectionWindow::actionConnect_triggered);
    connect(mainWindow, &MainWindow::deviceChanged, this, &ConnectionWindow::actionScan_triggered);
    connect(&mainWindow->connection, &Connection::connectionChanged, this, &ConnectionWindow::connectionChanged);

    actionScan_triggered();
}

ConnectionWindow::~ConnectionWindow()
{
    delete ui;
}

void ConnectionWindow::actionScan_triggered()
{
    // Find Serial ports
    //
    auto serial_items = new QTreeWidgetItem();

    for (auto p: QSerialPortInfo::availablePorts()) {
        auto item = new QTreeWidgetItem();

        item->setText(0, p.portName());
        item->setText(1, p.systemLocation());
        item->setText(2, p.description());

        serial_items->addChild(item);
    }

    ui->treeWidget->setUpdatesEnabled(false);

    ui->treeWidget->clear();
    ui->treeWidget->addTopLevelItem(serial_items);

    auto bold_font = serial_items->font(0);
    bold_font.setBold(true);

    serial_items->setText(0, "Serial Ports");
    serial_items->setFont(0, bold_font);
    serial_items->setFlags(Qt::ItemIsEnabled);
    serial_items->setExpanded(true);
    serial_items->setFirstColumnSpanned(true);

    ui->treeWidget->setUpdatesEnabled(true);
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

void ConnectionWindow::actionConnect_triggered()
{
    auto item = getSelectedItem();
    if (!item)
        return;

    mainWindow->connection.open( QSerialPortInfo(item->text(0)) );
}

void ConnectionWindow::actionDisconnect_triggered()
{
    mainWindow->connection.close();
}

void ConnectionWindow::treewidget_currentItemChanged()
{
    auto item = getSelectedItem();
    ui->actionConnect->setEnabled(!!item);
}

void ConnectionWindow::connectionChanged()
{
    auto c = (Connection*)sender();
    ui->actionDisconnect->setEnabled( c->isOpen() );
}

