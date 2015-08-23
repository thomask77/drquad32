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

#include "PlotWindow.h"
#include "ui_PlotWindow.h"
#include "TangoColors.h"
#include "MainWindow.h"
#include <math.h>


PlotWindow::PlotWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::PlotWindow)
{
    ui->setupUi(this);
    auto axisRect = ui->plot->axisRect();

    axisRect->setupFullAxesBox();
    axisRect->setRangeDrag(0);
    axisRect->setRangeZoom(0);

    ui->plot->setInteractions(
        QCP::iRangeDrag |
        QCP::iRangeZoom |
        QCP::iSelectAxes
    );

    ui->plot->setNoAntialiasingOnDrag(true);

    // x-axis
    //
    ui->plot->xAxis->setLabel("Samples");
/*
    ui->plot->xAxis->setTickLabelType(QCPAxis::ltDateTime);
    ui->plot->xAxis->setDateTimeFormat("hh:mm:ss");
    ui->plot->xAxis->setTickLabelType(QCPAxis::);
    ui->plot->xAxis->setAutoTickStep(false);
    ui->plot->xAxis->setTickStep(2);
*/

    // Accelerometer
    //
    auto axis = axisRect->axis(QCPAxis::atLeft);
    auto color = TangoColors::Aluminium4;
    axis->setBasePen(QPen(color));
    axis->setTickPen(QPen(color));
    axis->setSubTickPen(QPen(color));
    axis->setLabel("m/s²");
    axis->setLabelPadding(-16);
    axis->setTickLabelRotation(-90);
    axis->setRange(-10, 10);
    axis->setSelectableParts(QCPAxis::spAxis);

    ui->plot->addGraph(ui->plot->xAxis, axis);
    ui->plot->graph()->setPen(QPen(color, 1, Qt::SolidLine));

    ui->plot->addGraph(ui->plot->xAxis, axis);
    ui->plot->graph()->setPen(QPen(color, 1, Qt::DashLine));

    ui->plot->addGraph(ui->plot->xAxis, axis);
    ui->plot->graph()->setPen(QPen(color, 1, Qt::DotLine));

    // Gyroscope
    //
    axis = axisRect->addAxis(QCPAxis::atLeft);
    color = TangoColors::ScarletRed1;
    axis->setBasePen(QPen(color));
    axis->setTickPen(QPen(color));
    axis->setSubTickPen(QPen(color));
    axis->setLabel("rad/s");
    axis->setLabelPadding(-16);

    axis->setTickLabelRotation(-90);
    axis->setRange(-300, 300);
    axis->setSelectableParts(QCPAxis::spAxis);


    ui->plot->addGraph(ui->plot->xAxis, axis);
    ui->plot->graph()->setPen(QPen(color, 1, Qt::SolidLine));

    ui->plot->addGraph(ui->plot->xAxis, axis);
    ui->plot->graph()->setPen(QPen(color, 1, Qt::DashLine));

    ui->plot->addGraph(ui->plot->xAxis, axis);
    ui->plot->graph()->setPen(QPen(color, 1, Qt::DotLine));

    // Magnetic sensor
    //
    axis = axisRect->addAxis(QCPAxis::atLeft);
    color = TangoColors::Chameleon1;
    axis->setBasePen(QPen(color));
    axis->setTickPen(QPen(color));
    axis->setSubTickPen(QPen(color));
    axis->setLabel("µT");
    axis->setLabelPadding(-16);
    axis->setTickLabelRotation(-90);
    axis->setRange(-5000, 5000);
    axis->setSelectableParts(QCPAxis::spAxis);

    ui->plot->addGraph(ui->plot->xAxis, axis);
    ui->plot->graph()->setPen(QPen(color, 1, Qt::SolidLine));

    ui->plot->addGraph(ui->plot->xAxis, axis);
    ui->plot->graph()->setPen(QPen(color, 1, Qt::DashLine));

    ui->plot->addGraph(ui->plot->xAxis, axis);
    ui->plot->graph()->setPen(QPen(color, 1, Qt::DotLine));

    // Barometric sensor
    //
    axis = axisRect->addAxis(QCPAxis::atLeft);
    color = TangoColors::SkyBlue1;
    axis->setBasePen(QPen(color));
    axis->setTickPen(QPen(color));
    axis->setSubTickPen(QPen(color));
    axis->setLabel("hPa");
    axis->setLabelPadding(-16);
    axis->setTickLabelRotation(-90);
    axis->setRange(900, 1100);
    axis->setSelectableParts(QCPAxis::spAxis);

    ui->plot->addGraph(ui->plot->xAxis, axis);
    ui->plot->graph()->setPen(QPen(color, 1, Qt::SolidLine));

    // Temperature
    //
    axis = axisRect->addAxis(QCPAxis::atLeft);
    color = TangoColors::Orange1;
    axis->setBasePen(QPen(color));
    axis->setTickPen(QPen(color));
    axis->setSubTickPen(QPen(color));
    axis->setLabel("°C");
    axis->setLabelPadding(-16);
    axis->setTickLabelRotation(-90);
    axis->setRange(-40, 100);
    axis->setSelectableParts(QCPAxis::spAxis);

    ui->plot->addGraph(ui->plot->xAxis, axis);
    ui->plot->graph()->setPen(QPen(color, 1, Qt::SolidLine));

    connect(
        ui->plot, &QCustomPlot::axisClick,
        this,     &PlotWindow::axisClick
    );

    connect(&mainWindow->connection, &Connection::messageReceived, this, &PlotWindow::connection_messageReceived);
    connect(&timer, &QTimer::timeout, this, &PlotWindow::timer_timeout);
    timer.start(30);
}


PlotWindow::~PlotWindow()
{
    delete ui;
}


void PlotWindow::timer_timeout()
{
    static double key;

    for (const auto &imu: queue) {
        ui->plot->graph( 0)->addData(key, imu.acc_x);
        ui->plot->graph( 1)->addData(key, imu.acc_y);
        ui->plot->graph( 2)->addData(key, imu.acc_z);
        ui->plot->graph( 3)->addData(key, imu.gyro_x);
        ui->plot->graph( 4)->addData(key, imu.gyro_y);
        ui->plot->graph( 5)->addData(key, imu.gyro_z);
        ui->plot->graph( 6)->addData(key, imu.mag_x);
        ui->plot->graph( 7)->addData(key, imu.mag_y);
        ui->plot->graph( 8)->addData(key, imu.mag_z);
        ui->plot->graph( 9)->addData(key, imu.baro_hpa);
        ui->plot->graph(10)->addData(key, imu.baro_temp);
        key += 1;
    }

    if (queue.count()) {
//        if (!ui->plot->axisRect()->dragging())
//            ui->plot->xAxis->setRange(key + 10, 1000, Qt::AlignRight);

        ui->plot->replot();
        queue.clear();
    }
}


void PlotWindow::connection_messageReceived(const msg_generic &msg)
{
//    if (msg.h.id == MSG_ID_IMU_DATA)
//        queue.append((const msg_imu_data&)msg);
}


void PlotWindow::axisClick(QCPAxis *axis)
{
    auto axisRect = ui->plot->axisRect();

    if (axis == ui->plot->xAxis) {
        axisRect->setRangeDrag(Qt::Horizontal);
        axisRect->setRangeZoom(Qt::Horizontal);
        axisRect->setRangeDragAxes(ui->plot->xAxis, NULL);
        axisRect->setRangeZoomAxes(ui->plot->xAxis, NULL);
    }
    else {
        axisRect->setRangeDrag(Qt::Vertical);
        axisRect->setRangeZoom(Qt::Vertical);
        axisRect->setRangeDragAxes(NULL, axis);
        axisRect->setRangeZoomAxes(NULL, axis);
    }

    for (auto a: axisRect->axes(QCPAxis::atLeft))
        a->grid()->setVisible(a == axis);
}

