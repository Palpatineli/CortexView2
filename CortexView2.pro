#-------------------------------------------------
#
# Project created by QtCreator 2015-09-11T17:43:01
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = CortexView2
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    sessiondialog.cpp \
    camera.cpp \
    saveworker.cpp \
    recordparams.cpp \
    cameraview.cpp \
    onlineprocessor.cpp \
    nidaqbox.cpp

HEADERS  += mainwindow.h \
    sessiondialog.h \
    camera.h \
    saveworker.h \
    recordparams.h \
    cameraview.h \
    onlineprocessor.h \
    nidaqbox.h

FORMS    += mainwindow.ui \
    sessiondialog.ui

RESOURCES += \
    icons.qrc

unix: INCLUDEPATH += "/home/palpatine/Documents/specs/PvcamSDK/Inc"
unix: INCLUDEPATH += /usr/include/hdf5/serial
unix: INCLUDEPATH += "/home/palpatine/Documents/specs/National Instruments/"
win32:INCLUDEPATH += "C:\Program Files (x86)\National Instruments\Shared\ExternalCompilerSupport\C\include"

unix: LIBS += /usr/lib/x86_64-linux-gnu/libhdf5_serial.so
unix: LIBS += /usr/lib/x86_64-linux-gnu/libhdf5_serial_hl.so
win32:LIBS += "C:\Program Files (x86)\National Instruments\Shared\ExternalCompilerSupport\C\lib64\msvc\NIDAQmx.lib"

DISTFILES += \
    README.txt

RC_FILE = appicon.rc

CONFIG += c++11
