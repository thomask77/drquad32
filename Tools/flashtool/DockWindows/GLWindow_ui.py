# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'DockWindows/GLWindow.ui'
#
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    def _fromUtf8(s):
        return s

try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def _translate(context, text, disambig):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def _translate(context, text, disambig):
        return QtGui.QApplication.translate(context, text, disambig)

class Ui_GLWindow(object):
    def setupUi(self, GLWindow):
        GLWindow.setObjectName(_fromUtf8("GLWindow"))
        GLWindow.resize(390, 252)
        self.centralwidget = QtGui.QWidget(GLWindow)
        self.centralwidget.setObjectName(_fromUtf8("centralwidget"))
        self.horizontalLayout = QtGui.QHBoxLayout(self.centralwidget)
        self.horizontalLayout.setObjectName(_fromUtf8("horizontalLayout"))
        self.glWidget = QtGui.QWidget(self.centralwidget)
        self.glWidget.setObjectName(_fromUtf8("glWidget"))
        self.horizontalLayout.addWidget(self.glWidget)
        self.xSlider = QtGui.QSlider(self.centralwidget)
        self.xSlider.setMaximum(360)
        self.xSlider.setSingleStep(1)
        self.xSlider.setPageStep(15)
        self.xSlider.setOrientation(QtCore.Qt.Vertical)
        self.xSlider.setTickPosition(QtGui.QSlider.TicksBelow)
        self.xSlider.setTickInterval(15)
        self.xSlider.setObjectName(_fromUtf8("xSlider"))
        self.horizontalLayout.addWidget(self.xSlider)
        self.ySlider = QtGui.QSlider(self.centralwidget)
        self.ySlider.setMaximum(360)
        self.ySlider.setSingleStep(1)
        self.ySlider.setPageStep(15)
        self.ySlider.setOrientation(QtCore.Qt.Vertical)
        self.ySlider.setTickPosition(QtGui.QSlider.TicksBelow)
        self.ySlider.setTickInterval(15)
        self.ySlider.setObjectName(_fromUtf8("ySlider"))
        self.horizontalLayout.addWidget(self.ySlider)
        self.zSlider = QtGui.QSlider(self.centralwidget)
        self.zSlider.setMaximum(360)
        self.zSlider.setSingleStep(1)
        self.zSlider.setPageStep(15)
        self.zSlider.setOrientation(QtCore.Qt.Vertical)
        self.zSlider.setTickPosition(QtGui.QSlider.TicksBelow)
        self.zSlider.setTickInterval(15)
        self.zSlider.setObjectName(_fromUtf8("zSlider"))
        self.horizontalLayout.addWidget(self.zSlider)
        GLWindow.setCentralWidget(self.centralwidget)
        self.actionFront = QtGui.QAction(GLWindow)
        self.actionFront.setObjectName(_fromUtf8("actionFront"))
        self.actionRight = QtGui.QAction(GLWindow)
        self.actionRight.setObjectName(_fromUtf8("actionRight"))
        self.actionTop = QtGui.QAction(GLWindow)
        self.actionTop.setObjectName(_fromUtf8("actionTop"))
        self.actionAngled = QtGui.QAction(GLWindow)
        self.actionAngled.setObjectName(_fromUtf8("actionAngled"))
        self.actionOrthogonal = QtGui.QAction(GLWindow)
        self.actionOrthogonal.setCheckable(True)
        self.actionOrthogonal.setObjectName(_fromUtf8("actionOrthogonal"))
        self.actionAuto_rotate = QtGui.QAction(GLWindow)
        self.actionAuto_rotate.setCheckable(True)
        self.actionAuto_rotate.setObjectName(_fromUtf8("actionAuto_rotate"))

        self.retranslateUi(GLWindow)
        QtCore.QMetaObject.connectSlotsByName(GLWindow)

    def retranslateUi(self, GLWindow):
        GLWindow.setWindowTitle(_translate("GLWindow", "GL View", None))
        self.actionFront.setText(_translate("GLWindow", "Front (X+Z)", None))
        self.actionFront.setShortcut(_translate("GLWindow", "Ctrl+1", None))
        self.actionRight.setText(_translate("GLWindow", "Right (Y+Z)", None))
        self.actionRight.setShortcut(_translate("GLWindow", "Ctrl+2", None))
        self.actionTop.setText(_translate("GLWindow", "Top (X+Y)", None))
        self.actionTop.setShortcut(_translate("GLWindow", "Ctrl+3", None))
        self.actionAngled.setText(_translate("GLWindow", "Angled", None))
        self.actionAngled.setShortcut(_translate("GLWindow", "Ctrl+4", None))
        self.actionOrthogonal.setText(_translate("GLWindow", "Orthogonal", None))
        self.actionOrthogonal.setShortcut(_translate("GLWindow", "Ctrl+O", None))
        self.actionAuto_rotate.setText(_translate("GLWindow", "Auto rotate", None))
        self.actionAuto_rotate.setShortcut(_translate("GLWindow", "Ctrl+A", None))


if __name__ == "__main__":
    import sys
    app = QtGui.QApplication(sys.argv)
    GLWindow = QtGui.QMainWindow()
    ui = Ui_GLWindow()
    ui.setupUi(GLWindow)
    GLWindow.show()
    sys.exit(app.exec_())

