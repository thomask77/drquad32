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
#include "AddConnectionDialog.h"
#include "TryAction.h"

#include <QSerialPort>
#include <QMessageBox>
#include <QDebug>
#include <QSettings>

#include "MainWindow.h"
#include "PuTTYLauncher.h"


ConnectionWindow::ConnectionWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ConnectionWindow)
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

    serialItems = new QTreeWidgetItem();
    ui->treeWidget->addTopLevelItem(serialItems);
    serialItems->setText(0, "Serial Ports");
    serialItems->setFont(0, boldFont);
    serialItems->setFlags(Qt::ItemIsEnabled);
    serialItems->setFirstColumnSpanned(true);
    serialItems->setExpanded(true);

    wiFlyItems = new QTreeWidgetItem();
    ui->treeWidget->addTopLevelItem(wiFlyItems);
    wiFlyItems->setText(0, "WiFly Modules");
    wiFlyItems->setFont(0, boldFont);
    wiFlyItems->setFlags(Qt::ItemIsEnabled);
    wiFlyItems->setFirstColumnSpanned(true);
    wiFlyItems->setExpanded(true);

    favoriteItems = new QTreeWidgetItem();
    ui->treeWidget->addTopLevelItem(favoriteItems);
    favoriteItems->setText(0, "Favorites");
    favoriteItems->setFont(0, boldFont);
    favoriteItems->setFlags(Qt::ItemIsEnabled);
    favoriteItems->setFirstColumnSpanned(true);
    favoriteItems->setExpanded(true);

    QSettings settings;
    int size = settings.beginReadArray("favorites");
    for (int i=0; i<size; i++) {
        settings.setArrayIndex(i);
        auto item = new QTreeWidgetItem();
        favoriteItems->addChild(item);

        item->setText(0, settings.value("name").toString());
        item->setText(1, settings.value("hwid").toString());
        item->setText(2, settings.value("desc").toString());
        item->setToolTip(0, settings.value("url").toString());
        item->setData(0, Qt::UserRole, settings.value("url").toUrl());
    }
    settings.endArray();

    timer.setInterval(1000);
    timer.start();
}


ConnectionWindow::~ConnectionWindow()
{
    QSettings settings;
    settings.beginWriteArray("favorites");

    for (int i=0; i<favoriteItems->childCount(); i++) {
        auto item = favoriteItems->child(i);
        settings.setArrayIndex(i);
        settings.setValue("name", item->text(0));
        settings.setValue("hwid", item->text(1));
        settings.setValue("desc", item->text(2));
        settings.setValue("url",  item->data(0, Qt::UserRole));
    }

    settings.endArray();

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
        auto item = findChildOrNew(serialItems, p.portName());

        item->setText(0, p.portName());
        item->setText(1, p.systemLocation());
        item->setText(2, p.description());

        QUrl url;
        url.setScheme("serial");
        url.setPath(p.portName());
        url.setQuery("115200");

        item->setData(0, Qt::UserRole, url);
        item->setToolTip(0, url.toEncoded());
    }

    for (int i=0; i<serialItems->childCount(); i++) {
        auto child = serialItems->child(i);
        bool found = false;
        for (const auto &p: ports) {
            if (p.portName() == child->text(0)) {
                found = true;
                break;
            }
        }

        setItemColor(child, found ? Qt::black : Qt::gray);
    }

    // Update WiFly modules
    //
    for (const auto &ci: wiFlyListener.clients) {
        auto item = findChildOrNew(wiFlyItems, ci.address.toString());

        item->setText(0, ci.address.toString());
        item->setText(2, QString("%1, RSSI=%2").arg(ci.deviceId).arg(ci.rssi));

        auto age = ci.lastSeen.secsTo(QDateTime::currentDateTime());
        setItemColor(item, age < 10 ? Qt::black : Qt::gray);

        QUrl url;
        url.setScheme("wifly");
        url.setHost(ci.address.toString());
        url.setPort(ci.localPort);

        item->setData(0, Qt::UserRole, url);
        item->setToolTip(0, url.toString());
    }

    ui->treeWidget->setUpdatesEnabled(true);
}


void ConnectionWindow::actionAdd_triggered()
{
    AddConnectionDialog dlg(mainWindow);

    auto item = getSelectedItem();
    if (item) {
        dlg.ui->qle_name->setText(item->text(0));
        dlg.ui->qle_url->setText(item->data(0, Qt::UserRole).toString());
        dlg.ui->qle_desc->setText(item->text(2));
    }

    if (dlg.exec() != QDialog::Accepted)
        return;

    item = findChildOrNew(favoriteItems, dlg.ui->qle_name->text());
    item->setText(0, dlg.ui->qle_name->text());
    item->setText(2, dlg.ui->qle_desc->text());
    item->setToolTip(0, dlg.ui->qle_url->text());
    item->setData(0,  Qt::UserRole, QUrl(dlg.ui->qle_url->text()));
}


void ConnectionWindow::actionRemove_triggered()
{
    auto item = getSelectedItem();
    if (item)
        item->parent()->removeChild(item);
}


bool ConnectionWindow::tryConnect(QTreeWidgetItem *item)
{
    if (!item)
        return false;

    actionDisconnect_triggered();

    auto url = item->data(0, Qt::UserRole).toUrl();
    auto ret = tryAction(
        [&]() { return mainWindow->connection.openUrl(url); },
        [&]() { return QString("Can't open\n%1\n%2")
                    .arg(url.toString())
                    .arg(mainWindow->connection.errorString());
        }
    );

    if (ret)
        item->setFont(0, boldFont);

    return ret;
}


void ConnectionWindow::actionConnect_triggered()
{
    tryConnect(getSelectedItem());
}


void ConnectionWindow::actionTerminal_triggered()
{
    auto item = getSelectedItem();
    if (!item)
        return;

    auto url = item->data(0, Qt::UserRole).toUrl();
    auto old_url = mainWindow->connection.getUrl();

    // Disconnect if already connected
    //
    if (url == old_url)
        actionDisconnect_triggered();

    auto putty = new PuTTYLauncher(mainWindow);
    auto res = tryAction(
        [&]() { return putty->openUrl(url); },
        [&]() { return QString("Can't launch\n%1\n%2")
                    .arg(putty->puttyFilename)
                    .arg(putty->errorString());
        }
    );

    QMessageBox qmb(mainWindow);
    qmb.setText("Waiting for PuTTY...");
    qmb.setStandardButtons(QMessageBox::Cancel);
    qmb.setIcon(QMessageBox::Information);

    connect( putty, &PuTTYLauncher::finished, &qmb, &QMessageBox::accept );

    if (!res || (qmb.exec() != QMessageBox::Cancel)) {
        // Reconnect (only) if we have disconnected
        //
        if (url == old_url)
            tryConnect(item);
    }
}


void ConnectionWindow::actionDisconnect_triggered()
{
    mainWindow->connection.close();

    for (auto i: { favoriteItems, serialItems, wiFlyItems })
        for (int j=0; j < i->childCount(); j++)
            i->child(j)->setFont(0, ui->treeWidget->font());
}


void ConnectionWindow::treewidget_currentItemChanged()
{
    auto item = getSelectedItem();
    ui->actionConnect->setEnabled(item);
    ui->actionTerminal->setEnabled(item);
    ui->actionAdd->setEnabled(true);
    ui->actionRemove->setEnabled(item && item->parent() == favoriteItems);
}


void ConnectionWindow::connectionChanged()
{
    ui->actionDisconnect->setEnabled( mainWindow->connection.isOpen() );
}

