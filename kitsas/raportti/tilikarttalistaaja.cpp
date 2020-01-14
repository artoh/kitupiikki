/*
   Copyright (C) 2019 Arto Hyvättinen

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#include "tilikarttalistaaja.h"

#include "db/kirjanpito.h"

#include <bitset>

TiliKarttaListaaja::TiliKarttaListaaja(QObject *parent) : QObject(parent)
{

}

void TiliKarttaListaaja::kirjoita(TiliKarttaListaaja::KarttaValinta valinta, const Tilikausi &tilikaudelta, bool otsikot, bool tulostatyypit, const QDate &saldopvm, bool kirjausohjeet)
{
    valinta_ = valinta;
    tilikausi_ = tilikaudelta;
    otsikot_ = otsikot;
    tyypit_ = tulostatyypit;
    saldopvm_ = saldopvm;
    kirjausohjeet_ = kirjausohjeet;

    if( saldopvm.isValid()) {
        KpKysely *kysely = kpk("/saldot");
        kysely->lisaaAttribuutti("pvm", saldopvm);
        connect( kysely, &KpKysely::vastaus, this, &TiliKarttaListaaja::saldotSaapuu);
        kysely->kysy();
    } else {
        saldotSaapuu( nullptr );
    }
}

void TiliKarttaListaaja::saldotSaapuu(QVariant *data)
{
    QVariantMap saldot;
    if( data )
        saldot = data->toMap();

    RaportinKirjoittaja rk;
    rk.asetaOtsikko("TILILUETTELO");
    rk.asetaKausiteksti( tilikausi_.kausivaliTekstina() );

    RaporttiRivi otsikko(RaporttiRivi::EICSV);
    RaporttiRivi csvOtsikko(RaporttiRivi::CSV);

    rk.lisaaSarake("12345678", RaporttiRivi::EICSV); // Ennnen tilinumeroa
    rk.lisaaSarake("12345678"); // Tilinumero
    rk.lisaaVenyvaSarake();

    csvOtsikko.lisaa("Numero");
    csvOtsikko.lisaa("Nimi");
    otsikko.lisaa(" ",3);

    if( tyypit_ )
    {
        rk.lisaaSarake("Tyyppiteksti pidennyksellä");
        otsikko.lisaa("Tilin tyyppi");
        csvOtsikko.lisaa("Tilin tyyppi");
    }
    if( saldopvm_.isValid())
    {
        rk.lisaaSarake("Saldo XX.XX.XXXX");
        otsikko.lisaa( tr("Saldo %1").arg(saldopvm_.toString("dd.MM.yyyy")));
        csvOtsikko.lisaa( tr("Saldo %1").arg(saldopvm_.toString("dd.MM.yyyy")));
    }
    if( kirjausohjeet_)
    {
        rk.lisaaSarake(" ");
        otsikko.lisaa("Kirjausohjeet");
        csvOtsikko.lisaa("Kirjausohjeet");
    }

    rk.lisaaOtsake( otsikko);
    rk.lisaaOtsake(csvOtsikko);

    QSet<int> indeksitKaytossa;
    QVector<bool> otsikkobitit(128);

    bool tilejakaytossa = false;

    for(int i=kp()->tilit()->rowCount()-1; i > -1; i--) {
        Tili* tili = kp()->tilit()->tiliPIndeksilla(i);

        if( tili->otsikkotaso()) {
            if( !otsikot_)
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
            if( valinta_ == KAIKKI_TILIT ||
              ( valinta_ == KAYTOSSA_TILIT && ( tili->tila() || saldot.contains( QString::number(  tili->numero()) ))) ||
              ( valinta_ == KIRJATUT_TILIT && saldot.contains( QString::number(  tili->numero()) )) ||
              ( valinta_ == SUOSIKKI_TILIT && tili->tila() == Tili::TILI_SUOSIKKI)) {
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
            csvr.lisaa( tili->nimi() );
            if( tyypit_ )
                csvr.lisaa( tr("Otsikko %1").arg(tili->otsikkotaso()));
            if( saldopvm_.isValid())
                csvr.lisaa(" ");

            QString nimistr;
            for(int i=0; i < tili->otsikkotaso(); i++)
                nimistr.append("  ");
            nimistr.append(tili->nimi());
            rr.lisaa(nimistr, 3);

        }
        else
        {
            QString nrostr = QString::number( tili->numero());

            rr.lisaa("");

            rr.lisaaLinkilla(RaporttiRiviSarake::TILI_NRO, tili->numero(), QString::number(tili->numero()));
            csvr.lisaa( nrostr );


            csvr.lisaa( tili->nimi());


            QString teksti = tili->nimi();

            if( kirjausohjeet_ )
            {
               if( !tili->ohje().isEmpty())
                    teksti.append("\n" + tili->ohje());
            }

            rr.lisaaLinkilla(RaporttiRiviSarake::TILI_NRO, tili->numero(), teksti );

            if( tyypit_)
            {
                rr.lisaa( tili->tyyppi().kuvaus());
                csvr.lisaa( tili->tyyppi().kuvaus());
            }
            if( saldopvm_.isValid())
            {

                rr.lisaa( saldot.value(nrostr).toDouble() );
                csvr.lisaa( saldot.value(nrostr).toDouble());
            }
        }
        if( kirjausohjeet_ )
        {
            csvr.lisaa( tili->ohje());
        }

        rk.lisaaRivi(rr);
        rk.lisaaRivi(csvr);
    }

    emit valmis(rk);


}

