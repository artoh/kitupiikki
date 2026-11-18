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

windows {
    LIBS += -L$$PWD/../../../../../openjpeg-v2.5.0-windows-x64/openjpeg-v2.5.0-windows-x64/lib/ -lopenjp2
    INCLUDEPATH += $$PWD/../../../../../openjpeg-v2.5.0-windows-x64/openjpeg-v2.5.0-windows-x64/include
    DEPENDPATH += $$PWD/../../../../../openjpeg-v2.5.0-windows-x64/openjpeg-v2.5.0-windows-x64/include
    LIBS += -lbcrypt
}

CONFIG += c++14
CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app

INCLUDEPATH += $$PWD/../../kitsas
VPATH += $$PWD/../../kitsas

include(../../kitsas/sources.pri)
include(../../kitsas/pdftuonti.pri)

SOURCES += tst_laskuviivakoodi.cpp
