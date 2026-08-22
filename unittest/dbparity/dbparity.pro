include(../apptest.pri)

TARGET = dbparity

DEFINES += KITSAS_SRC_DIR=\\\"$$PWD/../../kitsas\\\"

HEADERS += testdb.h
SOURCES += tst_dbparity.cpp testdb.cpp
