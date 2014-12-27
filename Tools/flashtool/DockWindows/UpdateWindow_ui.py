# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'DockWindows/UpdateWindow.ui'
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

class Ui_UpdateWindow(object):
    def setupUi(self, UpdateWindow):
        UpdateWindow.setObjectName(_fromUtf8("UpdateWindow"))
        UpdateWindow.resize(377, 319)
        UpdateWindow.setAcceptDrops(True)
        self.centralwidget = QtGui.QWidget(UpdateWindow)
        self.centralwidget.setObjectName(_fromUtf8("centralwidget"))
        self.verticalLayout = QtGui.QVBoxLayout(self.centralwidget)
        self.verticalLayout.setObjectName(_fromUtf8("verticalLayout"))
        self.gridLayout = QtGui.QGridLayout()
        self.gridLayout.setObjectName(_fromUtf8("gridLayout"))
        self.lineEdit = QtGui.QLineEdit(self.centralwidget)
        self.lineEdit.setObjectName(_fromUtf8("lineEdit"))
        self.gridLayout.addWidget(self.lineEdit, 0, 0, 1, 1)
        self.browseButton = QtGui.QPushButton(self.centralwidget)
        self.browseButton.setObjectName(_fromUtf8("browseButton"))
        self.gridLayout.addWidget(self.browseButton, 0, 1, 1, 1)
        self.updateButton = QtGui.QPushButton(self.centralwidget)
        self.updateButton.setEnabled(False)
        self.updateButton.setObjectName(_fromUtf8("updateButton"))
        self.gridLayout.addWidget(self.updateButton, 3, 1, 1, 1)
        self.autoCheckBox = QtGui.QCheckBox(self.centralwidget)
        self.autoCheckBox.setChecked(True)
        self.autoCheckBox.setObjectName(_fromUtf8("autoCheckBox"))
        self.gridLayout.addWidget(self.autoCheckBox, 1, 0, 1, 2)
        spacerItem = QtGui.QSpacerItem(20, 40, QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Expanding)
        self.gridLayout.addItem(spacerItem, 2, 0, 1, 2)
        self.verticalLayout.addLayout(self.gridLayout)
        UpdateWindow.setCentralWidget(self.centralwidget)

        self.retranslateUi(UpdateWindow)
        QtCore.QMetaObject.connectSlotsByName(UpdateWindow)
        UpdateWindow.setTabOrder(self.lineEdit, self.browseButton)
        UpdateWindow.setTabOrder(self.browseButton, self.autoCheckBox)
        UpdateWindow.setTabOrder(self.autoCheckBox, self.updateButton)

    def retranslateUi(self, UpdateWindow):
        UpdateWindow.setWindowTitle(_translate("UpdateWindow", "Firmware Update", None))
        self.browseButton.setText(_translate("UpdateWindow", "&Browse...", None))
        self.updateButton.setText(_translate("UpdateWindow", "&Update", None))
        self.autoCheckBox.setText(_translate("UpdateWindow", "&Auto update on reset", None))


if __name__ == "__main__":
    import sys
    app = QtGui.QApplication(sys.argv)
    UpdateWindow = QtGui.QMainWindow()
    ui = Ui_UpdateWindow()
    ui.setupUi(UpdateWindow)
    UpdateWindow.show()
    sys.exit(app.exec_())

