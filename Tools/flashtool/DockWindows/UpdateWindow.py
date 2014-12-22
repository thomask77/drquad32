from UpdateWindow_ui import *
from Connection import connection
from BootProtocol import BootProtocol
from msg_structs import *
from Dispatcher import dispatcher

from PyQt4.QtCore import *
from PyQt4.QtGui import *

import traceback
from Queue import Queue


# TODO:
# * Auto Update


class UpdateWindow(QMainWindow, Ui_UpdateWindow):
    KEY_FILENAME = "update_filename"

    def __init__(self):
        QMainWindow.__init__(self)
        self.setupUi(self)

        fn = QSettings().value(self.KEY_FILENAME).toString()
        self.lineEdit.setText(fn)

        model = QFileSystemModel()
        model.setRootPath(QDir.currentPath())
        model.setNameFilters(["*.hex", ])
        model.setNameFilterDisables(True)

        completer = QCompleter()
        completer.setModel(model)
        completer.setCaseSensitivity(Qt.CaseInsensitive)

        # Note: This call may be slow on some windows machines.
        # Check if there are any dead network drives or CD-ROMs
        # that take a long time to load.
        #
        self.lineEdit.setCompleter(completer)

        self.lineEdit.textChanged.connect(self.line_edit_text_changed)
        self.browseButton.clicked.connect(self.browse_button_clicked)
        self.updateButton.clicked.connect(self.update_button_clicked)

        connection.ConnectionChanged.connect(self.connection_changed)

        self.queue = Queue()
        dispatcher.subscribe(self.queue, msg_shell_to_pc.MSG_ID)

    def dragEnterEvent(self, e):
        if (len(e.mimeData().urls()) == 1 and
                e.mimeData().urls()[0].isLocalFile()):
            e.accept()

    def dropEvent(self, e):
        self.lineEdit.setText(
            e.mimeData().urls()[0].toLocalFile()
        )

    def connection_changed(self, connected):
        self.updateButton.setEnabled(connected)

    def line_edit_text_changed(self, text):
        QSettings().setValue(self.KEY_FILENAME, text)

    def browse_button_clicked(self):
        fn = str(QFileDialog.getOpenFileName(
            self, "Open file", self.lineEdit.text(),
            "Intel HEX (*.hex);;All Files (*.*)"
        ))

        if fn == "":
            return

        self.lineEdit.setText(fn)

    def update_button_clicked(self):
        # TODO: qt-event for connection change, disable update button
        #
        if dispatcher.conn is None:
            return

        class CancelError(Exception):
            pass

        def progress(percent, text):
            pd.setValue(percent)
            pd.setLabelText(text)
            QApplication.processEvents()
            if pd.wasCanceled():
                raise CancelError()

        pd = QProgressDialog(self)
        pd.setWindowModality(Qt.WindowModal)
        pd.setWindowTitle("Updating...")

        fn = str(self.lineEdit.text())
        proto = BootProtocol()

        try:
            proto.write_hex_file(fn, progress)
            QApplication.beep()
        except CancelError:
            pass
        except:
            pd.close()
            QMessageBox.critical(self, "Firmware Update Failed", traceback.format_exc(), "&OK")

