 
QT += testlib

QT += gui
QT += widgets
QT += sql
QT += printsupport
QT += network
QT += svg
QT += xml

LIBS += -lpoppler-qt5
LIBS += -lpoppler
LIBS += -lzip

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app

INCLUDEPATH += $$PWD/../kitsas
VPATH += $$PWD/../kitsas

include(../kitsas/sources.pri)
