#include "laatijantositeluettelo.h"
#include "db/kirjanpito.h"
#include "db/tositetyyppimodel.h"
#include "model/euro.h"

LaatijanTositeLuettelo::LaatijanTositeLuettelo(RaportinLaatija *laatija, const RaporttiValinnat &valinnat) :
    LaatijanRaportti(laatija, valinnat)
{
}

void LaatijanTositeLuettelo::laadi()
{
    const QDate mista = valinnat().arvo(RaporttiValinnat::AlkuPvm).toDate();
    const QDate mihin = valinnat().arvo(RaporttiValinnat::LoppuPvm).toDate();


    KpKysely *kysely = kpk("/tositteet");
    kysely->lisaaAttribuutti("alkupvm", mista);
    kysely->lisaaAttribuutti("loppupvm", mihin);

    rk.asetaOtsikko(kaanna("TOSITELUETTELO"));
    rk.asetaKausiteksti(QString("%1 - %2").arg( mista.toString("dd.MM.yyyy") , mihin.toString("dd.MM.yyyy") ) );

    rk.lisaaSarake("ABC1234/99 ");
    rk.lisaaPvmSarake();
    if( valinnat().onko(RaporttiValinnat::TulostaKumppani))
        rk.lisaaVenyvaSarake(50);
    rk.lisaaVenyvaSarake();
    rk.lisaaSarake("XXX kpl ");
    rk.lisaaEurosarake();

    {
        RaporttiRivi otsikko;
        otsikko.lisaa(kaanna("Tosite"));
        otsikko.lisaa(kaanna("Pvm"));
        if( valinnat().onko(RaporttiValinnat::TulostaKumppani))
            otsikko.lisaa(kaanna("Asiakas/Toimittaja"));
        otsikko.lisaa(kaanna("Otsikko"));
        otsikko.lisaa(kaanna("Liitteitä"));
        otsikko.lisaa(kaanna("Summa €"),1,true);
        rk.lisaaOtsake(otsikko);
    }


    if( valinnat().onko(RaporttiValinnat::RyhmitteleTositelajit) ) {
        kysely->lisaaAttribuutti("jarjestys", valinnat().arvo(RaporttiValinnat::VientiJarjestys).toString() == "tosite"  ? "tyyppi,tosite" : "tyyppi,pvm");
    } else {
        kysely->lisaaAttribuutti("jarjestys", valinnat().arvo(RaporttiValinnat::VientiJarjestys).toString() == "tosite" ? "tosite" : "pvm");
    }

    connect( kysely, &KpKysely::vastaus, this, &LaatijanTositeLuettelo::dataSaapuu);
    kysely->kysy();

}

void LaatijanTositeLuettelo::dataSaapuu(QVariant *data)
{
    Euro lajisumma;
    Euro summa;

    int edellinentyyppi = -1;
    QDate edpvm;

    const bool samakausi = kp()->tilikaudet()->tilikausiPaivalle( valinnat().arvo(RaporttiValinnat::AlkuPvm).toDate() ).alkaa() ==
                           kp()->tilikaudet()->tilikausiPaivalle( valinnat().arvo(RaporttiValinnat::LoppuPvm).toDate()).alkaa();

    QVariantList lista = data->toList();
    for( const auto& item: qAsConst( lista )) {
        QVariantMap map = item.toMap();
        int tamatyyppi = map.value("tyyppi").toInt();

        if( valinnat().onko(RaporttiValinnat::RyhmitteleTositelajit)  && edellinentyyppi != tamatyyppi) {
            if( valinnat().onko(RaporttiValinnat::TulostaSummarivit)  && edellinentyyppi >= 0) {
                RaporttiRivi valisumma(RaporttiRivi::EICSV);
                valisumma.lisaa( kaanna("Yhteensä"), 5);
                valisumma.lisaa( lajisumma );
                valisumma.viivaYlle();
                rk.lisaaRivi(valisumma);
                lajisumma = Euro::Zero;
            }
            if( edellinentyyppi >= 0)
                rk.lisaaTyhjaRivi();

            RaporttiRivi laji(RaporttiRivi::EICSV);
            laji.lisaa( kaanna(kp()->tositeTyypit()->nimi(tamatyyppi)), 5 );
            laji.lihavoi();
            rk.lisaaRivi(laji);
            edellinentyyppi = tamatyyppi;
        }

        RaporttiRivi rivi;

        if( valinnat().onko(RaporttiValinnat::ErittelePaivat) && edpvm.isValid() &&
                map.value("pvm").toDate() != edpvm)
            rk.lisaaTyhjaRivi();
        edpvm = map.value("pvm").toDate();

        rivi.lisaaTositeTunnus( map.value("pvm").toDate(), map.value("sarja").toString(),
                                map.value("tunniste").toInt(), samakausi);

        rivi.lisaa( map.value("pvm").toDate() );
        if( valinnat().onko(RaporttiValinnat::TulostaKumppani))
            rivi.lisaa(map.value("kumppani").toString());
        rivi.lisaa( map.value("otsikko").toString());
        if( map.value("liitteita").toInt())
            rivi.lisaa( kaanna("%1 kpl").arg( map.value("liitteita").toInt() ));
        else
            rivi.lisaa("");

        Euro euro(map.value("summa").toString());
        rivi.lisaa( euro );

        lajisumma += euro;
        summa += euro;

        rk.lisaaRivi( rivi );

    }
    if( valinnat().onko(RaporttiValinnat::TulostaSummarivit)  && edellinentyyppi) {
        RaporttiRivi valisumma(RaporttiRivi::EICSV);
        valisumma.lisaa(kaanna("Yhteensä"), valinnat().onko(RaporttiValinnat::TulostaKumppani) ? 5 : 4);
        valisumma.lisaa( lajisumma );
        valisumma.viivaYlle();
        rk.lisaaRivi(valisumma);
    }
    if( valinnat().onko(RaporttiValinnat::TulostaSummarivit)  ) {
        rk.lisaaTyhjaRivi();
        RaporttiRivi summarivi(RaporttiRivi::EICSV);
        summarivi.lisaa(kaanna("Yhteensä"), valinnat().onko(RaporttiValinnat::TulostaKumppani) ? 5 : 4);
        summarivi.lisaa( summa );
        summarivi.viivaYlle();
        summarivi.lihavoi();
        rk.lisaaRivi(summarivi);
    }

    valmis();
}
