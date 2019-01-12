QT += testlib

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app

HEADERS += ../kitupiikki/validator/ibanvalidator.h \
    ../kitupiikki/tuonti/tuontiapu.h

SOURCES +=  tst_tuontitesti.cpp \
    ../kitupiikki/validator/ibanvalidator.cpp \
    ../kitupiikki/tuonti/tuontiapu.cpp
