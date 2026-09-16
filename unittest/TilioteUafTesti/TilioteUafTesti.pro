include(../apptest.pri)

TARGET = TilioteUafTesti

windows {
    LIBS += -L$$PWD/../../../../openjpeg-v2.5.0-windows-x64/openjpeg-v2.5.0-windows-x64/lib/ -lopenjp2
    INCLUDEPATH += $$PWD/../../../../openjpeg-v2.5.0-windows-x64/openjpeg-v2.5.0-windows-x64/include
    DEPENDPATH += $$PWD/../../../../openjpeg-v2.5.0-windows-x64/openjpeg-v2.5.0-windows-x64/include
    LIBS += -lbcrypt
}

SOURCES += \
    tst_tilioteuaftesti.cpp

RESOURCES += \
    ../data/testidata.qrc
