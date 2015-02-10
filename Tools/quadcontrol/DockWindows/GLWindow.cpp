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

#include "GLWindow.h"
#include "ui_GLWindow.h"

#include <QSerialPort>
#include <QSerialPortInfo>
#include <QAction>

#include "MainWindow.h"

GLWindow::GLWindow(MainWindow *parent)
    : QMainWindow(parent)
    , ui(new Ui::GLWindow)
    , mainWindow(parent)
{
    ui->setupUi(this);

    // Build context menu
    //
    ui->glWidget->setContextMenuPolicy(Qt::ActionsContextMenu);
    ui->glWidget->addAction(ui->actionFront);
    ui->glWidget->addAction(ui->actionRight);
    ui->glWidget->addAction(ui->actionTop);
    ui->glWidget->addAction(ui->actionAngled);

    auto separator = new QAction(this);
    separator->setSeparator(true);
    ui->glWidget->addAction(separator);

    ui->glWidget->addAction(ui->actionOrthogonal);
    ui->glWidget->addAction(ui->actionAuto_rotate);

    // Connect actions
    //
    connect(ui->xSlider, &QSlider::valueChanged, ui->glWidget, &MyGLWidget::setXRotation);
    connect(ui->ySlider, &QSlider::valueChanged, ui->glWidget, &MyGLWidget::setYRotation);
    connect(ui->zSlider, &QSlider::valueChanged, ui->glWidget, &MyGLWidget::setZRotation);

    connect(ui->glWidget, &MyGLWidget::xRotationChanged, ui->xSlider, &QSlider::setValue);
    connect(ui->glWidget, &MyGLWidget::yRotationChanged, ui->ySlider, &QSlider::setValue);
    connect(ui->glWidget, &MyGLWidget::zRotationChanged, ui->zSlider, &QSlider::setValue);

    connect(ui->actionOrthogonal,   &QAction::triggered, ui->glWidget, &MyGLWidget::setOrtho);
    connect(ui->actionAuto_rotate,  &QAction::triggered, ui->glWidget, &MyGLWidget::setAutoRotate);

    // TODO: Dangerous! No automatic disconnect when receiver is destroyed!
    //
    connect(ui->actionFront,  &QAction::triggered, [=]() { ui->glWidget->setRotation(270,   0,    0); } );
    connect(ui->actionRight,  &QAction::triggered, [=]() { ui->glWidget->setRotation(270,   0,  270); } );
    connect(ui->actionTop,    &QAction::triggered, [=]() { ui->glWidget->setRotation(  0,   0,    0); } );
    connect(ui->actionAngled, &QAction::triggered, [=]() { ui->glWidget->setRotation(300,   0,  330); } );

    ui->actionAngled->trigger();
}

GLWindow::~GLWindow()
{
    delete ui;
}
