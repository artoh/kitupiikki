/*
   Copyright (C) 2017 Arto Hyvättinen

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


TilikarttaRaportti::TilikarttaRaportti(QPrinter *printer)
    : Raportti(printer)
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

    return kirjoitaRaportti(valinta, kausi, ui->tilityypitCheck->isChecked(), saldopaiva);
}

RaportinKirjoittaja TilikarttaRaportti::kirjoitaRaportti(TilikarttaRaportti::KarttaValinta valinta, Tilikausi tilikaudelta, bool tulostatyyppi, QDate saldopvm)
{
    RaportinKirjoittaja rk;
    rk.asetaOtsikko("TILILUETTELO");
    rk.asetaKausiteksti( tilikaudelta.kausivaliTekstina() );

    RaporttiRivi otsikko;

    rk.lisaaSarake("12345678"); // Ennnen tilinumeroa
    rk.lisaaSarake("12345678"); // Tilinumero
    rk.lisaaVenyvaSarake();

    otsikko.lisaa(" ",3);

    if( tulostatyyppi )
    {
        rk.lisaaSarake("Tyyppiteksti pidennyksellä");
        otsikko.lisaa("Tilin tyyppi");
    }
    if( saldopvm.isValid())
    {
        rk.lisaaSarake("Saldo XX.XX.XXXX");
        otsikko.lisaa( tr("Saldo %1").arg(saldopvm.toString(Qt::SystemLocaleShortDate)));
    }
    rk.lisaaOtsake( otsikko);

    // Tässä vaiheessa ei vielä välitetä kirjauksia-rajoitteesta
    QSet<int> tiliIdtKaytossa;
    if( valinta == KIRJATUT_TILIT)
    {
        // tiliIdtKaytossa - settiin lisätään kaikki, joissa kirjauksia ko. tilikaudella
        // Lisäksi käytössä ovat ne tasetilit, joilla on saldoa

        QSqlQuery kysely( QString("SELECT tili FROM vienti WHERE PVM BETWEEN \"%1\" AND \"%2\" GROUP BY tili")
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

    }


    for( int i=0; i < kp()->tilit()->rowCount(QModelIndex()); i++)
    {
        RaporttiRivi rr;
        Tili tili = kp()->tilit()->tiliIndeksilla(i);

        if( valinta == KAYTOSSA_TILIT && tili.tila() == 0 )
            continue;   // Tili ei käytössä
        else if( valinta == SUOSIKKI_TILIT && tili.tila() < 2)
            continue;
        else if( valinta == KIRJATUT_TILIT )
        {
            if( tili.onkoTasetili() )
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
            QString nimistr;
            for(int i=0; i < tili.otsikkotaso(); i++)
                nimistr.append("  ");
            nimistr.append(tili.nimi());
            rr.lisaa(nimistr, 3);
        }
        else
        {
            rr.lisaa("");
            rr.lisaa(QString::number(tili.numero()));
            rr.lisaa(tili.nimi());
            if( tulostatyyppi)
                rr.lisaa( kp()->tilit()->index(i, TiliModel::TYYPPI).data().toString());
            if( saldopvm.isValid())
                rr.lisaa( tili.saldoPaivalle(saldopvm));
        }
        rk.lisaaRivi(rr);
    }


    return rk;
}



void TilikarttaRaportti::paivitaPaiva()
{
    Tilikausi kausi = kp()->tilikaudet()->tilikausiPaivalle( ui->tilikaudeltaCombo->currentData( TilikausiModel::PaattyyRooli ).toDate() );
    ui->saldotDate->setDate( kausi.paattyy());
}
