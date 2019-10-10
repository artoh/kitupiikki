
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


macx {
    LIBS += -L/usr/local/opt/poppler/lib -lpoppler-qt5
    LIBS += -L/usr/local/opt/libzip -lzip
    INCLUDEPATH += /usr/local/include
}

CONFIG += c++14

TARGET = kitupiikki

TEMPLATE = app

include(sources.pri)

SOURCES += main.cpp

DISTFILES += \
    uusikp/luo.sql \
    aloitussivu/qrc/avaanappi.png \
    aloitussivu/qrc/aloitus.css \
    uusikp/update3.sql


RC_ICONS = kitupiikki.ico














