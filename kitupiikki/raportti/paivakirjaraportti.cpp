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

PaivakirjaRaportti::PaivakirjaRaportti(QPrinter *printer)
    : Raportti(printer)
{
    ui = new Ui::Paivakirja;
    ui->setupUi( raporttiWidget );

    Tilikausi nykykausi = Kirjanpito::db()->tilikausiPaivalle( Kirjanpito::db()->paivamaara() );
    ui->alkupvm->setDate(nykykausi.alkaa());
    ui->loppupvm->setDate(nykykausi.paattyy());

    ui->kohdennusCombo->setModel( kp()->kohdennukset());
    ui->kohdennusCombo->setModelColumn( KohdennusModel::NIMI);
}

PaivakirjaRaportti::~PaivakirjaRaportti()
{
    delete ui;
}


RaportinKirjoittaja PaivakirjaRaportti::raportti()
{

    RaportinKirjoittaja kirjoittaja;
    if( ui->kohdennusCheck->isChecked() )
        kirjoittaja.asetaOtsikko( QString("PÄIVÄKIRJA (%1)").arg(ui->kohdennusCombo->currentData(KohdennusModel::NimiRooli).toString()) );
    else
        kirjoittaja.asetaOtsikko("PÄIVÄKIRJA");


    kirjoittaja.asetaKausiteksti(QString("%1 - %2").arg( ui->alkupvm->date().toString(Qt::SystemLocaleShortDate) )
                                             .arg( ui->loppupvm->date().toString(Qt::SystemLocaleShortDate) ) );

    kirjoittaja.lisaaPvmSarake();
    kirjoittaja.lisaaSarake("ABC1234 ");
    kirjoittaja.lisaaSarake("999999 Tilinimi tarkeinteilla");
    if( ui->tulostakohdennuksetCheck->isChecked())
        kirjoittaja.lisaaSarake("Kohdennusnimi");
    kirjoittaja.lisaaVenyvaSarake();
    kirjoittaja.lisaaEurosarake();
    kirjoittaja.lisaaEurosarake();

    RaporttiRivi otsikko;
    otsikko.lisaa("Pvm");
    otsikko.lisaa("Tosite");
    otsikko.lisaa("Tili");
    if( ui->tulostakohdennuksetCheck->isChecked())
        otsikko.lisaa("Kohdennus");
    otsikko.lisaa("Selite");
    otsikko.lisaa("Debet €", 1, true);
    otsikko.lisaa("Kredit €", 1, true);
    kirjoittaja.lisaaOtsake(otsikko);

    QSqlQuery kysely;
    QString jarjestys = "pvm, vientiId";
    if( ui->tositejarjestysRadio->isChecked())
        jarjestys = " tositelaji, tunniste, vientiId";
    else if( ui->ryhmittelelajeittainCheck->isChecked())
        jarjestys = " tositelaji, pvm, vientiId";

    // Kohdennuksen mukaan rajaaminen
    QString lisaehto;
    if( ui->kohdennusCheck->isChecked())
        lisaehto = QString(" AND kohdennusId=%1").arg( ui->kohdennusCombo->currentData(KohdennusModel::IdRooli).toInt());

    QString kysymys = QString("SELECT pvm, tositelaji, tunniste, tilinro, tilinimi, selite, debetsnt, kreditsnt, kohdennus, kohdennusId, tositelajiId from vientivw "
                              "WHERE pvm BETWEEN \"%1\" AND \"%2\" %4 ORDER BY %3")
                              .arg( ui->alkupvm->date().toString(Qt::ISODate) )
                              .arg( ui->loppupvm->date().toString(Qt::ISODate))
                              .arg(jarjestys).arg(lisaehto);

    kysely.exec(kysymys);

    int edellinenTositelajiId = -1;
    int debetYht = 0;
    int kreditYht = 0;
    int debetKaikki = 0;
    int kreditKaikki = 0;

    while( kysely.next())
    {
        if( ui->ryhmittelelajeittainCheck->isChecked() && edellinenTositelajiId != kysely.value("tositelajiId").toInt())
        {
            if( edellinenTositelajiId > -1 )
            {
                if( ui->tulostasummat->isChecked() )
                {
                    kirjoitaSummaRivi( kirjoittaja, debetYht, kreditYht);
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
        rivi.lisaa( kysely.value("pvm").toDate());
        rivi.lisaa( kysely.value("tositelaji").toString() + kysely.value("tunniste").toString());
        rivi.lisaa( tr("%1 %2").arg(kysely.value("tilinro").toString()).arg(kysely.value("tilinimi").toString()));

        if( ui->tulostakohdennuksetCheck->isChecked())
        {
            if( kysely.value("kohdennusId").toInt() )
                rivi.lisaa( kysely.value("kohdennus").toString());
            else
                rivi.lisaa("");
        }

        int debetSnt = kysely.value("debetsnt").toInt();
        int kreditSnt = kysely.value("kreditsnt").toInt();

        rivi.lisaa( kysely.value("selite").toString());
        rivi.lisaa(  debetSnt );
        rivi.lisaa( kreditSnt );
        kirjoittaja.lisaaRivi( rivi );

        if( ui->tulostasummat->isChecked())
        {
            debetYht += debetSnt;
            debetKaikki += debetSnt;
            kreditYht += kreditSnt;
            kreditKaikki += kreditSnt;
        }
    }

    if( ui->ryhmittelelajeittainCheck->isChecked() && ui->tulostasummat->isChecked())
        kirjoitaSummaRivi(kirjoittaja, debetYht, kreditYht);

    if( ui->tulostasummat->isChecked())
    {
        // Lopuksi vielä kaikki yhteensä -summarivi
        kirjoittaja.lisaaRivi();
        RaporttiRivi summarivi;
        summarivi.lisaa("Yhteensä", 4 + (int) ui->tulostakohdennuksetCheck->isChecked() );
        summarivi.lisaa(debetKaikki);
        summarivi.lisaa(kreditKaikki);
        summarivi.viivaYlle();
        summarivi.lihavoi();
        kirjoittaja.lisaaRivi( summarivi );
    }

    return kirjoittaja;

}

void PaivakirjaRaportti::kirjoitaSummaRivi(RaportinKirjoittaja &rk, int debet, int kredit)
{
    RaporttiRivi rivi;
    rivi.lisaa("Yhteensä", 4 + (int) ui->tulostakohdennuksetCheck->isChecked() );
    rivi.lisaa( debet );
    rivi.lisaa( kredit );
    rivi.viivaYlle(true);
    rk.lisaaRivi(rivi);
}
