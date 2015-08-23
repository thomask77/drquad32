#include "InputMappingWindow.h"
#include "ui_InputMappingWindow.h"

InputMappingWindow::InputMappingWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::InputMappingWindow)
{
    ui->setupUi(this);

    connect(ui->min,      SIGNAL(valueChanged(int)), this, SLOT(valueChanged()));
    connect(ui->max,      SIGNAL(valueChanged(int)), this, SLOT(valueChanged()));
    connect(ui->deadband, SIGNAL(valueChanged(int)), this, SLOT(valueChanged()));
    connect(ui->expo,     SIGNAL(valueChanged(int)), this, SLOT(valueChanged()));

    ui->plot->xAxis->setLabel("Pulse Width [µs]");
    ui->plot->xAxis->setAutoTickStep(false);
    ui->plot->xAxis->setAutoSubTicks(false);
    ui->plot->xAxis->setTickStep(500);
    ui->plot->xAxis->setSubTickCount(10);
    ui->plot->xAxis->setRange(900, 2100);

    ui->plot->yAxis->setLabel("Output");
    ui->plot->yAxis->setAutoTickStep(false);
    ui->plot->yAxis->setAutoSubTicks(false);
    ui->plot->yAxis->setTickStep(1);
    ui->plot->yAxis->setSubTickCount(10);
    ui->plot->yAxis->setRange(-1.1, 1.1);

    ui->plot->addGraph();
}


InputMappingWindow::~InputMappingWindow()
{
    delete ui;
}


void InputMappingWindow::valueChanged()
{
    QVector<double> x(101), y(101);

    for (int i=0; i<101; ++i) {
      x[i] = i/50.0 - 1;
      y[i] = x[i]*x[i];
    }

    ui->plot->graph(0)->setData(x, y);
    ui->plot->replot();
}
