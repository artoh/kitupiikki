QT += gui
QT += widgets
QT += sql

CONFIG += c++11

TARGET = kitupiikki

TEMPLATE = app

SOURCES += main.cpp \
    uusikp/uusikirjanpito.cpp \
    uusikp/introsivu.cpp \
    uusikp/nimisivu.cpp \
    uusikp/tilikarttasivu.cpp \
    uusikp/loppusivu.cpp \
    uusikp/sijaintisivu.cpp \
    uusikp/tilikausisivu.cpp

HEADERS += \
    uusikp/uusikirjanpito.h \
    uusikp/introsivu.h \
    uusikp/nimisivu.h \
    uusikp/tilikarttasivu.h \
    uusikp/loppusivu.h \
    uusikp/sijaintisivu.h \
    uusikp/tilikausisivu.h

RESOURCES += \
    tilikartat/tilikartat.qrc \
    pic/pic.qrc

FORMS += \
    uusikp/intro.ui \
    uusikp/nimi.ui \
    uusikp/tilikartta.ui \
    uusikp/sijainti.ui \
    uusikp/tilikausi.ui
