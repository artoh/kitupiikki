QT += testlib
QT += gui

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app

SOURCES +=  tst_validatortest.cpp \
    ../../kitsas/validator/ibanvalidator.cpp \
    ../../kitsas/laskutus/iban.cpp

HEADERS += ../../kitsas/validator/ibanvalidator.h \
    ../../kitsas/laskutus/iban.h
