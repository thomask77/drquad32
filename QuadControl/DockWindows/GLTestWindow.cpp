#include "GLTestWindow.h"
#include "ui_GLTestWindow.h"

#include <QMatrix4x4>
#include <QOpenGLFunctions_2_1>

class GLTestWidget :
        public QOpenGLWidget,
        protected QOpenGLFunctions_2_1
{
public:
    GLTestWidget(QWidget *parent = 0);
    void initializeGL();
    void resizeGL(int w, int h);
    void paintGL();

private:
    QMatrix4x4 m_projection;
};


GLTestWidget::GLTestWidget(QWidget *parent) : QOpenGLWidget(parent)
{
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    setFormat(format);
}


void GLTestWidget::initializeGL()
{
    initializeOpenGLFunctions();
}


void GLTestWidget::resizeGL(int w, int h)
{
    m_projection.setToIdentity();
    m_projection.perspective(60.0f, w / float(h), 0.01f, 1000.0f);
}


void GLTestWidget::paintGL()
{
    glClearColor(1, 0, 1, .5);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glColor3f(0.1, 0.2, 0.3);
}


GLTestWindow::GLTestWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::GLTestWindow)
{
    ui->setupUi(this);
    ui->openGLWidget = new GLTestWidget(this);
}


GLTestWindow::~GLTestWindow()
{
    delete ui;
}
