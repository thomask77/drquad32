# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'DockWindows/ConnectionWindow.ui'
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

class Ui_ConnectionWindow(object):
    def setupUi(self, ConnectionWindow):
        ConnectionWindow.setObjectName(_fromUtf8("ConnectionWindow"))
        ConnectionWindow.resize(519, 461)
        self.centralwidget = QtGui.QWidget(ConnectionWindow)
        self.centralwidget.setObjectName(_fromUtf8("centralwidget"))
        self.verticalLayout = QtGui.QVBoxLayout(self.centralwidget)
        self.verticalLayout.setMargin(0)
        self.verticalLayout.setObjectName(_fromUtf8("verticalLayout"))
        self.treeWidget = QtGui.QTreeWidget(self.centralwidget)
        self.treeWidget.setAlternatingRowColors(True)
        self.treeWidget.setTextElideMode(QtCore.Qt.ElideMiddle)
        self.treeWidget.setObjectName(_fromUtf8("treeWidget"))
        self.treeWidget.header().setMinimumSectionSize(0)
        self.treeWidget.header().setSortIndicatorShown(True)
        self.verticalLayout.addWidget(self.treeWidget)
        self.lineEdit = QtGui.QLineEdit(self.centralwidget)
        self.lineEdit.setObjectName(_fromUtf8("lineEdit"))
        self.verticalLayout.addWidget(self.lineEdit)
        ConnectionWindow.setCentralWidget(self.centralwidget)
        self.toolBar = QtGui.QToolBar(ConnectionWindow)
        self.toolBar.setEnabled(True)
        self.toolBar.setObjectName(_fromUtf8("toolBar"))
        ConnectionWindow.addToolBar(QtCore.Qt.TopToolBarArea, self.toolBar)
        self.action_scan = QtGui.QAction(ConnectionWindow)
        icon = QtGui.QIcon()
        icon.addPixmap(QtGui.QPixmap(_fromUtf8("Images/arrow_refresh.png")), QtGui.QIcon.Normal, QtGui.QIcon.Off)
        self.action_scan.setIcon(icon)
        self.action_scan.setObjectName(_fromUtf8("action_scan"))
        self.action_connect = QtGui.QAction(ConnectionWindow)
        self.action_connect.setEnabled(False)
        icon1 = QtGui.QIcon()
        icon1.addPixmap(QtGui.QPixmap(_fromUtf8("Images/connect.png")), QtGui.QIcon.Normal, QtGui.QIcon.Off)
        self.action_connect.setIcon(icon1)
        self.action_connect.setObjectName(_fromUtf8("action_connect"))
        self.action_disconnect = QtGui.QAction(ConnectionWindow)
        self.action_disconnect.setEnabled(False)
        icon2 = QtGui.QIcon()
        icon2.addPixmap(QtGui.QPixmap(_fromUtf8("Images/disconnect.png")), QtGui.QIcon.Normal, QtGui.QIcon.Off)
        self.action_disconnect.setIcon(icon2)
        self.action_disconnect.setObjectName(_fromUtf8("action_disconnect"))
        self.action_add = QtGui.QAction(ConnectionWindow)
        self.action_add.setObjectName(_fromUtf8("action_add"))
        self.action_remove = QtGui.QAction(ConnectionWindow)
        self.action_remove.setObjectName(_fromUtf8("action_remove"))
        self.toolBar.addAction(self.action_scan)
        self.toolBar.addSeparator()
        self.toolBar.addAction(self.action_connect)
        self.toolBar.addAction(self.action_disconnect)
        self.toolBar.addSeparator()
        self.toolBar.addAction(self.action_add)
        self.toolBar.addAction(self.action_remove)

        self.retranslateUi(ConnectionWindow)
        QtCore.QMetaObject.connectSlotsByName(ConnectionWindow)

    def retranslateUi(self, ConnectionWindow):
        ConnectionWindow.setWindowTitle(_translate("ConnectionWindow", "Connection", None))
        self.treeWidget.setSortingEnabled(True)
        self.treeWidget.headerItem().setText(0, _translate("ConnectionWindow", "Port", None))
        self.treeWidget.headerItem().setText(1, _translate("ConnectionWindow", "Hardware ID", None))
        self.treeWidget.headerItem().setText(2, _translate("ConnectionWindow", "Description", None))
        self.toolBar.setWindowTitle(_translate("ConnectionWindow", "toolBar", None))
        self.action_scan.setText(_translate("ConnectionWindow", "&Scan", None))
        self.action_scan.setToolTip(_translate("ConnectionWindow", "Scan available ports", None))
        self.action_connect.setText(_translate("ConnectionWindow", "&Connect", None))
        self.action_connect.setToolTip(_translate("ConnectionWindow", "Connect", None))
        self.action_disconnect.setText(_translate("ConnectionWindow", "&Disconnect", None))
        self.action_add.setText(_translate("ConnectionWindow", "&Add", None))
        self.action_add.setToolTip(_translate("ConnectionWindow", "Add Connection", None))
        self.action_remove.setText(_translate("ConnectionWindow", "&Remove", None))
        self.action_remove.setToolTip(_translate("ConnectionWindow", "Remove Connection", None))


if __name__ == "__main__":
    import sys
    app = QtGui.QApplication(sys.argv)
    ConnectionWindow = QtGui.QMainWindow()
    ui = Ui_ConnectionWindow()
    ui.setupUi(ConnectionWindow)
    ConnectionWindow.show()
    sys.exit(app.exec_())

