QT += pdf

CONFIG += c++14

CONFIG += qt console
CONFIG -= app_bundle

# DEFINES += QT_NO_DEBUG_OUTPUT
DEFINES += KITSAS_DEBUG

TEMPLATE = app

INCLUDEPATH += $$PWD/../kitsas
VPATH += $$PWD/../kitsas

include(../kitsas/pdftuonti.pri)


HEADERS += \
    pdftestiapu.h

SOURCES +=  \
    pdftesti.cpp \
    pdftestiapu.cpp
