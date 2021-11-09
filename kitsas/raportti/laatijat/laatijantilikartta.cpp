#include "laatijantilikartta.h"
#include "db/kirjanpito.h"

#include <bitset>

LaatijanTilikartta::LaatijanTilikartta(RaportinLaatija *laatija, const RaporttiValinnat &valinnat) :
    LaatijanRaportti(laatija, valinnat)
{

}

void LaatijanTilikartta::laadi()
{
    const QDate saldopvm = valinnat().arvo(RaporttiValinnat::SaldoPvm).toDate();
    const QDate kausipvm = valinnat().arvo(RaporttiValinnat::LuetteloPvm).toDate();

    if( saldopvm.isValid() || valinnat().arvo(RaporttiValinnat::LuettelonTilit).toString() == "kirjatut") {
        KpKysely *kysely = kpk("/saldot");
        kysely->lisaaAttribuutti("pvm", saldopvm.isValid() ? saldopvm : kausipvm);
        connect( kysely, &KpKysely::vastaus, this, &LaatijanTilikartta::saldotSaapuu );
        kysely->kysy();
    } else {
        saldotSaapuu( nullptr );
    }
}

void LaatijanTilikartta::saldotSaapuu(QVariant *data)
{
    const Tilikausi tilikausi = kp()->tilikaudet()->tilikausiPaivalle( valinnat().arvo(RaporttiValinnat::LuetteloPvm).toDate() );

    QVariantMap saldot;
    if( data )
        saldot = data->toMap();

    rk.asetaOtsikko(kaanna("TILILUETTELO"));
    rk.asetaKausiteksti( tilikausi.kausivaliTekstina() );

    RaporttiRivi otsikko(RaporttiRivi::EICSV);
    RaporttiRivi csvOtsikko(RaporttiRivi::CSV);

    rk.lisaaSarake("12345678", RaporttiRivi::EICSV); // Ennnen tilinumeroa
    rk.lisaaSarake("12345678"); // Tilinumero
    rk.lisaaVenyvaSarake();

    csvOtsikko.lisaa(tulkkaa("Numero", kielikoodi()));
    csvOtsikko.lisaa("Nimi");
    otsikko.lisaa(" ",3);

    const bool tyypit = valinnat().arvo(RaporttiValinnat::NaytaTyypit).toBool();
    const bool kirjausohjeet = valinnat().arvo(RaporttiValinnat::NaytaKirjausohjeet).toBool();
    const bool otsikot = valinnat().arvo(RaporttiValinnat::NaytaOtsikot).toBool();
    const QDate saldopvm = valinnat().arvo(RaporttiValinnat::SaldoPvm).toDate();
    const QString valinta = valinnat().arvo(RaporttiValinnat::LuettelonTilit).toString();

    if( tyypit )
    {
        rk.lisaaSarake("Tyyppiteksti pidennyksellä");
        otsikko.lisaa(tulkkaa("Tilin tyyppi", kielikoodi()));
        csvOtsikko.lisaa(tulkkaa("Tilin tyyppi", kielikoodi()));
    }
    if( saldopvm.isValid())
    {
        rk.lisaaSarake("Saldo XX.XX.XXXX");
        otsikko.lisaa( kaanna("Saldo %1").arg(saldopvm.toString("dd.MM.yyyy")));
        csvOtsikko.lisaa( kaanna("Saldo %1").arg(saldopvm.toString("dd.MM.yyyy")));
    }
    if( kirjausohjeet)
    {
        rk.lisaaSarake(" ");
        otsikko.lisaa(kaanna("Kirjausohjeet"));
        csvOtsikko.lisaa(kaanna("Kirjausohjeet"));
    }

    rk.lisaaOtsake( otsikko);
    rk.lisaaOtsake(csvOtsikko);

    QSet<int> indeksitKaytossa;
    QVector<bool> otsikkobitit(128);

    bool tilejakaytossa = false;

    for(int i=kp()->tilit()->rowCount()-1; i > -1; i--) {
        Tili* tili = kp()->tilit()->tiliPIndeksilla(i);

        if( tili->otsikkotaso()) {
            if( !otsikot)
                continue;

            if( tilejakaytossa ) {
                indeksitKaytossa.insert( i );
                for(int j=0; j < tili->otsikkotaso(); j++)
                    otsikkobitit.replace(j, true);
                tilejakaytossa = false;
            } else {
                if( otsikkobitit.value( tili->otsikkotaso())) {
                    indeksitKaytossa.insert(i);
                    for(int j= tili->otsikkotaso(); j < otsikkobitit.size(); j++)
                        otsikkobitit.replace(j, false);
                }
            }


        } else {
            if( valinta == "kaikki" ||
              ( valinta == "kaytossa" && ( tili->tila() || saldot.contains( QString::number(  tili->numero()) ))) ||
              ( valinta == "kirjatut" && saldot.contains( QString::number(  tili->numero()) )) ||
              ( valinta == "suosikki" && tili->tila() == Tili::TILI_SUOSIKKI)) {
                // Tämä tili tulee luetteloon
                indeksitKaytossa.insert( i );
                tilejakaytossa = true;
            }
        }
    }


    for( int i=0; i < kp()->tilit()->rowCount(QModelIndex()); i++)
    {
        if( !indeksitKaytossa.contains(i))
            continue;

        RaporttiRivi rr(RaporttiRivi::EICSV);
        RaporttiRivi csvr(RaporttiRivi::CSV);

        Tili *tili = kp()->tilit()->tiliPIndeksilla(i);

        if( tili->otsikkotaso() )
        {

            csvr.lisaa( QString::number(tili->numero()));
            csvr.lisaa( tili->nimi(kielikoodi()) );
            if( tyypit )
                csvr.lisaa( kaanna("Otsikko %1").arg(tili->otsikkotaso()));
            if( saldopvm.isValid())
                csvr.lisaa(" ");

            QString nimistr;
            for(int i=0; i < tili->otsikkotaso(); i++)
                nimistr.append("  ");
            nimistr.append(tili->nimi(kielikoodi()));
            rr.lisaa(nimistr, 3);

        }
        else
        {
            QString nrostr = QString::number( tili->numero());

            rr.lisaa("");

            rr.lisaaLinkilla(RaporttiRiviSarake::TILI_NRO, tili->numero(), QString::number(tili->numero()));
            csvr.lisaa( nrostr );


            csvr.lisaa( tili->nimi(kielikoodi()));


            QString teksti = tili->nimi(kielikoodi());

            if( kirjausohjeet )
            {
               if( !tili->ohje().isEmpty())
                    teksti.append("\n" + tili->ohje(kielikoodi()));
            }

            rr.lisaaLinkilla(RaporttiRiviSarake::TILI_NRO, tili->numero(), teksti );

            if( tyypit)
            {
                rr.lisaa( kaanna(tili->tyyppi().kuvaus()));
                csvr.lisaa( kaanna(tili->tyyppi().kuvaus()));
            }
            if( saldopvm.isValid())
            {

                rr.lisaa( saldot.value(nrostr).toDouble() );
                csvr.lisaa( saldot.value(nrostr).toDouble());
            }
        }
        if( kirjausohjeet )
        {
            csvr.lisaa( tili->ohje(kielikoodi()));
        }

        rk.lisaaRivi(rr);
        rk.lisaaRivi(csvr);
    }

    valmis();
}

