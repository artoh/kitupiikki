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

INCLUDEPATH += $$PWD/../kitupiikki
VPATH += $$PWD/../kitupiikki
include(../kitupiikki/sources.pri)

HEADERS += tuontitesti.h \
    tilitesti.h \
    tulomenorivitesti.h

SOURCES +=  testit.cpp \
	tilitesti.cpp \
	tulomenorivitesti.cpp \
	tuontitesti.cpp
