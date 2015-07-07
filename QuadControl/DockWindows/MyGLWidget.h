#ifndef MYGLWIDGET_H
#define MYGLWIDGET_H

#include "GLTools.h"
#include <QOpenGLWidget>
#include <QOpenGLFunctions_2_1>
#include <QMatrix4x4>
#include <QTimer>
#include <QTime>
#include <QMouseEvent>


class MyGLWidget : public QOpenGLWidget,
        protected QOpenGLFunctions_2_1
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
    GLTools *glt;
    QTimer  timer;
    QPoint  lastPos;
    float   xRot = 0, yRot = 0, zRot = 0;
    bool    ortho = false, auto_rotate = false;

    QTime   t_last;

    QMatrix4x4  m_projection;

    virtual void initializeGL() override;
    virtual void resizeGL(int w, int h) override;
    virtual void paintGL() override;


    float normalizeAngle(float angle);
    void  draw_xy_plane(float size);
    void  draw_coordinate_system(float size);
};

#endif // MYGLWIDGET_H

