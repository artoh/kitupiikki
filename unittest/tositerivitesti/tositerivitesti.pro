QT += testlib
QT -= gui

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app

INCLUDEPATH += $$PWD/../../kitsas
VPATH += $$PWD/../../kitsas

SOURCES +=  tst_tositerivitesti.cpp \
    model/tositerivi.cpp \
    model/euro.cpp \
    db/kantavariantti.cpp

HEADERS += model/tositerivi.h \
    model/euro.h \
    db/kantavariantti.h
