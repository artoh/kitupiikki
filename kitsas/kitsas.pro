# Kitsas (c) Arto Hyvättinen ja Kitsas Oy
# GPL License
#
# Tässä tiedostossa on tarvittavien kirjastojen
# määrittelyt. Muokkaa tarvittaessa tiedoston paikallista
# kopiota

# CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT

LIBS += -lpoppler-qt6
LIBS += -lpoppler
LIBS += -lzip

windows {
    LIBS += -lopenjp2
    LIBS += -lbcrypt
}

# DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x050F00

macx {
    LIBS += -L/usr/local/opt/poppler-qt5/lib -lpoppler-qt5
    LIBS += -L/usr/local/opt/libzip/lib -lzip
    INCLUDEPATH += /usr/local/include
}

# Otetaan mukaan tiedostot, joissa määritellään
# Kitsaan käyttämät Qt-määrittelyt sekä
# lähdekoodit.

include(kitsas.pri)
include(sources.pri) 





