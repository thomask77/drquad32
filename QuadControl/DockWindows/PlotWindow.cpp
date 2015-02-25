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
#include "qcustomplot.h"

#include <math.h>
#include "../MainWindow.h"


PlotWindow::PlotWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::PlotWindow)
{
    ui->setupUi(this);

    auto axisRect = ui->plot->axisRect();

    // acc
    //
    auto axis = axisRect->axis(QCPAxis::atLeft);
    axis->setLabel("Acceleration [m/s²]");
    axis->setRange(-10, 10);

    ui->plot->addGraph(ui->plot->xAxis, axis);
    ui->plot->graph()->setPen(QPen(Qt::red, 1, Qt::SolidLine));

    ui->plot->addGraph(ui->plot->xAxis, axis);
    ui->plot->graph()->setPen(QPen(Qt::green, 1, Qt::SolidLine));

    ui->plot->addGraph(ui->plot->xAxis, axis);
    ui->plot->graph()->setPen(QPen(Qt::blue, 1, Qt::SolidLine));

    // gyro
    //
    axis = axisRect->addAxis(QCPAxis::atLeft);
    axis->setLabel("Angular velocity [rad/s]");
    axis->setRange(-300, 300);

    ui->plot->addGraph(ui->plot->xAxis, axis);
    ui->plot->graph()->setPen(QPen(Qt::red, 1, Qt::DashLine));

    ui->plot->addGraph(ui->plot->xAxis, axis);
    ui->plot->graph()->setPen(QPen(Qt::green, 1, Qt::DashLine));

    ui->plot->addGraph(ui->plot->xAxis, axis);
    ui->plot->graph()->setPen(QPen(Qt::blue, 1, Qt::DashLine));

    // mag
    //
    axis = axisRect->addAxis(QCPAxis::atLeft);
    axis->setLabel("Magnetic field [µT]");
    axis->setRange(-5000, 5000);

    ui->plot->addGraph(ui->plot->xAxis, axis);
    ui->plot->graph()->setPen(QPen(Qt::red, 1, Qt::DotLine));

    ui->plot->addGraph(ui->plot->xAxis, axis);
    ui->plot->graph()->setPen(QPen(Qt::green, 1, Qt::DotLine));

    ui->plot->addGraph(ui->plot->xAxis, axis);
    ui->plot->graph()->setPen(QPen(Qt::blue, 1, Qt::DotLine));

    // baro
    //
    axis = axisRect->addAxis(QCPAxis::atLeft);
    axis->setLabel("Pressure [hPa]");
    axis->setRange(900, 1100);

    ui->plot->addGraph(ui->plot->xAxis, axis);
    ui->plot->graph()->setPen(QPen(Qt::yellow));

    axis = axisRect->addAxis(QCPAxis::atLeft);
    axis->setLabel("Temperature [°C]");
    axis->setRange(-40, 100);

    ui->plot->addGraph(ui->plot->xAxis, axis);
    ui->plot->graph()->setPen(QPen(Qt::darkYellow));

/*
    ui->plot->addGraph(); // blue dot
    ui->plot->graph(2)->setPen(QPen(Qt::blue));
    ui->plot->graph(2)->setLineStyle(QCPGraph::lsNone);
    ui->plot->graph(2)->setScatterStyle(QCPScatterStyle::ssDisc);

    ui->plot->addGraph(); // red dot
    ui->plot->graph(3)->setPen(QPen(Qt::red));
    ui->plot->graph(3)->setLineStyle(QCPGraph::lsNone);
    ui->plot->graph(3)->setScatterStyle(QCPScatterStyle::ssDisc);
    ui->plot->addGraph();

    ui->plot->xAxis->setTickLabelType(QCPAxis::ltDateTime);
    ui->plot->xAxis->setDateTimeFormat("hh:mm:ss");
    ui->plot->xAxis->setTickLabelType(QCPAxis::);
    ui->plot->xAxis->setAutoTickStep(false);
    ui->plot->xAxis->setTickStep(2);

    ui->plot->axisRect()->setupFullAxesBox();

    // make left and bottom axes transfer their ranges to right and top axes:
    // rangeChanged is overloaded, requiring some convoluted casts..
    //
    connect(
        ui->plot->xAxis,   (void (QCPAxis::*)(const QCPRange &)) &QCPAxis::rangeChanged,
        ui->plot->xAxis2,  (void (QCPAxis::*)(const QCPRange &)) &QCPAxis::setRange
    );

    connect(
        ui->plot->yAxis,   (void (QCPAxis::*)(const QCPRange &)) &QCPAxis::rangeChanged,
        ui->plot->yAxis2,  (void (QCPAxis::*)(const QCPRange &)) &QCPAxis::setRange
    );
*/

/*
    QCustomPlot::setNoAntialiasingOnDrag
*/

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
    /*
    double key = QDateTime::currentDateTime().toMSecsSinceEpoch()/1000.0;

    static double lastPointKey = 0;
    if (key-lastPointKey > 0.01) // at most add point every 10 ms
    {
        double value0 = qSin(key); //qSin(key*1.6+qCos(key*1.7)*2)*10 + qSin(key*1.2+0.56)*20 + 26;
        double value1 = qCos(key); //qSin(key*1.3+qCos(key*1.2)*1.2)*7 + qSin(key*0.9+0.26)*24 + 26;
        // add data to lines:
        ui->plot->graph(0)->addData(key, value0);
        ui->plot->graph(1)->addData(key, value1);
        // set data of dots:
        ui->plot->graph(2)->clearData();
        ui->plot->graph(2)->addData(key, value0);
        ui->plot->graph(3)->clearData();
        ui->plot->graph(3)->addData(key, value1);
        // remove data of lines that's outside visible range:
        ui->plot->graph(0)->removeDataBefore(key-8);
        ui->plot->graph(1)->removeDataBefore(key-8);
        // rescale value (vertical) axis to fit the current data:
        ui->plot->graph(0)->rescaleValueAxis();
        ui->plot->graph(1)->rescaleValueAxis(true);
        lastPointKey = key;
    }

    // make key axis range scroll with the data (at a constant range size of 8):
    //
    ui->plot->xAxis->setRange(key+0.25, 8, Qt::AlignRight);
    ui->plot->replot();
    */

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

    queue.clear();

    // make key axis range scroll with the data (at a constant range size of 1000):
    //
    ui->plot->xAxis->setRange(key, 1000, Qt::AlignRight);

/*    for (int i=0; i<ui->plot->graphCount(); i++)
        ui->plot->graph(i)->rescaleValueAxis(true);
*/
    ui->plot->replot();
}


void PlotWindow::connection_messageReceived(const msg_generic &msg)
{
    if (msg.h.id == MSG_ID_IMU_DATA)
        queue.append((const msg_imu_data&)msg);
}

