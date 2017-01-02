QT += gui
QT += widgets
QT += sql

CONFIG += c++11

TARGET = kitupiikki

TEMPLATE = app

SOURCES += main.cpp \
    uusikp/uusikirjanpito.cpp

HEADERS += \
    uusikp/uusikirjanpito.h

RESOURCES += \
    kitupiikki.qrc
