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

#include <QAction>
#include <QDockWidget>
#include <QSettings>
#include <QCloseEvent>
#include <QMessageBox>
#include <QDebug>
#include "Connection.h"
#include "DockWindows/ConnectionWindow.h"
#include "DockWindows/ConsoleWindow.h"
#include "DockWindows/GLWindow.h"
#include "DockWindows/PlotWindow.h"
#include "DockWindows/UpdateWindow.h"


MainWindow *mainWindow;


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , connection(this)
    , ui(new Ui::MainWindow)
    , old_stats()
{
    ui->setupUi(this);

    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::close);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::actionAbout_triggered);
    connect(ui->actionAboutQt, &QAction::triggered, this, &MainWindow::actionAboutQt_triggered);
    connect(ui->actionFullscreen, &QAction::triggered, this, &MainWindow::actionFullscreen_triggered);

    ui->statusBar->addPermanentWidget(&statisticsLabel);
    ui->statusBar->addWidget(&connectionLabel);

    connect(&timer, &QTimer::timeout, this, &MainWindow::timer_timeout);
    timer.start(500);
}


MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::showEvent(QShowEvent *event)
{
    if (isInitialized || event->spontaneous())
        return;

    connectionWindow = new class ConnectionWindow(this);
    updateWindow     = new class UpdateWindow(this);
    consoleWindow    = new class ConsoleWindow(this);
    glWindow         = new class GLWindow(this);
    plotWindow       = new class PlotWindow(this);

    addDockWindow(Qt::LeftDockWidgetArea, connectionWindow);
    addDockWindow(Qt::LeftDockWidgetArea, updateWindow);

    tabifyDockWidget(
        addDockWindow(Qt::RightDockWidgetArea, glWindow),
        addDockWindow(Qt::RightDockWidgetArea, plotWindow)
    );

    addDockWindow(Qt::BottomDockWidgetArea, consoleWindow);

    QSettings settings;
    restoreGeometry(settings.value("windowGeometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());

    ui->actionFullscreen->setChecked(windowState() & Qt::WindowFullScreen);

    isInitialized = true;
}


void MainWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings;
    settings.setValue("windowGeometry", saveGeometry());
    settings.setValue("windowState", saveState());
    event->accept();
}


void MainWindow::timer_timeout()
{
    if (connection.isOpen())
        connectionLabel.setText(connection.getUrl().toString());
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




QDockWidget *MainWindow::addDockWindow(Qt::DockWidgetArea area, QMainWindow *window)
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

    return dw;
}


void MainWindow::actionFullscreen_triggered()
{
    if (ui->actionFullscreen->isChecked())
        this->showFullScreen();
    else
        this->showNormal();
}


void MainWindow::actionAbout_triggered()
{
    QMessageBox::about(this,
        "QuadControl",
        "Copyright (c)2015 Thomas Kindler <a href=\"mailto:mail_drquad@t-kindler.de\">&lt;mail_drquad@t-kindler.de&gt;</a>"
        "<p>"
        "This program is free software: you can redistribute it and/or modify "
        "it under the terms of the GNU General Public License as published by "
        "the Free Software Foundation, either version 3 of the License, or "
        "(at your option) any later version. "
        "<p>"
        "This program is distributed in the hope that it will be useful, "
        "but WITHOUT ANY WARRANTY; without even the implied warranty of "
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the "
        "GNU General Public License for more details. "
        "<p>"
        "You should have received a copy of the GNU General Public License "
        "along with this program.  If not, see "
        "<a href=\"http://www.gnu.org/licenses/\">http://www.gnu.org/licenses/</a>."
    );
}


void MainWindow::actionAboutQt_triggered()
{
    QMessageBox::aboutQt(this);
}

