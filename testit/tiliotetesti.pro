QT += gui
QT += widgets
QT += sql
QT += printsupport
QT += network
QT += svg
QT += xml
QT += qml

CONFIG += c++14

LIBS += -lpoppler-qt5
LIBS += -lpoppler
LIBS += -lzip

CONFIG += qt console
CONFIG -= app_bundle

DEFINES += QT_NO_DEBUG_OUTPUT

TEMPLATE = app

INCLUDEPATH += $$PWD/../kitsas
VPATH += $$PWD/../kitsas
include(../kitsas/sources.pri)

RESOURCES += \
    data/testidata.qrc

SOURCES +=  tiliotetesti.cpp
