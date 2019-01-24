/*
   Copyright (C) 2017,2018 Arto Hyvättinen

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

#include <QSqlQuery>

#include "tilikarttaraportti.h"


TilikarttaRaportti::TilikarttaRaportti()
    : Raportti(nullptr)
{
    ui = new Ui::TilikarttaRaportti;
    ui->setupUi( raporttiWidget);

    connect( ui->tilikaudeltaCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(paivitaPaiva()));

    ui->tilikaudeltaCombo->setModel( kp()->tilikaudet() );
    ui->tilikaudeltaCombo->setModelColumn( TilikausiModel::KAUSI );
    ui->tilikaudeltaCombo->setCurrentIndex( ui->tilikaudeltaCombo->count() - 1);

    ui->saldotDate->setDateRange( kp()->tilikaudet()->kirjanpitoAlkaa(),
                                  kp()->tilikaudet()->kirjanpitoLoppuu());
}

TilikarttaRaportti::~TilikarttaRaportti()
{
    delete ui;
}

RaportinKirjoittaja TilikarttaRaportti::raportti()
{
    Tilikausi kausi = kp()->tilikaudet()->tilikausiPaivalle( ui->tilikaudeltaCombo->currentData( TilikausiModel::PaattyyRooli ).toDate() );

    KarttaValinta valinta = KAIKKI_TILIT;
    if( ui->kaytossaRadio->isChecked())
        valinta = KAYTOSSA_TILIT;
    else if( ui->kirjauksiaRadio->isChecked())
        valinta = KIRJATUT_TILIT;
    else if( ui->suosikkiRadio->isChecked())
        valinta = SUOSIKKI_TILIT;

    QDate saldopaiva;
    if( ui->saldotCheck->isChecked())
        saldopaiva = ui->saldotDate->date();

    return kirjoitaRaportti(valinta, kausi, ui->tilityypitCheck->isChecked(), saldopaiva, ui->kirjausohjeet->isChecked());
}

RaportinKirjoittaja TilikarttaRaportti::kirjoitaRaportti(TilikarttaRaportti::KarttaValinta valinta, const Tilikausi& tilikaudelta, bool tulostatyyppi, QDate saldopvm,
                                                         bool kirjausohjeet)
{
    RaportinKirjoittaja rk;
    rk.asetaOtsikko("TILILUETTELO");
    rk.asetaKausiteksti( tilikaudelta.kausivaliTekstina() );

    RaporttiRivi otsikko(RaporttiRivi::EICSV);
    RaporttiRivi csvOtsikko(RaporttiRivi::CSV);

    rk.lisaaSarake("12345678", RaporttiRivi::EICSV); // Ennnen tilinumeroa
    rk.lisaaSarake("12345678"); // Tilinumero
    rk.lisaaVenyvaSarake();

    csvOtsikko.lisaa("Numero");
    csvOtsikko.lisaa("Nimi");
    otsikko.lisaa(" ",3);

    if( tulostatyyppi )
    {
        rk.lisaaSarake("Tyyppiteksti pidennyksellä");
        otsikko.lisaa("Tilin tyyppi");
        csvOtsikko.lisaa("Tilin tyyppi");
    }
    if( saldopvm.isValid())
    {
        rk.lisaaSarake("Saldo XX.XX.XXXX");
        otsikko.lisaa( tr("Saldo %1").arg(saldopvm.toString("dd.MM.yyyy")));
        csvOtsikko.lisaa( tr("Saldo %1").arg(saldopvm.toString("dd.MM.yyyy")));
    }
    if( kirjausohjeet)
    {
        rk.lisaaSarake(" ");
        otsikko.lisaa("Kirjausohjeet");
        csvOtsikko.lisaa("Kirjausohjeet");
    }

    rk.lisaaOtsake( otsikko);
    rk.lisaaOtsake(csvOtsikko);

    // Tässä vaiheessa ei vielä välitetä kirjauksia-rajoitteesta
    QSet<int> tiliIdtKaytossa;

    // tiliIdtKaytossa - settiin lisätään kaikki, joissa kirjauksia ko. tilikaudella
    // Lisäksi käytössä ovat ne tasetilit, joilla on saldoa

    QSqlQuery kysely( QString("SELECT DISTINCT tili FROM vienti WHERE PVM BETWEEN \"%1\" AND \"%2\" ")
            .arg(tilikaudelta.alkaa().toString(Qt::ISODate))
            .arg(tilikaudelta.paattyy().toString(Qt::ISODate)) );
    while( kysely.next())
    {
        // Kirjataan myös "ylätileille"
        int tiliId = kysely.value(0).toInt();   // tilin id
        while( tiliId)
        {
            tiliIdtKaytossa.insert( tiliId);    // Merkitään, että on käytössä
            Tili tili = kp()->tilit()->tiliIdlla( tiliId );
            tiliId = tili.ylaotsikkoId();       // Haetaan seuraavaksi tämän ylätili
        }
    }

    QSet<int> ehtoTaytetty;     // Jos valitaan Käytössä tai Suosikit
    for(int i=0; i < kp()->tilit()->rowCount( QModelIndex());i++)
    {
        Tili tili = kp()->tilit()->tiliIndeksilla(i);
        if( (valinta == KAYTOSSA_TILIT && tili.tila() > 0) ||
            (valinta == SUOSIKKI_TILIT && tili.tila() > 1) )
        {
            ehtoTaytetty.insert(tili.id());
            int tiliId = tili.ylaotsikkoId();
            while( tiliId)
            {
                ehtoTaytetty.insert(tiliId);
                Tili tili = kp()->tilit()->tiliIdlla(tiliId);
                tiliId = tili.ylaotsikkoId();
            }
        }
    }



    for( int i=0; i < kp()->tilit()->rowCount(QModelIndex()); i++)
    {
        RaporttiRivi rr(RaporttiRivi::EICSV);
        RaporttiRivi csvr(RaporttiRivi::CSV);

        Tili tili = kp()->tilit()->tiliIndeksilla(i);

        if( valinta == KAYTOSSA_TILIT && !ehtoTaytetty.contains(tili.id()) && !tiliIdtKaytossa.contains( tili.id()))
            continue;   // Tili ei käytössä
        else if( valinta == SUOSIKKI_TILIT && !ehtoTaytetty.contains(tili.id() ) )
            continue;
        else if( valinta == KIRJATUT_TILIT )
        {
            if( tili.onko(TiliLaji::TASE)  )
            {
                // Tasetili luetellaan myös, jos sillä tilikauden alussa saldoa
                if( !tiliIdtKaytossa.contains(tili.id()) && !tili.saldoPaivalle( tilikaudelta.alkaa() ))
                    continue;
            }
            else
            {
                if( !tiliIdtKaytossa.contains( tili.id()))
                    continue;
            }
        }


        if( tili.otsikkotaso() )
        {

            csvr.lisaa( QString::number(tili.numero()));
            csvr.lisaa( tili.nimi() );
            if( tulostatyyppi )
                csvr.lisaa( tr("Otsikko %1").arg(tili.otsikkotaso()));
            if( saldopvm.isValid())
                csvr.lisaa(" ");

            QString nimistr;
            for(int i=0; i < tili.otsikkotaso(); i++)
                nimistr.append("  ");
            nimistr.append(tili.nimi());
            rr.lisaa(nimistr, 3);

        }
        else
        {
            rr.lisaa("");

            rr.lisaaLinkilla(RaporttiRiviSarake::TILI_NRO, tili.numero(), QString::number(tili.numero()));
            csvr.lisaa( QString::number(tili.numero()) );


            csvr.lisaa( tili.nimi());


            QString teksti = tili.nimi();
            if( kirjausohjeet )
            {
                if( !tili.json()->str("Taydentava").isEmpty())
                    teksti.append("\n" + tili.json()->str("Taydentava"));
                if( !tili.json()->str("Kirjausohje").isEmpty())
                    teksti.append("\n" + tili.json()->str("Kirjausohje"));
            }

            rr.lisaaLinkilla(RaporttiRiviSarake::TILI_NRO, tili.numero(), teksti );

            if( tulostatyyppi)
            {
                rr.lisaa( kp()->tilit()->index(i, TiliModel::TYYPPI).data().toString());
                csvr.lisaa( kp()->tilit()->index(i, TiliModel::TYYPPI).data().toString());
            }
            if( saldopvm.isValid())
            {
                rr.lisaa( tili.saldoPaivalle(saldopvm));
                csvr.lisaa( tili.saldoPaivalle(saldopvm));
            }
        }
        if( kirjausohjeet )
        {
            csvr.lisaa( tili.json()->str("Kirjausohje"));
        }

        rk.lisaaRivi(rr);
        rk.lisaaRivi(csvr);
    }


    return rk;
}



void TilikarttaRaportti::paivitaPaiva()
{
    Tilikausi kausi = kp()->tilikaudet()->tilikausiPaivalle( ui->tilikaudeltaCombo->currentData( TilikausiModel::PaattyyRooli ).toDate() );
    ui->saldotDate->setDate( kausi.paattyy());
}
