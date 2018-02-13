QT += gui
QT += widgets
QT += sql
QT += printsupport
QT += network

windows {
    LIBS += $$PWD/../poppler/lib/libpoppler-qt5.dll.a
    INCLUDEPATH += $$PWD/../poppler/
}

linux {
    LIBS += -lpoppler-qt5
}

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
    db/kirjanpito.cpp \
    maaritys/perusvalinnat.cpp \
    maaritys/maarityssivu.cpp \
    kirjaus/kirjauswg.cpp \
    kirjaus/kirjaussivu.cpp \
    db/tili.cpp \
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
    kirjaus/pvmdelegaatti.cpp \
    maaritys/tositelajit.cpp \
    db/tositelajimodel.cpp \
    db/asetusmodel.cpp \
    db/tilimodel.cpp \
    db/kohdennusmodel.cpp \
    db/kohdennus.cpp \
    db/tositelaji.cpp \
    db/tilikausimodel.cpp \
    kitupiikkisivu.cpp \
    raportti/raportinkirjoittaja.cpp \
    raportti/raporttirivi.cpp \
    db/tositemodel.cpp \
    db/vientimodel.cpp \
    db/liitemodel.cpp \
    db/jsonkentta.cpp \
    kirjaus/naytaliitewg.cpp \
    maaritys/tilikarttamuokkaus.cpp \
    db/tilinvalintaline.cpp \
    db/tilinvalintadialogi.cpp \
    maaritys/tilinmuokkausdialog.cpp \
    maaritys/kohdennusmuokkaus.cpp \
    maaritys/kohdennusdialog.cpp \
    maaritys/tositelajidialogi.cpp \
    kirjaus/kirjausapuridialog.cpp \
    db/verotyyppimodel.cpp \
    kirjaus/kohdennusdelegaatti.cpp \
    maaritys/raporttimuokkaus.cpp \
    maaritys/raportinkorostin.cpp \
    raportti/muokattavaraportti.cpp \
    ktpvienti/ktpintro.cpp \
    ktpvienti/ktpperustiedot.cpp \
    ktpvienti/ktpkuvaus.cpp \
    ktpvienti/ktpaloitusteksti.cpp \
    ktpvienti/ktpvienti.cpp \
    onniwidget.cpp \
    raportti/raportoija.cpp \
    raportti/paakirjaraportti.cpp \
    raportti/tilikarttaraportti.cpp \
    selaus/tositeselausmodel.cpp \
    arkistoija/arkistoija.cpp \
    raportti/tositeluetteloraportti.cpp \
    tilinpaatoseditori/tilinpaatoseditori.cpp \
    tilinpaatoseditori/tilinpaatostulostaja.cpp \
    maaritys/liitetietokaavamuokkaus.cpp \
    tilinpaatoseditori/tpaloitus.cpp \
    tilinpaatoseditori/mrichtexteditor/mrichtextedit.cpp \
    tilinpaatoseditori/mrichtexteditor/mtextedit.cpp \
    arkisto/arkistosivu.cpp \
    maaritys/maarityswidget.cpp \
    kirjaus/ehdotusmodel.cpp \
    db/eranvalintamodel.cpp \
    laskutus/laskutussivu.cpp \
    kirjaus/verodialogi.cpp \
    db/tilityyppimodel.cpp \
    kirjaus/taseeravalintadialogi.cpp \
    maaritys/alvmaaritys.cpp \
    maaritys/alvilmoitusdialog.cpp \
    maaritys/alvilmoitustenmodel.cpp \
    laskutus/laskumodel.cpp \
    laskutus/laskudialogi.cpp \
    laskutus/laskuntulostaja.cpp \
    laskutus/laskutusverodelegaatti.cpp \
    maaritys/laskuvalintawidget.cpp \
    laskutus/tuotemodel.cpp \
    laskutus/smtp.cpp \
    maaritys/emailmaaritys.cpp \
    laskutus/laskunmaksudialogi.cpp \
    laskutus/laskutmodel.cpp \
    raportti/taseerittely.cpp \
    arkisto/tilinpaattaja.cpp \
    arkisto/poistaja.cpp \
    maaritys/kaavankorostin.cpp \
    kirjaus/kohdennusproxymodel.cpp \
    db/tilinvalintaview.cpp \
    maaritys/tilikarttaohje.cpp \
    uusikp/paivitakirjanpito.cpp \
    arkisto/tararkisto.cpp \
    tuonti/tuonti.cpp \
    tuonti/pdftuonti.cpp

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
    db/kirjanpito.h \
    maaritys/perusvalinnat.h \
    maaritys/maarityssivu.h \
    kirjaus/kirjauswg.h \
    kirjaus/kirjaussivu.h \
    db/tili.h \
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
    kirjaus/pvmdelegaatti.h \
    maaritys/tositelajit.h \
    db/tositelajimodel.h \
    db/asetusmodel.h \
    db/tilimodel.h \
    db/kohdennusmodel.h \
    db/kohdennus.h \
    db/tositelaji.h \
    db/tilikausimodel.h \
    maaritys/maarityswidget.h \
    kitupiikkisivu.h \
    raportti/raportinkirjoittaja.h \
    raportti/raporttirivi.h \
    db/tositemodel.h \
    db/vientimodel.h \
    db/liitemodel.h \
    db/jsonkentta.h \
    kirjaus/naytaliitewg.h \
    maaritys/tilikarttamuokkaus.h \
    db/tilinvalintaline.h \
    db/tilinvalintadialogi.h \
    maaritys/tilinmuokkausdialog.h \
    maaritys/kohdennusmuokkaus.h \
    maaritys/kohdennusdialog.h \
    maaritys/tositelajidialogi.h \
    kirjaus/kirjausapuridialog.h \
    db/verotyyppimodel.h \
    kirjaus/kohdennusdelegaatti.h \
    maaritys/raporttimuokkaus.h \
    maaritys/raportinkorostin.h \
    raportti/muokattavaraportti.h \
    ktpvienti/ktpintro.h \
    ktpvienti/ktpperustiedot.h \
    ktpvienti/ktpkuvaus.h \
    ktpvienti/ktpaloitusteksti.h \
    ktpvienti/ktpvienti.h \
    onniwidget.h \
    raportti/raportoija.h \
    raportti/paakirjaraportti.h \
    raportti/tilikarttaraportti.h \
    selaus/tositeselausmodel.h \
    arkistoija/arkistoija.h \
    raportti/tositeluetteloraportti.h \
    tilinpaatoseditori/tilinpaatoseditori.h \
    tilinpaatoseditori/tilinpaatostulostaja.h \
    maaritys/liitetietokaavamuokkaus.h \
    tilinpaatoseditori/tpaloitus.h \
    tilinpaatoseditori/mrichtexteditor/mrichtextedit.h \
    tilinpaatoseditori/mrichtexteditor/mtextedit.h \
    arkisto/arkistosivu.h \
    kirjaus/ehdotusmodel.h \
    db/eranvalintamodel.h \
    laskutus/laskutussivu.h \
    kirjaus/verodialogi.h \
    db/tilityyppimodel.h \
    kirjaus/taseeravalintadialogi.h \
    maaritys/alvmaaritys.h \
    maaritys/alvilmoitusdialog.h \
    maaritys/alvilmoitustenmodel.h \
    laskutus/laskumodel.h \
    laskutus/laskudialogi.h \
    laskutus/laskuntulostaja.h \
    laskutus/laskutusverodelegaatti.h \
    maaritys/laskuvalintawidget.h \
    laskutus/tuotemodel.h \
    laskutus/smtp.h \
    maaritys/emailmaaritys.h \
    laskutus/laskunmaksudialogi.h \
    laskutus/laskutmodel.h \
    raportti/taseerittely.h \
    arkisto/tilinpaattaja.h \
    arkisto/poistaja.h \
    maaritys/kaavankorostin.h \
    kirjaus/kohdennusproxymodel.h \
    db/tilinvalintaview.h \
    maaritys/tilikarttaohje.h \
    uusikp/paivitakirjanpito.h \
    arkisto/tararkisto.h \
    tuonti/tuonti.h \
    tuonti/pdftuonti.h

RESOURCES += \
    tilikartat/tilikartat.qrc \
    pic/pic.qrc \
    uusikp/sql.qrc \
    aloitussivu/qrc/aloitus.qrc \
    arkistoija/arkisto.qrc

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
    raportti/paivakirja.ui \
    maaritys/tilinavaus.ui \
    maaritys/tositelajit.ui \
    maaritys/tilikarttamuokkaus.ui \
    maaritys/tilinmuokkaus.ui \
    db/tilinvalintadialogi.ui \
    maaritys/kohdennukset.ui \
    maaritys/kohdennusdialog.ui \
    maaritys/tositelajidialogi.ui \
    kirjaus/kirjausapuridialog.ui \
    maaritys/raportinmuokkaus.ui \
    raportti/muokattavaraportti.ui \
    ktpvienti/ktpintro.ui \
    ktpvienti/ktpperustiedot.ui \
    ktpvienti/ktpkuvaus.ui \
    ktpvienti/ktpaloitusteksti.ui \
    onniwidget.ui \
    raportti/tilikarttaraportti.ui \
    aloitussivu/aboutdialog.ui \
    tilinpaatoseditori/tpaloitus.ui \
    tilinpaatoseditori/mrichtexteditor/mrichtextedit.ui \
    aloitussivu/aloitus.ui \
    arkisto/arkisto.ui \
    arkisto/lisaatilikausidlg.ui \
    arkisto/lukitsetilikausi.ui \
    kirjaus/verodialogi.ui \
    kirjaus/taseeravalintadialogi.ui \
    maaritys/arvonlisavero.ui \
    maaritys/alvilmoitusdialog.ui \
    laskutus/laskutus.ui \
    laskutus/laskudialogi.ui \
    maaritys/laskumaaritys.ui \
    maaritys/emailmaaritys.ui \
    laskutus/laskunmaksudialogi.ui \
    raportti/taseerittely.ui \
    arkisto/tilinpaattaja.ui \
    arkisto/poistaja.ui \
    maaritys/lisaaraporttidialogi.ui \
    maaritys/kaavaeditori.ui \
    arkisto/muokkaatilikausi.ui \
    maaritys/tilikarttaohje.ui \
    aloitussivu/tervetuloa.ui \
    uusikp/tkpaivitys.ui \
    uusikp/paivityskorvaa.ui \
    arkisto/arkistonvienti.ui \
    raportti/csvvientivalinnat.ui

DISTFILES += \
    uusikp/luo.sql \
    aloitussivu/qrc/avaanappi.png \
    aloitussivu/qrc/aloitus.css

