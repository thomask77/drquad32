# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'DockWindows/ConsoleWindow.ui'
#
#      by: PyQt4 UI code generator 4.11
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

class Ui_ConsoleWindow(object):
    def setupUi(self, ConsoleWindow):
        ConsoleWindow.setObjectName(_fromUtf8("ConsoleWindow"))
        ConsoleWindow.resize(510, 210)
        self.centralwidget = QtGui.QWidget(ConsoleWindow)
        self.centralwidget.setObjectName(_fromUtf8("centralwidget"))
        self.verticalLayout = QtGui.QVBoxLayout(self.centralwidget)
        self.verticalLayout.setSpacing(0)
        self.verticalLayout.setMargin(0)
        self.verticalLayout.setObjectName(_fromUtf8("verticalLayout"))
        self.plainTextEdit = QtGui.QPlainTextEdit(self.centralwidget)
        palette = QtGui.QPalette()
        brush = QtGui.QBrush(QtGui.QColor(192, 192, 192))
        brush.setStyle(QtCore.Qt.SolidPattern)
        palette.setBrush(QtGui.QPalette.Active, QtGui.QPalette.Text, brush)
        brush = QtGui.QBrush(QtGui.QColor(0, 0, 0))
        brush.setStyle(QtCore.Qt.SolidPattern)
        palette.setBrush(QtGui.QPalette.Active, QtGui.QPalette.Base, brush)
        brush = QtGui.QBrush(QtGui.QColor(192, 192, 192))
        brush.setStyle(QtCore.Qt.SolidPattern)
        palette.setBrush(QtGui.QPalette.Inactive, QtGui.QPalette.Text, brush)
        brush = QtGui.QBrush(QtGui.QColor(0, 0, 0))
        brush.setStyle(QtCore.Qt.SolidPattern)
        palette.setBrush(QtGui.QPalette.Inactive, QtGui.QPalette.Base, brush)
        brush = QtGui.QBrush(QtGui.QColor(120, 120, 120))
        brush.setStyle(QtCore.Qt.SolidPattern)
        palette.setBrush(QtGui.QPalette.Disabled, QtGui.QPalette.Text, brush)
        brush = QtGui.QBrush(QtGui.QColor(240, 240, 240))
        brush.setStyle(QtCore.Qt.SolidPattern)
        palette.setBrush(QtGui.QPalette.Disabled, QtGui.QPalette.Base, brush)
        self.plainTextEdit.setPalette(palette)
        font = QtGui.QFont()
        font.setFamily(_fromUtf8("Lucida Console"))
        font.setPointSize(9)
        self.plainTextEdit.setFont(font)
        self.plainTextEdit.setVerticalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOn)
        self.plainTextEdit.setLineWrapMode(QtGui.QPlainTextEdit.NoWrap)
        self.plainTextEdit.setTextInteractionFlags(QtCore.Qt.LinksAccessibleByKeyboard|QtCore.Qt.LinksAccessibleByMouse|QtCore.Qt.TextBrowserInteraction|QtCore.Qt.TextSelectableByKeyboard|QtCore.Qt.TextSelectableByMouse)
        self.plainTextEdit.setObjectName(_fromUtf8("plainTextEdit"))
        self.verticalLayout.addWidget(self.plainTextEdit)
        ConsoleWindow.setCentralWidget(self.centralwidget)
        self.toolBar = QtGui.QToolBar(ConsoleWindow)
        self.toolBar.setObjectName(_fromUtf8("toolBar"))
        ConsoleWindow.addToolBar(QtCore.Qt.TopToolBarArea, self.toolBar)
        self.action_save = QtGui.QAction(ConsoleWindow)
        icon = QtGui.QIcon()
        icon.addPixmap(QtGui.QPixmap(_fromUtf8("Images/save.png")), QtGui.QIcon.Normal, QtGui.QIcon.Off)
        self.action_save.setIcon(icon)
        self.action_save.setObjectName(_fromUtf8("action_save"))
        self.action_clear = QtGui.QAction(ConsoleWindow)
        icon1 = QtGui.QIcon()
        icon1.addPixmap(QtGui.QPixmap(_fromUtf8("Images/clear.png")), QtGui.QIcon.Normal, QtGui.QIcon.Off)
        self.action_clear.setIcon(icon1)
        self.action_clear.setObjectName(_fromUtf8("action_clear"))
        self.action_wrap = QtGui.QAction(ConsoleWindow)
        self.action_wrap.setCheckable(True)
        icon2 = QtGui.QIcon()
        icon2.addPixmap(QtGui.QPixmap(_fromUtf8("Images/wordwrap.png")), QtGui.QIcon.Normal, QtGui.QIcon.Off)
        self.action_wrap.setIcon(icon2)
        self.action_wrap.setObjectName(_fromUtf8("action_wrap"))
        self.action_pause = QtGui.QAction(ConsoleWindow)
        self.action_pause.setCheckable(True)
        icon3 = QtGui.QIcon()
        icon3.addPixmap(QtGui.QPixmap(_fromUtf8("Images/pause.png")), QtGui.QIcon.Normal, QtGui.QIcon.Off)
        self.action_pause.setIcon(icon3)
        self.action_pause.setObjectName(_fromUtf8("action_pause"))
        self.action_terminal = QtGui.QAction(ConsoleWindow)
        icon4 = QtGui.QIcon()
        icon4.addPixmap(QtGui.QPixmap(_fromUtf8("Images/application_xp_terminal.png")), QtGui.QIcon.Normal, QtGui.QIcon.Off)
        self.action_terminal.setIcon(icon4)
        self.action_terminal.setObjectName(_fromUtf8("action_terminal"))
        self.toolBar.addAction(self.action_save)
        self.toolBar.addAction(self.action_clear)
        self.toolBar.addAction(self.action_pause)
        self.toolBar.addAction(self.action_wrap)
        self.toolBar.addSeparator()
        self.toolBar.addAction(self.action_terminal)

        self.retranslateUi(ConsoleWindow)
        QtCore.QMetaObject.connectSlotsByName(ConsoleWindow)

    def retranslateUi(self, ConsoleWindow):
        ConsoleWindow.setWindowTitle(_translate("ConsoleWindow", "Console", None))
        self.toolBar.setWindowTitle(_translate("ConsoleWindow", "toolBar", None))
        self.action_save.setText(_translate("ConsoleWindow", "&Save", None))
        self.action_clear.setText(_translate("ConsoleWindow", "&Clear", None))
        self.action_wrap.setText(_translate("ConsoleWindow", "&Word Wrap", None))
        self.action_pause.setText(_translate("ConsoleWindow", "&Pause", None))
        self.action_pause.setToolTip(_translate("ConsoleWindow", "Pause", None))
        self.action_terminal.setText(_translate("ConsoleWindow", "&Terminal", None))
        self.action_terminal.setToolTip(_translate("ConsoleWindow", "Start Terminal Program", None))


if __name__ == "__main__":
    import sys
    app = QtGui.QApplication(sys.argv)
    ConsoleWindow = QtGui.QMainWindow()
    ui = Ui_ConsoleWindow()
    ui.setupUi(ConsoleWindow)
    ConsoleWindow.show()
    sys.exit(app.exec_())

