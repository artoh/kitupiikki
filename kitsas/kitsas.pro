# Kitsas (c) Arto Hyvättinen ja Kitsas Oy
# GPL License
#
# Tässä tiedostossa on tarvittavien kirjastojen
# määrittelyt. Muokkaa tarvittaessa tiedoston paikallista
# kopiota


LIBS += -lpoppler-qt5
LIBS += -lpoppler
LIBS += -lzip

windows {
    LIBS += -lopenjp2
    LIBS += -lbcrypt
}


macx {
    LIBS += -L/usr/local/opt/poppler/lib -lpoppler-qt5
    LIBS += -L/usr/local/opt/libzip -lzip
    INCLUDEPATH += /usr/local/include
}

# Otetaan mukaan tiedostot, joissa määritellään
# Kitsaan käyttämät Qt-määrittelyt sekä
# lähdekoodit.

include(kitsas.pri)
include(sources.pri) 

