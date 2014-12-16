from Connection import connection
from Dispatcher import dispatcher

from copy import copy
from datetime import datetime

from PyQt4 import uic
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from DockWindows import *

class MainWindow(QMainWindow):
    # TODO:
    # * Layout Load/Save with recently used list
    # * Lock Layout
    #   * https://github.com/tegesoft/qt-manhattan-style ?
    #

    def __init__(self):
        QMainWindow.__init__(self)
        uic.loadUi("MainWindow.ui", self)

        self.setDockNestingEnabled(True)

        self.statisticsLabel = QLabel()
        self.connectionLabel = QLabel()
        self.statusBar.addPermanentWidget(self.statisticsLabel)
        self.statusBar.addWidget(self.connectionLabel)

        self.statusBar.setStyleSheet("QStatusBar::item { border: none; }")
        self.centralwidget.setStyleSheet("background-image: url(Images/carbon_gradient.png)")

        self.action_full_screen.triggered.connect(self.action_full_screen_triggered)

        self.add_dock_window(ConnectionWindow(), Qt.LeftDockWidgetArea)
        self.add_dock_window(UpdateWindow(), Qt.RightDockWidgetArea)
        self.add_dock_window(ConsoleWindow(), Qt.BottomDockWidgetArea)
        self.add_dock_window(GLWindow(), Qt.BottomDockWidgetArea)

        settings = QSettings()
        self.restoreGeometry(settings.value("windowGeometry").toByteArray())
        self.restoreState(settings.value("windowState").toByteArray())

        self.action_full_screen.setChecked(self.windowState() & Qt.WindowFullScreen)

        self.old_stats = None
        self.timer = QTimer()
        self.timer.timeout.connect(self.update_gui)
        self.timer.start(500)

    def closeEvent(self, event):
        settings = QSettings()
        settings.setValue("windowGeometry", self.saveGeometry())
        settings.setValue("windowState", self.saveState())
        event.accept()

    def add_dock_window(self, window, area):
        window.setWindowFlags(Qt.Widget)

        dock = QDockWidget()
        dock.setObjectName(window.windowTitle())
        dock.setWindowTitle(window.windowTitle())
        dock.setWidget(window)

        self.addDockWidget(area, dock)

        self.menu_view.insertAction(
            self.menu_view.actions()[-2],
            dock.toggleViewAction()
        )

    def action_full_screen_triggered(self):
        if self.action_full_screen.isChecked():
            self.showFullScreen()
        else:
            self.showNormal()

    def update_gui(self):     
        if dispatcher.conn is None:
            self.connectionLabel.setText("Not connected")
            self.statisticsLabel.setText("")
            self.old_stats = None
            return

        self.connectionLabel.setText(dispatcher.conn.description)

        # Update RX/TX statistics
        #
        t = datetime.now()

        if self.old_stats is not None:
            dt = (t - self.old_stats.t).total_seconds()
            self.statisticsLabel.setText(
                "RX: %d Byte/s, TX: %d Byte/s" % (
                    (dispatcher.conn.stats.rx_bytes - self.old_stats.rx_bytes) / dt,
                    (dispatcher.conn.stats.tx_bytes - self.old_stats.tx_bytes) / dt
                )
            )

        self.old_stats = copy(dispatcher.conn.stats)
        self.old_stats.t = t
