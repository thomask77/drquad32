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

#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "DockWindows/ConnectionWindow.h"
#include "DockWindows/UpdateWindow.h"
#include "DockWindows/ConsoleWindow.h"
#include "DockWindows/GLWindow.h"

#include "Connection.h"

#include <QAction>
#include <QDockWidget>
#include <QSettings>
#include <QCloseEvent>
#include <QDebug>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , connection(this)
    , ui(new Ui::MainWindow)
    , old_stats()
{
    ui->setupUi(this);

    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::close);
    connect(ui->actionFullscreen, &QAction::triggered, this, &MainWindow::actionFullscreen_triggered);

    ui->statusBar->addPermanentWidget(&statisticsLabel);
    ui->statusBar->addWidget(&connectionLabel);

    connectionWindow = new class ConnectionWindow(this);
    updateWindow     = new class UpdateWindow(this);
    consoleWindow    = new class ConsoleWindow(this);
    glWindow         = new class GLWindow(this);

    addDockWindow(Qt::LeftDockWidgetArea, connectionWindow);
    addDockWindow(Qt::RightDockWidgetArea, updateWindow);
    addDockWindow(Qt::BottomDockWidgetArea, consoleWindow);
    addDockWindow(Qt::BottomDockWidgetArea, glWindow);

    QSettings settings;
    restoreGeometry(settings.value("windowGeometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());

    ui->actionFullscreen->setChecked(windowState() & Qt::WindowFullScreen);

    connect(&timer, &QTimer::timeout, this, &MainWindow::timer_timeout);
    timer.start(500);
}

#ifdef Q_OS_WIN

#include <qt_windows.h>
#include <dbt.h>

bool MainWindow::nativeEvent(const QByteArray &, void *message, long *)
{
    MSG *msg = (MSG*)message;

    if (msg->message == WM_DEVICECHANGE) {
        qDebug("WM_DEVICECHANGE: %x", msg->wParam);

        if (msg->wParam == DBT_DEVICEARRIVAL ||
            msg->wParam == DBT_DEVICEREMOVECOMPLETE)
        {
            emit deviceChanged();
        }
    }
    return false;
}

#else

bool MainWindow::nativeEvent(const QByteArray &, void *, long *)
{
    return false;
}

#endif

void MainWindow::timer_timeout()
{
    if (connection.isOpen())
        connectionLabel.setText(connection.portName());
    else
        connectionLabel.setText("Not connected");

    // Update RX/TX statistics
    //
    auto t = QTime::currentTime();
    auto dt = old_stats_t.msecsTo(t) * 1e-3;

    if (dt > 0) {
        statisticsLabel.setText(
            QString().sprintf("RX: %d Byte/s, TX: %d Byte/s",
                (int)((connection.stats.rx_bytes - old_stats.rx_bytes) / dt),
                (int)((connection.stats.tx_bytes - old_stats.tx_bytes) / dt)
            )
        );
    }

    old_stats = connection.stats;
    old_stats_t = t;
}


void MainWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings;
    settings.setValue("windowGeometry", saveGeometry());
    settings.setValue("windowState", saveState());
    event->accept();
}


void MainWindow::addDockWindow(Qt::DockWidgetArea area, QMainWindow *window)
{
    window->setWindowFlags(Qt::Widget);

    auto dw = new QDockWidget();

    dw->setWidget(window);
    dw->setObjectName(window->windowTitle());
    dw->setWindowTitle(window->windowTitle());
    dw->setAllowedAreas(Qt::AllDockWidgetAreas);

    addDockWidget(area, dw);

    ui->menuView->insertAction(
        ui->menuView->actions().end()[-2],
        dw->toggleViewAction()
    );
}

void MainWindow::actionFullscreen_triggered()
{
    if (ui->actionFullscreen->isChecked())
        this->showFullScreen();
    else
        this->showNormal();
}

MainWindow::~MainWindow()
{
    delete ui;
}
