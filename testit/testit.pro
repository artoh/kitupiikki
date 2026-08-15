QT += testlib

QT += gui
QT += widgets
QT += sql
QT += printsupport
QT += network
QT += svg
QT += xml
QT += pdf
QT += pdfwidgets
QT += webenginewidgets

equals(QT_MAJOR_VERSION,6) {
    QT += core5compat
    QT += svgwidgets
}

linux {
    DEFINES += USE_ZIPLIB
    LIBS += -lzip
}


CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app

INCLUDEPATH += $$PWD/../kitsas
VPATH += $$PWD/../kitsas
include(../kitsas/sources.pri)
include(../kitsas/pdftuonti.pri)

RESOURCES += \
    data/testidata.qrc

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
