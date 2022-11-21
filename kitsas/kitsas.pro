# Kitsas (c) Arto Hyvättinen ja Kitsas Oy
# GPL License
#
# Tässä tiedostossa on tarvittavien kirjastojen
# määrittelyt. Muokkaa tarvittaessa tiedoston paikallista
# kopiota

# CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT

equals(QT_MAJOR_VERSION,6) {
    LIBS += -lpoppler-qt6
}
equals(QT_MAJOR_VERSION,5) {
    LIBS += -lpoppler-qt5
}


LIBS += -lpoppler
LIBS += -lzip

windows {
    LIBS += -lopenjp2
    LIBS += -lbcrypt
}

macx {
    LIBS += -L/usr/local/opt/poppler-qt5/lib -lpoppler-qt6
    LIBS += -L/usr/local/opt/libzip/lib -lzip
    INCLUDEPATH += /usr/local/include
}

# Otetaan mukaan tiedostot, joissa määritellään
# Kitsaan käyttämät Qt-määrittelyt sekä
# lähdekoodit.

include(kitsas.pri)
include(sources.pri) 








