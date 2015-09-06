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
#include "TangoColors.h"
#include "MainWindow.h"
#include <QtMath>


MyGLWidget::MyGLWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{
    QSurfaceFormat format;
    format.setSamples(4);
    setFormat(format);

    connect(&timer, &QTimer::timeout, this, &MyGLWidget::timer_timeout);
    connect(&mainWindow->connection, &Connection::messageReceived, this, &MyGLWidget::connection_messageReceived);

    m_dcm.setToIdentity();

    timer.start(10);
}


MyGLWidget::~MyGLWidget()
{
    delete glt;
}


void MyGLWidget::connection_messageReceived(const msg_generic &msg)
{
    if (msg.h.id == MSG_ID_DCM_MATRIX) {
        auto m = (const msg_dcm_matrix&)msg;
        m_dcm = QMatrix4x4(
            m.m00, m.m01, m.m02, 0,
            m.m10, m.m11, m.m12, 0,
            m.m20, m.m21, m.m22, 0,
            0,     0,     0,     1
        );

    }

    if (msg.h.id == MSG_ID_DCM_REFERENCE)
        dcm_reference_queue.append( (const msg_dcm_reference&)msg );
}


void MyGLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glt = new GLTools();

    // Set up the rendering context, load shaders and other resources, etc.
    //
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

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

    if (orthoProjection)
        m_projection.ortho(-64 * aspect, 64 * aspect, -64, 64, -1000, 1000);
    else
        m_projection.perspective(45, aspect, 10, 15000);

    glViewport(0, 0, w, h);
}


void MyGLWidget::drawPointCloud(const float *points, int numPoints, const QColor &color)
{
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    glEnable(GL_NORMALIZE);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);

    glVertexPointer(3, GL_FLOAT, 0, points);
    glNormalPointer(GL_FLOAT, 0, points);

    int recent = std::min(numPoints, 1);

    if (numPoints > recent) {
        glDepthMask(GL_FALSE);
        glEnable(GL_LIGHTING);
        glPointSize(2);
        glColor4ub(255, 255, 255, 128);
        glDrawArrays(GL_POINTS, 0, numPoints-recent);
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_LIGHTING);
    glPointSize(5);
    glColor3ub(color.red(), color.green(), color.blue());
    glDrawArrays(GL_POINTS, numPoints-recent, recent);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);

    glPopAttrib();
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

    glPushMatrix();
      glTranslatef(-32, -32, -32);
      glt->drawBigCoordinateSystem(32);
    glPopMatrix();

    // Switch to North, East, Down air-frame
    // https://pixhawk.org/dev/know-how/frames_of_reference
    //
    glRotatef(90,  0, 0, 1);
    glRotatef(180, 1, 0, 0);

    glt->drawCoordinateSystem(40);

    // Draw down reference
    //
    if (enableDrawPointCloud) {
        const int N = std::min(dcm_reference_queue.length(), 1000);

        if (N > 0) {
            float down[N][3], north[N][3];

            for (int i=0; i<N; i++) {
                const auto &d = dcm_reference_queue[i + dcm_reference_queue.length() - N];
                down[i][0]  = d.down_x;     down[i][1]  = d.down_y;     down[i][2]  = d.down_z;
                north[i][0] = d.north_x;    north[i][1] = d.north_y;    north[i][2] = d.north_z;
            }

            drawPointCloud(down[0], N, TangoColors::Chameleon1);
            drawPointCloud(north[0], N, TangoColors::ScarletRed1);
        }
    }

    // Apply the DCM matrix
    //
    glMultMatrixf(m_dcm.constData());

    glt->drawBigCoordinateSystem(32);
    glt->drawXyPlane(32);


    /*
    // Draw a flying teapot
    //
    {
        glPushMatrix();
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glDisable(GL_CULL_FACE);

        glRotatef(-90,  1, 0, 0);
        glColor3ub(128, 128, 128);
        glt->drawSolidTeapot(16);

        glPopAttrib();
        glPopMatrix();
    }
    */


    /*
    // Draw accelerometer values
    //
    const int N = std::min(queue.length(), 1000);

    if (N > 0) {
        float pts[N][3];

        for (int i=0; i<N; i++) {
            const auto &imu = queue[i + queue.length()-N];
            pts[i][0] = imu.acc_x;
            pts[i][1] = imu.acc_y;
            pts[i][2] = imu.acc_z;
        }

        drawPointCloud(pts[0], N, TangoColors::Chameleon1);
    }
    */
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
    x = glt->normalizeAngle(x);
    if (x != xRot) {
        xRot = x;
        xRotationChanged(x);
        update();
    }
}


void MyGLWidget::setYRotation(float y)
{
    y = glt->normalizeAngle(y);
    if (y != yRot) {
        yRot = y;
        yRotationChanged(y);
        update();
    }
}


void MyGLWidget::setZRotation(float z)
{
    z = glt->normalizeAngle(z);
    if (z != zRot) {
        zRot = z;
        zRotationChanged(z);
        update();
    }
}


void MyGLWidget::setOrthoProjection(bool ortho)
{
    this->orthoProjection = ortho;
    resizeGL(width(), height());
    update();
}


void MyGLWidget::setEnableAutoRotate(bool rotate)
{
    enableAutoRotate = rotate;
}


void MyGLWidget::setEnableDrawPointCloud(bool draw)
{
    enableDrawPointCloud = draw;
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

    if (enableAutoRotate)
        setZRotation(zRot + 360 * dt / 60);

    update();

    t_last = t;
}

