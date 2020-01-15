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

TARGET=kitupiikki-test
TEMPLATE = app

INCLUDEPATH += $$PWD/../kitsas
VPATH += $$PWD/../kitsas
include(../kitsas/sources.pri)

HEADERS += tuontitesti.h \
    kpdateedittesti.h \
    testiapu.h \
    tilitesti.h \
    tulomenoapuritesti.h \
    tulomenorivitesti.h

SOURCES +=  testit.cpp \
	kpdateedittesti.cpp \
	testiapu.cpp \
	tilitesti.cpp \
	tulomenoapuritesti.cpp \
	tulomenorivitesti.cpp \
	tuontitesti.cpp

RESOURCES += \
    data/testidata.qrc
