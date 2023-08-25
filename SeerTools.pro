#-------------------------------------------------
#
# Project created by QtCreator 2017-04-14T13:49:57
#
#-------------------------------------------------

QT       += core gui network sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG += c++11
TRANSLATIONS += $$PWD/resource/Ch.ts

TRANSLATIONS += \
    zh.ts

TARGET = SeerTools
TEMPLATE = app

DESTDIR = $$PWD/bin
#include(libconfig.prf)

SOURCES += main.cpp\
        SCTcpToolWidget.cpp \
    SCStatusTcp.cpp \
    Core/BaseThread.cpp

HEADERS  += SCTcpToolWidget.h \
    SCStatusTcp.h \
    SCHeadData.h \
    Core/BaseThread.h

FORMS   += SCTcpToolWidget.ui

win32:RC_FILE = ICO.rc

RESOURCES += \
    resource.qrc

DISTFILES +=
