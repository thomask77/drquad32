from GLWindow_ui import *
from GLTools import *

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.QtOpenGL import *

from OpenGL.GLUT import *
from datetime import datetime


class GLWindow(QMainWindow, Ui_GLWindow):
    def __init__(self):
        QMainWindow.__init__(self)
        self.setupUi(self)

        # exchange placeholder
        #
        layout = self.centralWidget().layout()
        layout.removeWidget(self.glWidget)
        self.glWidget = GLWidget()
        layout.insertWidget(0, self.glWidget)
       
        self.xSlider.valueChanged.connect(self.glWidget.setXRotation)
        self.glWidget.xRotationChanged.connect(self.xSlider.setValue)
        self.ySlider.valueChanged.connect(self.glWidget.setYRotation)
        self.glWidget.yRotationChanged.connect(self.ySlider.setValue)
        self.zSlider.valueChanged.connect(self.glWidget.setZRotation)
        self.glWidget.zRotationChanged.connect(self.zSlider.setValue)

        self.glWidget.setContextMenuPolicy(Qt.ActionsContextMenu)
        self.glWidget.addAction(self.actionFront)
        self.glWidget.addAction(self.actionRight)
        self.glWidget.addAction(self.actionTop)
        self.glWidget.addAction(self.actionAngled)

        separator = QAction(self)
        separator.setSeparator(True)
        self.glWidget.addAction(separator)

        self.glWidget.addAction(self.actionOrthogonal)
        self.glWidget.addAction(self.actionAuto_rotate)

        self.actionOrthogonal.triggered.connect(self.glWidget.setOrtho)
        self.actionAuto_rotate.triggered.connect(self.glWidget.setAutoRotate)

        self.actionFront.triggered.connect (lambda: self.glWidget.setRotation(270,   0,    0))
        self.actionRight.triggered.connect (lambda: self.glWidget.setRotation(270,   0,  270))
        self.actionTop.triggered.connect   (lambda: self.glWidget.setRotation(  0,   0,    0))
        self.actionAngled.triggered.connect(lambda: self.glWidget.setRotation(300,   0,  330))

        self.xSlider.setValue(300)
        self.ySlider.setValue(0)
        self.zSlider.setValue(330)


class GLWidget(QGLWidget):
    xRotationChanged = pyqtSignal(int)
    yRotationChanged = pyqtSignal(int)
    zRotationChanged = pyqtSignal(int)

    def __init__(self):
        QGLWidget.__init__(self)
        self.setFormat(QGLFormat(QGL.SampleBuffers))

        self.xRot = 0
        self.yRot = 0
        self.zRot = 0
        self.lastPos = QPoint()
        self.ortho = False
        self.auto_rotate = False

        self.timer = QTimer()
        self.timer.timeout.connect(self.timer_timeout)
        self.timer.start(16)
        self.t_last = datetime.now()
        
    def timer_timeout(self):
        t = datetime.now()
        dt = (t - self.t_last).total_seconds()        

        if self.auto_rotate:
            self.zRot -= 360 * dt / 60
            self.updateGL()

        self.t_last = t
        
    def minimumSizeHint(self):
        return QSize(50, 50)

    def sizeHint(self):
        return QSize(400, 400)

    def setRotation(self, x, y, z):
        self.setXRotation(x)
        self.setYRotation(y)
        self.setZRotation(z)

    def setXRotation(self, angle):
        angle = self.normalizeAngle(angle)
        if angle != self.xRot:
            self.xRot = angle
            self.xRotationChanged.emit(angle)
            self.updateGL()

    def setYRotation(self, angle):
        angle = self.normalizeAngle(angle)
        if angle != self.yRot:
            self.yRot = angle
            self.yRotationChanged.emit(angle)
            self.updateGL()

    def setZRotation(self, angle):
        angle = self.normalizeAngle(angle)
        if angle != self.zRot:
            self.zRot = angle
            self.zRotationChanged.emit(angle)
            self.updateGL()

    def setOrtho(self, ortho):
        self.ortho = ortho
        self.resizeGL(self.width(), self.height())
        self.updateGL()

    def setAutoRotate(self, auto):
        self.auto_rotate = auto

    def initializeGL(self):
        glutInit()

        glEnable(GL_DEPTH_TEST)
        glEnable(GL_CULL_FACE)
        glEnable(GL_MULTISAMPLE)

        glShadeModel(GL_SMOOTH)

        glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST)

        # Set up lights
        #
        glLightfv(GL_LIGHT0, GL_POSITION, (1, 1, 1, 0))
        glLightfv(GL_LIGHT0, GL_AMBIENT, (0.25, 0.25, 0.25, 1.0))
        glLightfv(GL_LIGHT0, GL_DIFFUSE, (0.75, 0.75, 0.75, 1.0))
        glLightfv(GL_LIGHT0, GL_SPECULAR, (1.0, 1.0, 1.0, 1.0))
        glEnable(GL_LIGHT0)
        glEnable(GL_LIGHTING)

        # Set up some fog
        #
        glFogi(GL_FOG_MODE, GL_LINEAR)
        glFogf(GL_FOG_START, 100)
        glFogf(GL_FOG_END, 256)
        glFogfv(GL_FOG_COLOR, (0, 0, 0, 0))
        glEnable(GL_FOG)

        # Set up materials
        #
        glMaterialfv(GL_FRONT, GL_SPECULAR, (1.0, 1.0, 1.0, 1.0))
        glMaterialf(GL_FRONT, GL_SHININESS, 16)
        glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE)
        glEnable(GL_COLOR_MATERIAL)

    def resizeGL(self, width, height):
        aspect = float(width) / height

        glViewport(0, 0, width, height)

        glMatrixMode(GL_PROJECTION)
        glLoadIdentity()

        if self.ortho:
            glOrtho(-64 * aspect, 64 * aspect, -64, 64, -1000, 1000)
        else:
            gluPerspective(45, aspect, 10, 15000)

        glMatrixMode(GL_MODELVIEW)

    def paintGL(self):
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
        glLoadIdentity()

        # Apply mouse rotation
        #
        glTranslatef(0, 0, -128)
        glRotatef(self.xRot, 1, 0, 0)
        glRotatef(self.yRot, 0, 1, 0)
        glRotatef(self.zRot, 0, 0, 1)

        # Draw fixed ground reference systems
        #
        glTranslatef(-32, -32, -32)
        draw_big_coordinate_system(32)

        glTranslatef(32, 32, 32)
        draw_coordinate_system(48)

        # Switch to air-frame
        #
        glPushMatrix()
        glRotatef(90, 1, 0, 0)

        draw_big_coordinate_system(32)

        # Draw a flying teapot
        #
        glColor3ub(128, 128, 128)
        glDisable(GL_CULL_FACE)
        glutSolidTeapot(16)

        glEnable(GL_CULL_FACE)

        glPopMatrix()

        # glCallList(self.object)

    def mousePressEvent(self, event):
        self.lastPos = event.pos()

    def mouseMoveEvent(self, event):
        dx = event.x() - self.lastPos.x()
        dy = event.y() - self.lastPos.y()

        if event.buttons() & Qt.LeftButton:
            self.setXRotation(self.xRot + dy)
            self.setYRotation(self.yRot + dx)
        elif event.buttons() & Qt.RightButton:
            self.setXRotation(self.xRot + dy)
            self.setZRotation(self.zRot + dx)

        self.lastPos = event.pos()

    def normalizeAngle(self, angle):
        while angle < 0:
            angle += 360
        while angle >= 360:
            angle -= 360
        return angle
