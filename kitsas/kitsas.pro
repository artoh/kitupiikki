# Kitsas (c) Arto Hyvättinen ja Kitsas Oy
# GPL License
#
# Tässä tiedostossa on tarvittavien kirjastojen
# määrittelyt. Muokkaa tarvittaessa tiedoston paikallista
# kopiota

# CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT



linux {
    DEFINES += USE_ZIPLIB
    LIBS += -lzip
}

windows {
    DEFINES += USE_ZIPLIB
    LIBS += -lzip
    LIBS += -lopenjp2
    LIBS += -lbcrypt
}

macx {
}


# Otetaan mukaan tiedostot, joissa määritellään
# Kitsaan käyttämät Qt-määrittelyt sekä
# lähdekoodit.

include(kitsas.pri)
include(sources.pri) 
include(pdftuonti.pri)







