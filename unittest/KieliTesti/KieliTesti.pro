QT += testlib
QT -= gui

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app

INCLUDEPATH += $$PWD/../../kitsas
VPATH += $$PWD/../../kitsas

SOURCES +=  tst_monikielinen.cpp \
    kieli/monikielinen.cpp \
    kieli/kieli.cpp \
    kieli/kielet.cpp

HEADERS += kieli/kielet.h

RESOURCES += \
    ../data/testidata.qrc
