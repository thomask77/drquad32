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

#include "MyGLWidget.h"
#include <QGLFormat>

#include "../GLTools.h"
#include "glut_teapot.h"

MyGLWidget::MyGLWidget(QWidget *parent)
    : QGLWidget(parent)
    , xRot(), yRot(), zRot()
    , ortho(), auto_rotate()
{
    connect(&timer, &QTimer::timeout, this, &MyGLWidget::timer_timeout);
    timer.start(10);

    setFormat(QGLFormat(QGL::SampleBuffers));
}

MyGLWidget::~MyGLWidget()
{
}

void MyGLWidget::initializeGL()
{
    // Set up the rendering context, load shaders and other resources, etc.
    //
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_MULTISAMPLE);

    glShadeModel(GL_SMOOTH);

    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    // Set up lights
    //
    float light_pos[]       = { -1, 1, 1, 0 };
    float light_ambient[]   = { 0.25, 0.25, 0.25, 1.0 };
    float light_diffuse[]   = { 0.75, 0.75, 0.75, 1.0 };
    float light_specular[]  = { 1.0,  1.0,  1.0,  1.0 };

    glLightfv(GL_LIGHT0, GL_POSITION, light_pos     );
    glLightfv(GL_LIGHT0, GL_AMBIENT,  light_ambient );
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_diffuse );
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);

    // Set up some fog
    //
    glFogi(GL_FOG_MODE, GL_LINEAR);
    glFogf(GL_FOG_START, 100);
    glFogf(GL_FOG_END, 256);

    float fog_color[] = { 0, 0, 0, 0 };

    glFogfv(GL_FOG_COLOR, fog_color);
    glEnable(GL_FOG);

    // Set up materials
    //
    float mat_specular[]  = { 1.0,  1.0,  1.0,  1.0 };

    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
    glMaterialf(GL_FRONT, GL_SHININESS, 16);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
}

void MyGLWidget::resizeGL(int w, int h)
{
    const float aspect = float(w) / h;
    m_projection.setToIdentity();

    if (ortho)
        m_projection.ortho(-64 * aspect, 64 * aspect, -64, 64, -1000, 1000);
    else
        m_projection.perspective(45, aspect, 10, 15000);

    glViewport(0, 0, w, h);
}

void MyGLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0, 0, 0, 0);

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(m_projection.constData());

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Apply mouse rotation
    //
    glTranslatef(0, 0, -128);
    glRotatef(xRot, 1, 0, 0);
    glRotatef(yRot, 0, 1, 0);
    glRotatef(zRot, 0, 0, 1);

    // Draw fixed ground reference systems
    //
    glTranslatef(-32, -32, -32);
    draw_big_coordinate_system(32);

    glTranslatef(32, 32, 32);
    draw_coordinate_system(48);

    // Switch to air-frame
    //
    glPushMatrix();
    glRotatef(90, 1, 0, 0);

    draw_big_coordinate_system(32);

    // Draw a flying teapot
    //
    glColor3ub(128, 128, 128);
    glDisable(GL_CULL_FACE);
    glutSolidTeapot(16);

    glEnable(GL_CULL_FACE);

    glPopMatrix();
}

void MyGLWidget::mousePressEvent(QMouseEvent *event)
{
    lastPos = event->pos();
}

void MyGLWidget::mouseMoveEvent(QMouseEvent *event)
{
    float dx = event->x() - lastPos.x();
    float dy = event->y() - lastPos.y();

    if (event->buttons() & Qt::LeftButton) {
        setXRotation(xRot + dy);
        setZRotation(zRot + dx);
    }
    if (event->buttons() & Qt::RightButton) {
        setXRotation(xRot + dy);
        setYRotation(yRot + dx);
    }

    lastPos = event->pos();
}

void MyGLWidget::setXRotation(float x)
{
    x = normalizeAngle(x);
    if (x != xRot) {
        xRot = x;
        xRotationChanged(x);
        update();
    }
}

void MyGLWidget::setYRotation(float y)
{
    y = normalizeAngle(y);
    if (y != yRot) {
        yRot = y;
        yRotationChanged(y);
        update();
    }
}

void MyGLWidget::setZRotation(float z)
{
    z = normalizeAngle(z);
    if (z != zRot) {
        zRot = z;
        zRotationChanged(z);
        update();
    }
}

void MyGLWidget::setOrtho(bool ortho)
{
    this->ortho = ortho;
    resizeGL(width(), height());
    update();
}

void MyGLWidget::setAutoRotate(bool rotate)
{
    auto_rotate = rotate;
}

void MyGLWidget::setRotation(float x, float y, float z)
{
    setXRotation(x);
    setYRotation(y);
    setZRotation(z);
}

void MyGLWidget::timer_timeout()
{
    auto t = QTime::currentTime();
    auto dt = t_last.msecsTo(t) * 1e-3;

    if (auto_rotate)
        setZRotation(zRot + 360 * dt / 60);

    t_last = t;
}
