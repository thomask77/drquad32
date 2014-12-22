from ConsoleWindow_ui import *
from msg_structs import *
from Dispatcher import dispatcher
from AnsiParser import AnsiParser

from PyQt4.QtCore import *
from PyQt4.QtGui import *

import sys
from datetime import datetime
from Queue import Queue

ansi_palette = {
    0:  QBrush(QColor("#000")),
    1:  QBrush(QColor("#C00")),
    2:  QBrush(QColor("#0C0")),
    3:  QBrush(QColor("#CC0")),
    4:  QBrush(QColor("#00C")),
    5:  QBrush(QColor("#C0C")),
    6:  QBrush(QColor("#0CC")),
    7:  QBrush(QColor("#CCC")),
    8:  QBrush(QColor("#444")),
    9:  QBrush(QColor("#F44")),
    10: QBrush(QColor("#4F4")),
    11: QBrush(QColor("#FF4")),
    12: QBrush(QColor("#44F")),
    13: QBrush(QColor("#F4F")),
    14: QBrush(QColor("#4FF")),
    15: QBrush(QColor("#FFF"))
}


test = """
\x1B[0m   _  \x1B[1;31m_\x1B[0m       \x1B[1;31m______\x1B[0m                             \x1B[1;31m______\x1B[0m
  / \x1B[1;30m\\\x1B[31m/\x1B[0m \x1B[31m\\\x1B[1;33m_\x1B[0m    \x1B[1;31m/\x1B[0m \x1B[31m____/\x1B[37m_______  __  ______   _  __ \x1B[1;31m/\x1B[0;31m_\x1B[37m  \x1B[31m__/\x1B[37m______   __  ___
 /  \x1B[1;31m/ \x1B[0m \x1B[1;33m/\x1B[0m \x1B[33m\\\x1B[37m  \x1B[1;31m/\x1B[0m \x1B[31m/\x1B[37m \x1B[1;31m__ \x1B[0m/ \x1B[1;30m__\x1B[0m/ _ \x1B[1;30m\\\x1B[0m/  \x1B[1;30m|\x1B[0m/  / _ \x1B[1;30m|\x1B[0m / \x1B[1;30m|\x1B[0m/ \x1B[1;30m/\x1B[0m  \x1B[1;31m/\x1B[0m \x1B[31m/\x1B[37m / \x1B[1;30m__\x1B[0m/ _\x1B[1;30m |\x1B[0m /\x1B[1;30m  |\x1B[0m/\x1B[1;30m  /\x1B[0m
 \\\x1B[1;30m_/\x1B[31m\\\x1B[0;31m_\x1B[1;33m/\x1B[0m  \x1B[33m/\x1B[37m \x1B[1;31m/\x1B[0m \x1B[31m/\x1B[1m_/\x1B[0m \x1B[31m/\x1B[37m/ \x1B[1;30m_/\x1B[0m/ \x1B[1;30m, _\x1B[0m/\x1B[1;30m /|_/ \x1B[0m/\x1B[1;30m __ |\x1B[0m/\x1B[1;30m    /\x1B[0m \x1B[1;31m /\x1B[0m \x1B[31m/\x1B[37m / \x1B[1;30m_/\x1B[0m/ \x1B[1;30m__\x1B[0m \x1B[1;30m|\x1B[0m/ \x1B[1;30m/|_/\x1B[0m \x1B[1;30m/\x1B[0m
      \x1B[1;33m\\\x1B[0;33m_/\x1B[37m  \x1B[1;31m\\\x1B[0;31m____/\x1B[37m/\x1B[1;30m___\x1B[0m/\x1B[1;30m_/\x1B[0m|\x1B[1;30m_\x1B[0m/\x1B[1;30m_/\x1B[0m  /\x1B[1;30m_\x1B[0m/\x1B[1;30m_/\x1B[0m |\x1B[1;30m_\x1B[0m/\x1B[1;30m_/|_/\x1B[0m  \x1B[1;31m/\x1B[0;31m_/\x1B[37m /\x1B[1;30m___\x1B[0m/\x1B[1;30m_/ \x1B[0m|\x1B[1;30m_\x1B[0m/\x1B[1;30m_/  \x1B[0m/\x1B[1;30m_/\x1B[0m
\x1B[0m

\x1B[0m\x1B[1m    \x1B[0m   \x1B[1m_\x1B[32m_\x1B[0;37m           \x1B[1m_\x1B[32m_\x1B[0;37m       \x1B[1m__\x1B[32m__\x1B[0;37m      \x1B[1m_\x1B[32m_\x1B[0;37m  \x1B[1m_\x1B[32m_\x1B[0;37m  \x1B[1;30m     \x1B[0;32m \x1B[1;30mf00^06\x1B[0;37m
\x1B[1;30m  \x1B[37m_\x1B[32m__\x1B[0;37m \x1B[1m/\x1B[0m \x1B[1;30m/\x1B[37m_\x1B[32m_\x1B[37m_\x1B[32m__ \x1B[37m_\x1B[32m__ \x1B[37m/\x1B[0m \x1B[1;30m/\x1B[32m \x1B[37m_\x1B[32m__ \x1B[0;37m \x1B[1m/\x1B[32m \x1B[37m/\x1B[0m \x1B[1;30m/\x1B[37m_\x1B[32m__ \x1B[37m_/\x1B[0m \x1B[1;30m/\x1B[32m_\x1B[37m/\x1B[0m \x1B[1;30m/\x1B[0;37m   \x1B[1m__\x1B[32m__\x1B[37m_\x1B[32m_ __\x1B[0;37m
 \x1B[1m(\x1B[30m_\x1B[32m-\x1B[30m<\x1B[32m/\x1B[0;37m  \x1B[1;32m'_\x1B[37m/\x1B[0m \x1B[1;32m-\x1B[30m_\x1B[32m)\x1B[0;37m \x1B[1;32m-\x1B[30m_)\x1B[0;37m \x1B[1;32m_\x1B[0;37m \x1B[1;32m\\\x1B[0;37m \x1B[1;32m_\x1B[0;37m \x1B[1;30m`\x1B[32m/ /\x1B[0;37m \x1B[1;30m/\x1B[37m/\x1B[32m _\x1B[0;37m \x1B[1;30m`\x1B[32m/ __/\x1B[0;37m \x1B[1;30m_\x1B[0;37m \x1B[1;32m\\\x1B[37m_/\x1B[0m \x1B[1;32m__\x1B[30m/\x1B[32m\\ \\\x1B[0;37m \x1B[1;30m/\x1B[0;37m
\x1B[1;32m/\x1B[30m___\x1B[32m/\x1B[30m_/\\_\\\\__/\\__\x1B[32m/\x1B[30m_.__/_,_\x1B[32m/\x1B[30m_\x1B[32m/\x1B[30m_\x1B[32m(\x1B[30m_)_,_/\\__\x1B[32m/\x1B[30m_/\x1B[32m/\x1B[30m_\x1B[32m(\x1B[30m_)__/\x1B[32m/\x1B[30m_\\_\\\x1B[0;37m
\x1B[0m
"""


# TODO:
# * http://doc.qt.digia.com/4.6/debug.html
#   qInstallMsgHandler() for qWarning(), qDebug()
# * Text Input
#
class ConsoleWindow(QMainWindow, Ui_ConsoleWindow):
    def __init__(self):
        QMainWindow.__init__(self)
        self.setupUi(self)

        self.setContextMenuPolicy(Qt.NoContextMenu)

        self.action_save.triggered.connect(self.action_save_triggered)
        self.action_clear.triggered.connect(self.action_clear_triggered)
        self.action_wrap.triggered.connect(self.action_wrap_triggered)

        self.cursor = QTextCursor(self.plainTextEdit.document())

        self.ansi_format = QTextCharFormat()

        self.ansi_parser = AnsiParser()
        self.ansi_parser.attrib_changed = self.ansi_attrib_changed
        self.ansi_parser.print_text = self.ansi_print_text

        self.queue = Queue()
        dispatcher.subscribe(self.queue, msg_shell_to_pc.MSG_ID)

        self.timer = QTimer()
        self.timer.timeout.connect(self.timer_timeout)
        self.timer.start(100)

    def action_save_triggered(self):
        t = datetime.now()

        fn = "log_%02d%02d%02d_%02d%02d%02d.txt" % (
            t.year % 100, t.month, t.day,
            t.hour, t.minute, t.second
        )

        modifiers = QApplication.keyboardModifiers()

        if modifiers != Qt.ShiftModifier:
            fn = str(QFileDialog.getSaveFileName(
                self, "Save console output", fn,
                "Text files (*.txt);;All Files (*.*)"
            ))

        if fn == "":
            return

        with open(fn, "w") as f:
            f.write(self.plainTextEdit.toPlainText())

    def action_clear_triggered(self):
        self.plainTextEdit.clear()

    def action_wrap_triggered(self):
        if self.action_wrap.isChecked():
            self.plainTextEdit.setLineWrapMode(QPlainTextEdit.WidgetWidth)
        else:
            self.plainTextEdit.setLineWrapMode(QPlainTextEdit.NoWrap)

    def ansi_print_text(self, text):
        self.cursor.insertText(text)

    def ansi_attrib_changed(self):
        fg = self.ansi_parser.foreground + (8 if self.ansi_parser.bold else 0)
        bg = self.ansi_parser.background
        
        self.ansi_format.setForeground(ansi_palette[fg])
        self.ansi_format.setBackground(ansi_palette[bg])

        self.cursor.setCharFormat(self.ansi_format)

    def timer_timeout(self):
        if self.action_pause.isChecked():
            return

        # combine messages
        #
        text = ""
        # text = "\x1b[31mRed\x1b[32mGreen\x1b[34mBlue"
        # text = test
        while not self.queue.empty():
            msg = self.queue.get()
            text += msg[2:]

        if text == "":
            return

        pte = self.plainTextEdit
        vsb = pte.verticalScrollBar()
        at_bottom = vsb.value() == vsb.maximum()

        pte.setUpdatesEnabled(False)

        self.ansi_parser.parse(text)
        sys.stdout.write(text)

        if at_bottom:
            vsb.setValue(vsb.maximum())

        pte.setUpdatesEnabled(True)
