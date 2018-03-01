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

#include <QDateEdit>

#include <QSqlQuery>

#include "paivakirjaraportti.h"

#include "db/kirjanpito.h"
#include "db/tilikausi.h"

#include "raportinkirjoittaja.h"

#include <QDebug>
#include <QSqlError>

PaivakirjaRaportti::PaivakirjaRaportti()
    : Raportti()
{
    ui = new Ui::Paivakirja;
    ui->setupUi( raporttiWidget );

    Tilikausi nykykausi = Kirjanpito::db()->tilikausiPaivalle( Kirjanpito::db()->paivamaara() );
    ui->alkupvm->setDate(nykykausi.alkaa());
    ui->loppupvm->setDate(nykykausi.paattyy());

    ui->kohdennusCombo->setModel( kp()->kohdennukset());
    ui->kohdennusCombo->setModelColumn( KohdennusModel::NIMI);

    ui->tulostaviennitCheck->hide();
}

PaivakirjaRaportti::~PaivakirjaRaportti()
{
    delete ui;
}


RaportinKirjoittaja PaivakirjaRaportti::raportti(bool csvmuoto)
{
    int kohdennuksella = -1;
    if( ui->kohdennusCheck->isChecked())
        kohdennuksella = ui->kohdennusCombo->currentData( KohdennusModel::IdRooli).toInt();

    return kirjoitaRaportti( ui->alkupvm->date(), ui->loppupvm->date(),
                             kohdennuksella, ui->tositejarjestysRadio->isChecked(),
                             ui->ryhmittelelajeittainCheck->isChecked(), ui->tulostakohdennuksetCheck->isChecked() && !csvmuoto,
                             ui->tulostasummat->isChecked() && !csvmuoto);

}

RaportinKirjoittaja PaivakirjaRaportti::kirjoitaRaportti(QDate mista, QDate mihin, int kohdennuksella, bool tositejarjestys, bool ryhmitalajeittain, bool tulostakohdennukset, bool tulostasummat)
{

    RaportinKirjoittaja kirjoittaja;
    if( kohdennuksella > -1 )
        // Tulostetaan vain yhdestä kohdennuksesta
        kirjoittaja.asetaOtsikko( QString("PÄIVÄKIRJA (%1)").arg( kp()->kohdennukset()->kohdennus(kohdennuksella).nimi() ) );
    else
        kirjoittaja.asetaOtsikko("PÄIVÄKIRJA");


    kirjoittaja.asetaKausiteksti(QString("%1 - %2").arg( mista.toString(Qt::SystemLocaleShortDate) )
                                             .arg( mihin.toString(Qt::SystemLocaleShortDate) ) );

    kirjoittaja.lisaaPvmSarake();
    kirjoittaja.lisaaSarake("ABC1234/99 ");
    kirjoittaja.lisaaSarake("999999 Tilinimi tarkeinteilla");
    if(tulostakohdennukset )
        kirjoittaja.lisaaSarake("Kohdennusnimi");
    kirjoittaja.lisaaVenyvaSarake();
    kirjoittaja.lisaaEurosarake();
    kirjoittaja.lisaaEurosarake();

    RaporttiRivi otsikko;
    otsikko.lisaa("Pvm");
    otsikko.lisaa("Tosite");
    otsikko.lisaa("Tili");
    if( tulostakohdennukset )
        otsikko.lisaa("Kohdennus");
    otsikko.lisaa("Selite");
    otsikko.lisaa("Debet €", 1, true);
    otsikko.lisaa("Kredit €", 1, true);
    kirjoittaja.lisaaOtsake(otsikko);

    QSqlQuery kysely;
    QString jarjestys = "pvm, vientiId";
    if(  tositejarjestys )
        jarjestys = " tositelaji, tunniste, vientiId";
    else if( ryhmitalajeittain )
        jarjestys = " tositelaji, pvm, vientiId";

    // Kohdennuksen mukaan rajaaminen
    QString lisaehto;
    if( kohdennuksella > -1)
        lisaehto = QString(" AND kohdennusId=%1").arg( kohdennuksella );

    QString kysymys = QString("SELECT pvm, tositelaji, tunniste, tilinro, tilinimi, selite, debetsnt, kreditsnt, kohdennus, kohdennusId, tositelajiId, tositeId from vientivw "
                              "WHERE pvm BETWEEN \"%1\" AND \"%2\" %4 ORDER BY %3")
                              .arg(mista.toString(Qt::ISODate) )
                              .arg( mihin.toString(Qt::ISODate))
                              .arg(jarjestys).arg(lisaehto);

    kysely.exec(kysymys);

    int edellinenTositelajiId = -1;
    qlonglong debetYht = 0;
    qlonglong kreditYht = 0;
    qlonglong debetKaikki = 0;
    qlonglong kreditKaikki = 0;

    while( kysely.next())
    {
        if( ryhmitalajeittain && edellinenTositelajiId != kysely.value("tositelajiId").toInt())
        {
            if( edellinenTositelajiId > -1 )
            {
                if( tulostasummat )
                {
                    kirjoitaSummaRivi( kirjoittaja, debetYht, kreditYht, 4 + (int) tulostakohdennukset );
                    debetYht = 0;
                    kreditYht = 0;
                }
                kirjoittaja.lisaaRivi();
            }

            // Ryhmittely tositelajeittain: Tulostetaan tositelajien otsikot
            edellinenTositelajiId = kysely.value("tositelajiId").toInt();
            Tositelaji laji = kp()->tositelajit()->tositelaji( edellinenTositelajiId );
            RaporttiRivi rr;
            kirjoittaja.lisaaRivi(rr);  // Lisätään ensin tyhjä rivi
            rr.lisaa( laji.nimi() , 3);
            rr.lihavoi( true );
            kirjoittaja.lisaaRivi( rr );
        }

        RaporttiRivi rivi;

        QDate pvm = kysely.value("pvm").toDate();
        rivi.lisaa( pvm );

        rivi.lisaaLinkilla( RaporttiRiviSarake::TOSITE_ID, kysely.value("tositeId").toInt() ,
                          QString("%1%2/%3").arg(kysely.value("tositelaji").toString()).arg(kysely.value("tunniste").toInt())
                          .arg( kp()->tilikaudet()->tilikausiPaivalle(pvm).kausitunnus() ));
        rivi.lisaaLinkilla( RaporttiRiviSarake::TILI_NRO, kysely.value("tilinro").toInt() , tr("%1 %2").arg(kysely.value("tilinro").toString()).arg(kysely.value("tilinimi").toString()));

        if( tulostakohdennukset )
        {
            // Kohdennussarake
            if( kysely.value("kohdennusId").toInt() )
                rivi.lisaa( kysely.value("kohdennus").toString());
            else
                rivi.lisaa("");
        }

        qlonglong debetSnt = kysely.value("debetsnt").toLongLong();
        qlonglong kreditSnt = kysely.value("kreditsnt").toLongLong();

        rivi.lisaa( kysely.value("selite").toString());
        rivi.lisaa(  debetSnt );
        rivi.lisaa( kreditSnt );
        kirjoittaja.lisaaRivi( rivi );

        if(  tulostasummat )
        {
            debetYht += debetSnt;
            debetKaikki += debetSnt;
            kreditYht += kreditSnt;
            kreditKaikki += kreditSnt;
        }
    }

    if( ryhmitalajeittain && tulostasummat )
        kirjoitaSummaRivi(kirjoittaja, debetYht, kreditYht, 4 + (int)  tulostakohdennukset );

    if(  tulostasummat )
    {
        // Lopuksi vielä kaikki yhteensä -summarivi
        kirjoittaja.lisaaRivi();
        RaporttiRivi summarivi;
        summarivi.lisaa("Yhteensä", 4 + (int) tulostakohdennukset);
        summarivi.lisaa(debetKaikki);
        summarivi.lisaa(kreditKaikki);
        summarivi.viivaYlle();
        summarivi.lihavoi();
        kirjoittaja.lisaaRivi( summarivi );
    }

    return kirjoittaja;
}

void PaivakirjaRaportti::kirjoitaSummaRivi(RaportinKirjoittaja &rk, int debet, int kredit, int sarakeleveys)
{
    RaporttiRivi rivi;
    rivi.lisaa("Yhteensä", sarakeleveys );
    rivi.lisaa( debet );
    rivi.lisaa( kredit );
    rivi.viivaYlle(true);
    rk.lisaaRivi(rivi);
}
