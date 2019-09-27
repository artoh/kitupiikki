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

#include "paivakirja.h"

#include "naytin/naytinikkuna.h"

#include <QDebug>
#include <QSqlError>

PaivakirjaRaportti::PaivakirjaRaportti()
    : Raportti(nullptr)
{
    ui = new Ui::Paivakirja;
    ui->setupUi( raporttiWidget );

    Tilikausi nykykausi = Kirjanpito::db()->tilikausiPaivalle( Kirjanpito::db()->paivamaara() );
    if( !nykykausi.alkaa().isValid())
        nykykausi = kp()->tilikaudet()->tilikausiIndeksilla( kp()->tilikaudet()->rowCount(QModelIndex()) - 1 );


    ui->alkupvm->setDate(nykykausi.alkaa());
    ui->loppupvm->setDate(nykykausi.paattyy());

    if( kp()->kohdennukset()->kohdennuksia())
    {
        ui->kohdennusCombo->setModel( kp()->kohdennukset());
        ui->kohdennusCombo->setModelColumn( KohdennusModel::NIMI);
    }
    else
    {
        ui->kohdennusCheck->setVisible(false);
        ui->kohdennusCombo->setVisible(false);
    }

    ui->tiliBox->hide();
    ui->tiliCombo->hide();

}

PaivakirjaRaportti::~PaivakirjaRaportti()
{
    delete ui;
}


RaportinKirjoittaja PaivakirjaRaportti::raportti()
{
    int kohdennuksella = -1;
    if( ui->kohdennusCheck->isChecked())
        kohdennuksella = ui->kohdennusCombo->currentData( KohdennusModel::IdRooli).toInt();

    return kirjoitaRaportti( ui->alkupvm->date(), ui->loppupvm->date(),
                             kohdennuksella, ui->tositejarjestysRadio->isChecked(),
                             ui->ryhmittelelajeittainCheck->isChecked(), ui->tulostakohdennuksetCheck->isChecked(),
                             ui->tulostasummat->isChecked());

}

RaportinKirjoittaja PaivakirjaRaportti::kirjoitaRaportti(QDate mista, QDate mihin, int kohdennuksella, bool tositejarjestys, bool ryhmitalajeittain, bool tulostakohdennukset, bool tulostasummat)
{

    RaportinKirjoittaja kirjoittaja;

    if( kohdennuksella > -1 )
        // Tulostetaan vain yhdestä kohdennuksesta
        kirjoittaja.asetaOtsikko( QString("PÄIVÄKIRJA (%1)").arg( kp()->kohdennukset()->kohdennus(kohdennuksella).nimi() ) );
    else
        kirjoittaja.asetaOtsikko("PÄIVÄKIRJA");


    kirjoittaja.asetaKausiteksti(QString("%1 - %2").arg( mista.toString("dd.MM.yyyy") )
                                             .arg( mihin.toString("dd.MM.yyyy") ) );

    kirjoittaja.lisaaPvmSarake();
    kirjoittaja.lisaaSarake("ABC1234/99 ");
    kirjoittaja.lisaaSarake("999999 Tilinimi tarkeinteilla");
    if(tulostakohdennukset )
        kirjoittaja.lisaaSarake("Kohdennusnimi");
    kirjoittaja.lisaaVenyvaSarake();
    kirjoittaja.lisaaEurosarake();
    kirjoittaja.lisaaEurosarake();

    {
        RaporttiRivi otsikko(RaporttiRivi::EICSV);
        otsikko.lisaa("Pvm");
        otsikko.lisaa("Tosite");
        otsikko.lisaa("Tili");
        if( tulostakohdennukset )
            otsikko.lisaa("Kohdennus");
        otsikko.lisaa("Selite");
        otsikko.lisaa("Debet €", 1, true);
        otsikko.lisaa("Kredit €", 1, true);
        kirjoittaja.lisaaOtsake(otsikko);
    }
    {
        // CSV-tiedostoon tulostetaan myös arvonlisäveron kentät
        RaporttiRivi otsikko(RaporttiRivi::CSV);
        otsikko.lisaa("Pvm");
        otsikko.lisaa("Tosite");
        otsikko.lisaa("Tili");
        if( tulostakohdennukset )
            otsikko.lisaa("Kohdennus");
        otsikko.lisaa("Selite");
        otsikko.lisaa("Debet €", 1, true);
        otsikko.lisaa("Kredit €", 1, true);
        otsikko.lisaa("Alv%", 1, true);
        otsikko.lisaa("Alvkoodi", 1, true);
        kirjoittaja.lisaaOtsake(otsikko);

    }


    QSqlQuery kysely;
    QString jarjestys = "vienti.pvm, vientiId";
    if(  tositejarjestys )
        jarjestys = " tositelajiId, tunniste, vientiId";
    else if( ryhmitalajeittain )
        jarjestys = " tositelajiId, vienti.pvm, vientiId";

    QString kysymys;


    kysymys = QString("SELECT vienti.pvm, tili, selite, debetsnt, kreditsnt, vienti.kohdennus, tosite.laji as tositelajiId, tosite.tunniste as tunniste, vienti.id as vientiId, alvkoodi, alvprosentti, tosite.id as tositeId " );

    if( kohdennuksella > -1)
    {


        if( kp()->kohdennukset()->kohdennus(kohdennuksella).tyyppi() == Kohdennus::MERKKAUS)
            kysymys.append( QString(" FROM merkkaus, vienti, tosite WHERE merkkaus.kohdennus=%4 AND vienti.pvm BETWEEN '%1' AND '%2' AND vienti.tosite=tosite.id ORDER BY %3")
                              .arg(mista.toString(Qt::ISODate) )
                              .arg( mihin.toString(Qt::ISODate))
                              .arg(jarjestys).arg(kohdennuksella));
        else
            kysymys.append(QString("FROM vienti,tosite "
                              "WHERE vienti.pvm BETWEEN \"%1\" AND \"%2\" AND vienti.tosite=tosite.id AND kohdennus=%4 ORDER BY %3")
                              .arg(mista.toString(Qt::ISODate) )
                              .arg( mihin.toString(Qt::ISODate))
                              .arg(jarjestys).arg(kohdennuksella));
    }
    else
        kysymys.append(QString("FROM vienti, tosite "
                              "WHERE vienti.pvm BETWEEN \"%1\" AND \"%2\" AND vienti.tosite=tosite.id ORDER BY %3")
                              .arg(mista.toString(Qt::ISODate) )
                              .arg( mihin.toString(Qt::ISODate))
                              .arg(jarjestys));

    qDebug() << kysymys;

    if(!kysely.exec(kysymys))
        kp()->lokiin(kysely);

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
                    kirjoitaSummaRivi( kirjoittaja, debetYht, kreditYht, 4 + (tulostakohdennukset ? 1 : 0) );
                    debetYht = 0;
                    kreditYht = 0;
                }
                kirjoittaja.lisaaRivi();
            }

            // Ryhmittely tositelajeittain: Tulostetaan tositelajien otsikot
            edellinenTositelajiId = kysely.value("tositelajiId").toInt();
            RaporttiRivi rr;
            kirjoittaja.lisaaRivi(rr);  // Lisätään ensin tyhjä rivi
            rr.lihavoi( true );
            kirjoittaja.lisaaRivi( rr );
        }

        RaporttiRivi rivi(RaporttiRivi::EICSV);
        RaporttiRivi csvRivi(RaporttiRivi::CSV);

        QDate pvm = kysely.value("vienti.pvm").toDate();
        rivi.lisaa( pvm );
        csvRivi.lisaa(pvm);

/*
        rivi.lisaaLinkilla( RaporttiRiviSarake::TOSITE_ID, kysely.value("tositeId").toInt() ,
                          QString("%1%2/%3").arg( laji.tunnus() ).arg(kysely.value("tunniste").toInt())
                          .arg( kp()->tilikaudet()->tilikausiPaivalle(pvm).kausitunnus() ));

        csvRivi.lisaaLinkilla( RaporttiRiviSarake::TOSITE_ID, kysely.value("tositeId").toInt() ,
                          QString("%1%2/%3").arg( laji.tunnus() ).arg(kysely.value("tunniste").toInt())
                          .arg( kp()->tilikaudet()->tilikausiPaivalle(pvm).kausitunnus() ));
*/
        Tili tili = kp()->tilit()->tiliIdllaVanha( kysely.value("tili").toInt() );
        if( !tili.onkoValidi())
            continue;   // Maksuperusteisen laskun valvontarivi

        rivi.lisaaLinkilla( RaporttiRiviSarake::TILI_NRO, tili.numero() , tr("%1 %2").arg( tili.numero() ).arg( tili.nimi() ) );
        csvRivi.lisaaLinkilla( RaporttiRiviSarake::TILI_NRO, tili.numero() , tr("%1 %2").arg( tili.numero() ).arg( tili.nimi() ) );

        if( tulostakohdennukset )
        {
            // Kohdennussarake
            if( kysely.value("vienti.kohdennus").toInt() )
            {
                Kohdennus kohdennus = kp()->kohdennukset()->kohdennus( kysely.value("kohdennus").toInt() );
                rivi.lisaa( kohdennus.nimi() );
                csvRivi.lisaa( kohdennus.nimi() );
            }
            else
            {
                rivi.lisaa("");
                csvRivi.lisaa("");
            }
        }

        qlonglong debetSnt = kysely.value("debetsnt").toLongLong();
        qlonglong kreditSnt = kysely.value("kreditsnt").toLongLong();

        rivi.lisaa( kysely.value("selite").toString());
        rivi.lisaa(  debetSnt );
        rivi.lisaa( kreditSnt );
        kirjoittaja.lisaaRivi( rivi );

        csvRivi.lisaa( kysely.value("selite").toString());
        csvRivi.lisaa(  debetSnt );
        csvRivi.lisaa( kreditSnt );
        csvRivi.lisaa( kysely.value("alvprosentti").toString());
        csvRivi.lisaa( kysely.value("alvkoodi").toString());
        kirjoittaja.lisaaRivi(csvRivi);


        if(  tulostasummat )
        {
            debetYht += debetSnt;
            debetKaikki += debetSnt;
            kreditYht += kreditSnt;
            kreditKaikki += kreditSnt;
        }
    }

    if( ryhmitalajeittain && tulostasummat )
        kirjoitaSummaRivi(kirjoittaja, debetYht, kreditYht, 4 + (  tulostakohdennukset ? 1 : 0) );

    if(  tulostasummat )
    {
        // Lopuksi vielä kaikki yhteensä -summarivi
        kirjoittaja.lisaaRivi();
        RaporttiRivi summarivi(RaporttiRivi::EICSV);
        summarivi.lisaa("Yhteensä", 4 + ( tulostakohdennukset ? 1 : 0 ) );
        summarivi.lisaa(debetKaikki);
        summarivi.lisaa(kreditKaikki);
        summarivi.viivaYlle();
        summarivi.lihavoi();
        kirjoittaja.lisaaRivi( summarivi );
    }

    if( kirjoittaja.tyhja())
        kirjoittaja.lisaaRivi();

    return kirjoittaja;
}


void PaivakirjaRaportti::kirjoitaSummaRivi(RaportinKirjoittaja &rk, qlonglong debet, qlonglong kredit, int sarakeleveys)
{
    RaporttiRivi rivi(RaporttiRivi::EICSV);
    rivi.lisaa("Yhteensä", sarakeleveys );
    rivi.lisaa( debet );
    rivi.lisaa( kredit );
    rivi.viivaYlle(true);
    rk.lisaaRivi(rivi);
}

void PaivakirjaRaportti::esikatsele()
{
    int kohdennuksella = -1;
    if( ui->kohdennusCheck->isChecked())
        kohdennuksella = ui->kohdennusCombo->currentData( KohdennusModel::IdRooli).toInt();
    int optiot = 0;

    if( ui->tositejarjestysRadio->isChecked() )
        optiot |= Paivakirja::TositeJarjestyksessa;
    if( ui->ryhmittelelajeittainCheck->isChecked() )
        optiot |= Paivakirja::RyhmitteleLajeittain;
    if( ui->tulostakohdennuksetCheck->isChecked() )
        optiot |= Paivakirja::TulostaKohdennukset;
    if( ui->tulostasummat->isChecked() )
        optiot |= Paivakirja::TulostaSummat;

    Paivakirja *kirja = new Paivakirja(this);
    connect( kirja, &Paivakirja::valmis, this, &Raportti::nayta );
    kirja->kirjoita( ui->alkupvm->date(), ui->loppupvm->date(),
                     optiot, kohdennuksella);
}


