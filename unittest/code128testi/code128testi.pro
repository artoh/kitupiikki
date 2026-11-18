QT += testlib gui pdf

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app

INCLUDEPATH += $$PWD/../../kitsas
VPATH += $$PWD/../../kitsas

SOURCES += tst_code128.cpp \
    laskutus/tulostus/code128.cpp

HEADERS += laskutus/tulostus/code128.h
