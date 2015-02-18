#ifndef MYGLWIDGET_H
#define MYGLWIDGET_H

#include <QGLWidget>
#include <QMatrix4x4>
#include <QTimer>
#include <QTime>
#include <QMouseEvent>

class MyGLWidget : public QGLWidget
{
    Q_OBJECT

public:
    explicit MyGLWidget(QWidget *parent = 0);
    ~MyGLWidget();

    // TODO: Replace by QProperty?!

    void setXRotation(float x);
    void setYRotation(float y);
    void setZRotation(float z);

    void setRotation(float x, float y, float z);

    void setOrtho(bool ortho);
    void setAutoRotate(bool rotate);

    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

    void timer_timeout();

signals:
    void xRotationChanged(float x);
    void yRotationChanged(float y);
    void zRotationChanged(float z);

private:
    QTimer  timer;
    QPoint  lastPos;
    float   xRot, yRot, zRot;
    bool    ortho, auto_rotate;

    QTime   t_last;

    QMatrix4x4  m_projection;

    virtual void initializeGL() override;
    virtual void resizeGL(int w, int h) override;
    virtual void paintGL() override;
};

#endif // MYGLWIDGET_H

