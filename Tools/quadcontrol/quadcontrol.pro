#-------------------------------------------------
#
# Project created by QtCreator 2014-12-30T18:40:50
#
#-------------------------------------------------

QT += core gui
QT += serialport
QT += widgets
QT += opengl
QT += network
QT += concurrent
QT += printsupport
win32:QT += winextras

TARGET = quadcontrol
TEMPLATE = app

CONFIG += c++11

INCLUDEPATH += \
    Widgets/qcustomplot-1.3.0 \
    Widgets/qflightinstruments-20140126

SOURCES += main.cpp \
    MainWindow.cpp \
    AnsiParser.cpp \
    DockWindows/ConnectionWindow.cpp \
    DockWindows/ConsoleWindow.cpp \
    DockWindows/GLWindow.cpp \
    DockWindows/UpdateWindow.cpp \
    DockWindows/PlotWindow.cpp \
    ..\..\Libraries\cobs\cobsr.c \
    ..\..\Libraries\crc\crc16.c \
    ..\..\Libraries\crc\crc32.c \
    Connection.cpp \
    IntelHexFile.cpp \
    DockWindows/MyGLWidget.cpp \
    BootProtocol.cpp \
    glut_teapot.cpp \
    WiFlyListener.cpp \
    GLTools.cpp \
    PuTTYLauncher.cpp \
    Widgets/qcustomplot-1.3.0/qcustomplot.cpp \
    DockWindows/AddConnectionDialog.cpp

HEADERS  += MainWindow.h \
    AnsiParser.h \
    GLTools.h \
    DockWindows/ConnectionWindow.h \
    DockWindows/ConsoleWindow.h \
    DockWindows/GLWindow.h \
    DockWindows/UpdateWindow.h \
    DockWindows/PlotWindow.h \
    ..\..\Libraries\cobs\cobsr.h \
    ..\..\Libraries\crc\crc16.h \
    ..\..\Libraries\crc\crc32.h \
    Connection.h \
    IntelHexFile.h \
    DockWindows/MyGLWidget.h \
    BootProtocol.h \
    glut_teapot.h \
    QProgressDialogEx.h \
    ../../Bootloader/msg_structs.h \
    WiFlyListener.h \
    PuTTYLauncher.h \
    Widgets/qcustomplot-1.3.0/qcustomplot.h \
    DockWindows/AddConnectionDialog.h

FORMS    += MainWindow.ui \
    DockWindows/ConnectionWindow.ui \
    DockWindows/ConsoleWindow.ui \
    DockWindows/GLWindow.ui \
    DockWindows/UpdateWindow.ui \
    DockWindows/PlotWindow.ui \
    DockWindows/AddConnectionDialog.ui

RESOURCES += \
    resources.qrc

unix:LIBS += -lGLU
