u#-------------------------------------------------
#
# Project created by QtCreator 2014-12-30T18:40:50
#
#-------------------------------------------------

QT += core gui
QT += serialport
QT += widgets
QT += network
QT += concurrent
QT += printsupport
win32:QT += winextras

TARGET = quadcontrol
TEMPLATE = app

CONFIG += c++14

INCLUDEPATH += .. \
    Libraries/qcustomplot-1.3.0 \
    Libraries/qflightinstruments-20140126 \
    Libraries/half-1.11.0/include

SOURCES += main.cpp \
    MainWindow.cpp \
    DockWindows/ConnectionWindow.cpp \
    DockWindows/ConsoleWindow.cpp \
    DockWindows/UpdateWindow.cpp \
    DockWindows/PlotWindow.cpp \
    Connection.cpp \
    IntelHexFile.cpp \
    BootProtocol.cpp \
    WiFlyListener.cpp \
    PuTTYLauncher.cpp \
    Libraries/qcustomplot-1.3.0/qcustomplot.cpp \
    DockWindows/AddConnectionDialog.cpp \
    ../Shared/cobsr.c \
    ../Shared/crc16.c \
    ../Shared/crc32.c \
    ../Shared/errors.c \
    TangoColors.cpp \
    VersionInfo.cpp \
    DockWindows/GLWindow.cpp \
    DockWindows/MyGLWidget.cpp \
    GLTools.cpp \
    TerminalWidget.cpp

HEADERS += MainWindow.h \
    DockWindows/ConnectionWindow.h \
    DockWindows/ConsoleWindow.h \
    DockWindows/UpdateWindow.h \
    DockWindows/PlotWindow.h \
    Connection.h \
    IntelHexFile.h \
    BootProtocol.h \
    QProgressDialogEx.h \
    WiFlyListener.h \
    PuTTYLauncher.h \
    Libraries/qcustomplot-1.3.0/qcustomplot.h \
    DockWindows/AddConnectionDialog.h \
    TryAction.h \
    InterleavedFuture.h \
    ../Shared/cobsr.h \
    ../Shared/crc16.h \
    ../Shared/crc32.h \
    ../Shared/errors.h \
    ../Shared/msg_structs.h \
    TangoColors.h \
    VersionInfo.h \
    DockWindows/GLWindow.h \
    DockWindows/MyGLWidget.h \
    GLTools.h \
    TerminalWidget.h

FORMS += MainWindow.ui \
    DockWindows/ConnectionWindow.ui \
    DockWindows/ConsoleWindow.ui \
    DockWindows/UpdateWindow.ui \
    DockWindows/PlotWindow.ui \
    DockWindows/AddConnectionDialog.ui \
    DockWindows/GLWindow.ui

RESOURCES += \
    resources.qrc

ADD_VERSION_INFO = \
    ../Tools/add_version_info/add_version_info.py -rv --no-crc

# this is insane :/
#
linux:QMAKE_POST_LINK += $$ADD_VERSION_INFO $(OUT_PWD)$(TARGET)
win32:QMAKE_POST_LINK += $$ADD_VERSION_INFO $(DESTDIR_TARGET)
