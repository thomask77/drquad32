from ConnectionWindow_ui import *
from Dispatcher import dispatcher
from WiFlyListener import WiFlyListener
from Connection import (
    connection,
    SerialStream,
    TCPStream
)

from PyQt4.QtCore import *
from PyQt4.QtGui import *

import traceback
from datetime import datetime, timedelta
from serial.tools import list_ports


# TODO: Handle WM_DEVICECHANGE
# Select old selection
#

class ConnectionWindow(QMainWindow, Ui_ConnectionWindow):
    def __init__(self):
        QMainWindow.__init__(self)
        self.setupUi(self)

        self.setContextMenuPolicy(Qt.NoContextMenu)

        self.treeWidget.sortByColumn(0, Qt.AscendingOrder)
        self.treeWidget.setColumnWidth(0, 150)
        self.treeWidget.setColumnWidth(1, 0)

        self.treeWidget.currentItemChanged.connect(self.item_changed)
        self.treeWidget.itemActivated.connect(self.action_connect_triggered)
        self.action_scan.triggered.connect(self.update_gui)
        self.action_connect.triggered.connect(self.action_connect_triggered)
        self.action_disconnect.triggered.connect(self.action_disconnect_triggered)

        self.wifly_listener = WiFlyListener()

        connection.ConnectionChanged.connect(self.connection_changed)

        self.update_gui()

    def update_gui(self):
        self.treeWidget.setUpdatesEnabled(False)
        self.treeWidget.clear()

        # Add Serial ports
        #
        serial_items = QTreeWidgetItem()
        self.treeWidget.addTopLevelItem(serial_items)

        serial_items.setText(0, "Serial Ports")
        serial_items.setFlags(Qt.ItemIsEnabled)
        serial_items.setExpanded(True)

        for port, desc, hwid in list_ports.comports():
            item = QTreeWidgetItem()
            item.setText(0, port)
            item.setText(1, hwid)
            item.setText(2, desc)
            serial_items.addChild(item)

        # Add WiFly modules
        #
        wifly_items = QTreeWidgetItem()
        self.treeWidget.addTopLevelItem(wifly_items)

        wifly_items.setText(0, "WiFly Modules")
        wifly_items.setFlags(Qt.ItemIsEnabled)
        wifly_items.setExpanded(True)

        for ip, info in self.wifly_listener.clients.iteritems():
            item = QTreeWidgetItem()

            age = datetime.now() - info["last_seen"]

            if age > timedelta(seconds=10):
                for i in range(3):
                    item.setTextColor(i, QColor(255, 0, 0))

            if age > timedelta(seconds=30):
                for i in range(3):
                    item.setTextColor(i, QColor(128, 128, 128))

            item.setText(0, ip)
            item.setText(1, "AP: %s, CH: %d" % (info["ap_mac"], info["channel"]))
            item.setText(2, "ID: %s, RSSI: %d" % (info["device_id"], info["rssi"]))
            wifly_items.addChild(item)

        # TODO: #/\  ICMP7 hack
        #
        item = QTreeWidgetItem()

        # item.setText(0, "192.168.1.35")   # home
        # item.setText(0, "169.254.1.1")    # ad-hoc
        # item.setText(0, "t-kindler.de", port=80)
        #
        item.setText(0, "94.45.227.81")
        item.setText(2, "ICMP7")
        wifly_items.addChild(item)

        self.treeWidget.setUpdatesEnabled(True)

    def get_selected_item(self):
        item = self.treeWidget.currentItem()

        # ignore top-level items
        #
        if not item or not item.parent():
            return None

        protocol = str(item.parent().text(0))
        name = str(item.text(0))

        return (protocol, name)

    def action_connect_triggered(self):
        item = self.get_selected_item()
        if item is None:
            return

        QApplication.setOverrideCursor(Qt.WaitCursor)

        self.action_disconnect_triggered()

        try:
            if item[0] == "Serial Ports":
                dispatcher.conn = SerialStream(item[1])
            if item[0] == "WiFly Modules":
                dispatcher.conn = TCPStream(item[1])
        except:
            QMessageBox.critical(self, "Connect Failed", traceback.format_exc(), "&OK")

        QApplication.restoreOverrideCursor()

    def action_disconnect_triggered(self):
        if dispatcher.conn is None:
            return

        QApplication.setOverrideCursor(Qt.WaitCursor)
        dispatcher.conn.close()
        dispatcher.conn = None
        QApplication.restoreOverrideCursor()

    def connection_changed(self, connected):
        self.action_disconnect.setEnabled(connected)

    def item_changed(self):
        item = self.get_selected_item()
        self.action_connect.setEnabled(item is not None)
