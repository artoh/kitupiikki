QT += gui
QT += widgets
QT += sql
QT += webenginewidgets
QT += printsupport

CONFIG += c++11

TARGET = kitupiikki

TEMPLATE = app

SOURCES += main.cpp \
    uusikp/uusikirjanpito.cpp \
    uusikp/introsivu.cpp \
    uusikp/nimisivu.cpp \
    uusikp/tilikarttasivu.cpp \
    uusikp/loppusivu.cpp \
    uusikp/sijaintisivu.cpp \
    uusikp/tilikausisivu.cpp \
    kitupiikkiikkuna.cpp \
    aloitussivu/aloitussivu.cpp \
    aloitussivu/sisalto.cpp \
    db/kirjanpito.cpp \
    maaritys/perusvalinnat.cpp \
    maaritys/maarityssivu.cpp \
    kirjaus/kirjauswg.cpp \
    kirjaus/kirjaussivu.cpp \
    kirjaus/tositewg.cpp \
    db/tili.cpp \
    kirjaus/vientimodel.cpp \
    kirjaus/tilidelegaatti.cpp \
    kirjaus/eurodelegaatti.cpp \
    selaus/selauswg.cpp \
    db/tilikausi.cpp \
    selaus/selausmodel.cpp \
    raportti/raporttisivu.cpp \
    raportti/raportti.cpp \
    raportti/paivakirjaraportti.cpp \
    maaritys/tilinavaus.cpp \
    maaritys/tilinavausmodel.cpp \
    db/tositetyyppi.cpp \
    kirjaus/pvmdelegaatti.cpp \
    maaritys/tositelajit.cpp \
    maaritys/tositelajitmodel.cpp \
    db/projektimodel.cpp \
    db/kustannuspaikkamodel.cpp

HEADERS += \
    uusikp/uusikirjanpito.h \
    uusikp/introsivu.h \
    uusikp/nimisivu.h \
    uusikp/tilikarttasivu.h \
    uusikp/loppusivu.h \
    uusikp/sijaintisivu.h \
    uusikp/tilikausisivu.h \
    kitupiikkiikkuna.h \
    aloitussivu/aloitussivu.h \
    aloitussivu/sisalto.h \
    db/kirjanpito.h \
    maaritys/perusvalinnat.h \
    maaritys/maarityssivu.h \
    kirjaus/kirjauswg.h \
    kirjaus/kirjaussivu.h \
    kirjaus/tositewg.h \
    db/tili.h \
    kirjaus/vientimodel.h \
    kirjaus/tilidelegaatti.h \
    kirjaus/eurodelegaatti.h \
    selaus/selauswg.h \
    db/tilikausi.h \
    selaus/selausmodel.h \
    raportti/raporttisivu.h \
    raportti/raportti.h \
    raportti/paivakirjaraportti.h \
    maaritys/tilinavaus.h \
    maaritys/tilinavausmodel.h \
    db/tositetyyppi.h \
    kirjaus/pvmdelegaatti.h \
    maaritys/tositelajit.h \
    maaritys/tositelajitmodel.h \
    db/projektimodel.h \
    db/kustannuspaikkamodel.h

RESOURCES += \
    tilikartat/tilikartat.qrc \
    pic/pic.qrc \
    uusikp/sql.qrc \
    aloitussivu/qrc/aloitus.qrc

FORMS += \
    uusikp/intro.ui \
    uusikp/nimi.ui \
    uusikp/tilikartta.ui \
    uusikp/sijainti.ui \
    uusikp/tilikausi.ui \
    maaritys/perusvalinnat.ui \
    kirjaus/kirjaus.ui \
    kirjaus/tositewg.ui \
    selaus/selauswg.ui \
    raportti/raportti.ui \
    raportti/paivakirja.ui \
    maaritys/tilinavaus.ui \
    maaritys/tositelajit.ui

DISTFILES += \
    uusikp/luo.sql \
    aloitussivu/qrc/avaanappi.png \
    aloitussivu/qrc/aloitus.css
