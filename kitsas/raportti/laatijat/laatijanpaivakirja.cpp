#include "laatijanpaivakirja.h"
#include "db/kirjanpito.h"
#include "db/tositetyyppimodel.h"

#include "model/euro.h"

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

    if( valinnat().onko(RaporttiValinnat::TulostaKumppani) && valinnat().onko(RaporttiValinnat::TulostaKohdennus)) {
        rk.lisaaVenyvaSarake(100, RaporttiRivi::PDF); // Kumppani + selite
        rk.lisaaSarake("", RaporttiRivi::EIPDF); // Kumppani
        rk.lisaaSarake("", RaporttiRivi::EIPDF); // Selite
    } else if( valinnat().onko(RaporttiValinnat::TulostaKumppani)) {
        rk.lisaaVenyvaSarake(75); // Kumppani
        rk.lisaaVenyvaSarake(); // Selite
    } else {
        rk.lisaaVenyvaSarake(); // Selite
    }

    if( valinnat().onko(RaporttiValinnat::NaytaAlvProsentti))
        rk.lisaaSarake("EU25,5%");
    rk.lisaaEurosarake();
    rk.lisaaEurosarake();

    {
        RaporttiRivi otsikko;
        otsikko.lisaa(kaanna("Pvm"));
        otsikko.lisaa(kaanna("Tosite"));
        otsikko.lisaa(kaanna("Tili"));
        if( valinnat().onko(RaporttiValinnat::TulostaKohdennus) )
            otsikko.lisaa(kaanna("Kohdennus"));

        if( valinnat().onko(RaporttiValinnat::TulostaKumppani) && valinnat().onko(RaporttiValinnat::TulostaKohdennus)) {
            otsikko.lisaa(kaanna("Asiakas/Toimittaja") + ", " + kaanna("Selite"));
            otsikko.lisaa(kaanna("Asiakas/Toimittaja"));
        } else if( valinnat().onko(RaporttiValinnat::TulostaKumppani)) {
            otsikko.lisaa(kaanna("Asiakas/Toimittaja"));
        }
        otsikko.lisaa(kaanna("Selite"));

        if( valinnat().onko(RaporttiValinnat::NaytaAlvProsentti))
            otsikko.lisaa(kaanna("ALV"));

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

    const int EITYYPPIA = -99;
    int edellinentyyppi = EITYYPPIA;

    Euro debetsumma;
    Euro kreditsumma;

    Euro debetvalisumma;
    Euro kreditvalisumma;


    for( const auto& item : qAsConst( lista )) {
        QVariantMap map = item.toMap();

        int tositetyyppi = map.value("tosite").toMap().value("tyyppi").toInt();
        if(  valinnat().onko(RaporttiValinnat::RyhmitteleTositelajit)  && edellinentyyppi != tositetyyppi ) {
            if( valinnat().onko(RaporttiValinnat::TulostaSummarivit) && edellinentyyppi != EITYYPPIA) {
                RaporttiRivi valisumma(RaporttiRivi::EICSV);
                valisumma.lisaa(kaanna("Yhteensä"), rk.sarakkeita() - 2  );
                valisumma.lisaa( debetvalisumma, true);
                valisumma.lisaa(kreditvalisumma, true);
                valisumma.viivaYlle();
                rk.lisaaRivi(valisumma);

                debetvalisumma = Euro::Zero;
                kreditvalisumma = Euro::Zero;
            }

            if( edellinentyyppi != EITYYPPIA)
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
            rivi.lisaa( kaanna("tili_puuttuu") );

        if( valinnat().onko(RaporttiValinnat::TulostaKohdennus)  )
            rivi.lisaa( kp()->kohdennukset()->kohdennus( map.value("kohdennus").toInt() ).nimi(kielikoodi()) );

        const QString kumppani = map.value("kumppani").toMap().value("nimi").toString();
        const QString selite = map.value("selite").toString();
        const QString tselite = kumppani == selite ? "" : selite;

        if( valinnat().onko(RaporttiValinnat::TulostaKumppani) && valinnat().onko(RaporttiValinnat::TulostaKohdennus)) {
            rivi.lisaa(kumppani + (kumppani.isEmpty() || tselite.isEmpty() ? "" : "\n") + tselite);
            rivi.lisaa(kumppani);
            rivi.lisaa(selite);
        } else if( valinnat().onko(RaporttiValinnat::TulostaKumppani)) {
            rivi.lisaa(kumppani);
            rivi.lisaa(tselite);
        } else {
            rivi.lisaa(selite.isEmpty() ? kumppani : selite);
        }
        if( valinnat().onko(RaporttiValinnat::NaytaAlvProsentti)) {
            rivi.lisaa( alvTeksti(map), 1, true );
        }

        Euro debetsnt = Euro::fromString( map.value("debet").toString() );
        Euro kreditsnt = Euro::fromString( map.value("kredit").toString() );

        debetsumma += debetsnt;
        debetvalisumma += debetsnt;

        kreditsumma += kreditsnt;
        kreditvalisumma += kreditsnt;

        rivi.lisaa( debetsnt );
        rivi.lisaa( kreditsnt );

        rk.lisaaRivi(rivi);

    }


    if( valinnat().onko(RaporttiValinnat::TulostaSummarivit)  && edellinentyyppi != EITYYPPIA) {
        RaporttiRivi valisumma(RaporttiRivi::EICSV);
        valisumma.lisaa(kaanna("Yhteensä"), rk.sarakkeita() - 2);
        valisumma.lisaa( debetvalisumma, true);
        valisumma.lisaa(kreditvalisumma, true);
        valisumma.viivaYlle();
        rk.lisaaRivi(valisumma);
    }

    if( valinnat().onko(RaporttiValinnat::TulostaSummarivit) ) {
        rk.lisaaTyhjaRivi();
        RaporttiRivi summarivi(RaporttiRivi::EICSV);
        summarivi.lisaa(kaanna("Yhteensä"), 2 );
        summarivi.lisaa(kaanna("Vientejä %1 kpl").arg(lista.count()));
        summarivi.lisaa("",rk.sarakkeita() - 5);
        summarivi.lisaa( debetsumma, true);
        summarivi.lisaa(kreditsumma, true);
        summarivi.viivaYlle();
        summarivi.lihavoi();
        rk.lisaaRivi(summarivi);
    }

    valmis();
}
