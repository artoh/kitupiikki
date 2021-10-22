#include "laatijanpaivakirja.h"
#include "db/kirjanpito.h"
#include "db/tositetyyppimodel.h"

LaatijanPaivakirja::LaatijanPaivakirja(RaportinLaatija *laatija, const RaporttiValinnat &valinnat) :
    LaatijanRaportti(laatija, valinnat)
{

}

void LaatijanPaivakirja::laadi()
{
    const QDate mista = valinnat().arvo(RaporttiValinnat::AlkuPvm).toDate();
    const QDate mihin = valinnat().arvo(RaporttiValinnat::LoppuPvm).toDate();
    const int kohdennuksella = valinnat().arvo(RaporttiValinnat::Kohdennuksella).toInt();

    const Tilikausi alkukausi = kp()->tilikaudet()->tilikausiPaivalle(mista);
    const Tilikausi loppukausi = kp()->tilikaudet()->tilikausiPaivalle(mihin);
    samatilikausi_ = alkukausi.alkaa() == loppukausi.alkaa();


    if( kohdennuksella > -1 ) {
        // Tulostetaan vain yhdestä kohdennuksesta
        rk.asetaOtsikko( QString("%1 (%2)").arg(kaanna("PÄIVÄKIRJA"), kp()->kohdennukset()->kohdennus(kohdennuksella).nimi() ) );
    } else
    rk.asetaOtsikko(kaanna("PÄIVÄKIRJA"));



    rk.asetaKausiteksti(QString("%1 - %2").arg( mista.toString("dd.MM.yyyy") )
                                             .arg( mihin.toString("dd.MM.yyyy") ) );

    rk.lisaaPvmSarake();
    if( kp()->asetukset()->onko("erisarjaan") )
        rk.lisaaSarake("ABC1234/99 ");
    else
        rk.lisaaSarake("12345");

    rk.lisaaSarake("1234 ArvonlisäverosaamisetXX");
    if(  valinnat().onko(RaporttiValinnat::TulostaKohdennus) )
        rk.lisaaSarake("Kohdennusnimi");
    if( valinnat().onko(RaporttiValinnat::TulostaKumppani) )
        rk.lisaaVenyvaSarake(75);
    rk.lisaaVenyvaSarake();
    rk.lisaaEurosarake();
    rk.lisaaEurosarake();

    {
        RaporttiRivi otsikko;
        otsikko.lisaa(kaanna("Pvm"));
        otsikko.lisaa(kaanna("Tosite"));
        otsikko.lisaa(kaanna("Tili"));
        if( valinnat().onko(RaporttiValinnat::TulostaKohdennus) )
            otsikko.lisaa(kaanna("Kohdennus"));
        if( valinnat().onko(RaporttiValinnat::TulostaKumppani) )
            otsikko.lisaa(kaanna("Asiakas/Toimittaja"));
        otsikko.lisaa(kaanna("Selite"));
        otsikko.lisaa(kaanna("Debet €"), 1, true);
        otsikko.lisaa(kaanna("Kredit €"), 1, true);
        rk.lisaaOtsake(otsikko);
    }

    KpKysely *kysely = kpk("/viennit");
    kysely->lisaaAttribuutti("alkupvm", mista);
    kysely->lisaaAttribuutti("loppupvm", mihin);

    if( valinnat().arvo(RaporttiValinnat::VientiJarjestys).toString() == "tosite" ) {
        if( valinnat().onko(RaporttiValinnat::RyhmitteleTositelajit) )
            kysely->lisaaAttribuutti("jarjestys","laji,tosite");
        else
            kysely->lisaaAttribuutti("jarjestys","tosite");
    } else if( valinnat().onko( RaporttiValinnat::RyhmitteleTositelajit ) )
        kysely->lisaaAttribuutti("jarjestys","laji");

    if( kohdennuksella > -1)
        kysely->lisaaAttribuutti("kohdennus", kohdennuksella);

    connect( kysely, &KpKysely::vastaus, this, &LaatijanPaivakirja::dataSaapuu);

    kysely->kysy();

}

void LaatijanPaivakirja::dataSaapuu(QVariant *data)
{
    QVariantList lista = data->toList();
    QDate edpaiva;

    int edellinentyyppi = 0;

    qlonglong debetsumma = 0;
    qlonglong kreditsumma = 0;

    qlonglong debetvalisumma = 0;
    qlonglong kreditvalisumma = 0;


    for( const auto& item : qAsConst( lista )) {
        QVariantMap map = item.toMap();

        int tositetyyppi = map.value("tosite").toMap().value("tyyppi").toInt();
        if(  valinnat().onko(RaporttiValinnat::RyhmitteleTositelajit)  && edellinentyyppi != tositetyyppi ) {
            if( valinnat().onko(RaporttiValinnat::TulostaSummarivit) && edellinentyyppi) {
                RaporttiRivi valisumma(RaporttiRivi::EICSV);
                valisumma.lisaa(kaanna("Yhteensä"), valinnat().onko(RaporttiValinnat::TulostaKohdennus) ? 5 : 4  );
                if( valinnat().onko(RaporttiValinnat::TulostaKumppani) )
                    valisumma.lisaa("");
                valisumma.lisaa( debetvalisumma);
                valisumma.lisaa(kreditvalisumma);
                valisumma.viivaYlle();
                rk.lisaaRivi(valisumma);

                debetvalisumma = 0l;
                kreditvalisumma = 0l;
            }

            if( edellinentyyppi)
                rk.lisaaTyhjaRivi();

            RaporttiRivi ryhma(RaporttiRivi::EICSV);
            ryhma.lisaa( kaanna(kp()->tositeTyypit()->nimi(tositetyyppi)),4 );
            ryhma.lihavoi();
            rk.lisaaRivi(ryhma);

            edellinentyyppi = tositetyyppi;
        }


        RaporttiRivi rivi;

        QDate pvm = map.value("pvm").toDate();

        if( valinnat().onko(RaporttiValinnat::ErittelePaivat) && pvm != edpaiva && edpaiva.isValid())
            rk.lisaaTyhjaRivi();
        edpaiva = pvm;

        rivi.lisaa( pvm );

        // Ei toisteta turhaan tilikauden tunnusta
        QVariantMap tositemap = map.value("tosite").toMap();
        rivi.lisaaTositeTunnus( tositemap.value("pvm").toDate(), tositemap.value("sarja").toString(), tositemap.value("tunniste").toInt(), samatilikausi_ );

        Tili* tili = kp()->tilit()->tili( map.value("tili").toInt() );
        if( tili )
            rivi.lisaaLinkilla(RaporttiRiviSarake::TILI_NRO, tili->numero(), tili->nimiNumero(kielikoodi()));
        else
            continue;   // ei kelvollista tiliä!

        if( valinnat().onko(RaporttiValinnat::TulostaKohdennus)  )
            rivi.lisaa( kp()->kohdennukset()->kohdennus( map.value("kohdennus").toInt() ).nimi(kielikoodi()) );

        QString kumppani = map.value("kumppani").toMap().value("nimi").toString();
        QString selite = map.value("selite").toString();

        if( valinnat().onko(RaporttiValinnat::TulostaKumppani) )
            rivi.lisaa( kumppani );
        rivi.lisaa( valinnat().onko(RaporttiValinnat::TulostaKumppani) && selite == kumppani ? "" : selite );

        qlonglong debetsnt = qRound64( map.value("debet").toDouble() * 100.0 );
        qlonglong kreditsnt = qRound64( map.value("kredit").toDouble() * 100.0 );

        debetsumma += debetsnt;
        debetvalisumma += debetsnt;
        kreditsumma += kreditsnt;
        kreditvalisumma += kreditsnt;

        rivi.lisaa( debetsnt );
        rivi.lisaa( kreditsnt );

        rk.lisaaRivi(rivi);

    }

    if( valinnat().onko(RaporttiValinnat::TulostaSummarivit)  && edellinentyyppi) {
        RaporttiRivi valisumma(RaporttiRivi::EICSV);
        valisumma.lisaa(kaanna("Yhteensä"), valinnat().onko(RaporttiValinnat::TulostaKohdennus) ? 5 : 4  );
        if( valinnat().onko(RaporttiValinnat::TulostaKumppani))
            valisumma.lisaa("");
        valisumma.lisaa( debetvalisumma);
        valisumma.lisaa(kreditvalisumma);
        valisumma.viivaYlle();
        rk.lisaaRivi(valisumma);
    }

    if( valinnat().onko(RaporttiValinnat::TulostaSummarivit) ) {
        rk.lisaaTyhjaRivi();
        RaporttiRivi summarivi(RaporttiRivi::EICSV);
        summarivi.lisaa(kaanna("Yhteensä"), valinnat().onko(RaporttiValinnat::TulostaKohdennus)  ? 5 : 4  );
        if( valinnat().onko(RaporttiValinnat::TulostaKumppani) )
            summarivi.lisaa("");
        summarivi.lisaa( debetsumma);
        summarivi.lisaa(kreditsumma);
        summarivi.viivaYlle();
        summarivi.lihavoi();
        rk.lisaaRivi(summarivi);
    }

    valmis();
}
