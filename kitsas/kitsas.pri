
QT += gui
QT += widgets
QT += sql
QT += printsupport
QT += network
QT += svg
QT += xml

CONFIG += c++14

TARGET = kitsas

TEMPLATE = app

SOURCES += main.cpp

DISTFILES += \
    uusikp/luo.sql \
    aloitussivu/qrc/avaanappi.png \
    aloitussivu/qrc/aloitus.css \
    uusikp/update3.sql

TRANSLATIONS = tr/kitsas_en.ts \
               tr/kitsas_sv.ts

RC_ICONS = kitsas.ico 
