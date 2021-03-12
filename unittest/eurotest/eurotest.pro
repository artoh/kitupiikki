QT += testlib
QT -= gui

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app

INCLUDEPATH += $$PWD/../../kitsas
VPATH += $$PWD/../../kitsas

SOURCES +=  tst_eurotest.cpp  \
    model/euro.cpp

HEADERS += model/euro.h
