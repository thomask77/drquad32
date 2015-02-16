#include "GLWindow.h"
#include "ui_GLWindow.h"

#include <QSerialPort>
#include <QSerialPortInfo>
#include <QAction>

#include "MainWindow.h"

GLWindow::GLWindow(MainWindow *parent) :
    QMainWindow(parent),
    ui(new Ui::GLWindow),
    mainWindow(parent)
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
    connect(ui->actionFront,        &QAction::triggered, [=]() { ui->glWidget->setRotation(270,   0,    0); } );
    connect(ui->actionRight,        &QAction::triggered, [=]() { ui->glWidget->setRotation(270,   0,  270); } );
    connect(ui->actionTop,          &QAction::triggered, [=]() { ui->glWidget->setRotation(  0,   0,    0); } );
    connect(ui->actionAngled,       &QAction::triggered, [=]() { ui->glWidget->setRotation(300,   0,  330); } );

    ui->actionAngled->trigger();
}

GLWindow::~GLWindow()
{
    delete ui;
}
