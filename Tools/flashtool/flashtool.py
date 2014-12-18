#!/usr/bin/env python
#
import sys
import os

from PyQt4.QtGui import *

from MainWindow import MainWindow


if __name__ == "__main__":
    # reopen stdout file descriptor with write mode
    # and 0 as the buffer size (unbuffered)
    #
    sys.stdout = os.fdopen(sys.stdout.fileno(), 'w', 0)

    app = QApplication(sys.argv)
    app.setApplicationName("flashtool")
    app.setOrganizationName("t-kindler")
    app.setOrganizationDomain("t-kindler.de")

    # Main Window
    #
    main_window = MainWindow()
    main_window.show()
    ret = app.exec_()

    # tear down objects to prevent segfaults on exit
    #
    del main_window
    del app

    sys.exit(ret)

