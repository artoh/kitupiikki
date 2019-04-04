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
#include "db/kirjanpito.h"

#include "tositeluetteloraportti.h"

TositeluetteloRaportti::TositeluetteloRaportti()
    : Raportti(nullptr)
{
    ui = new Ui::Paivakirja;
    ui->setupUi(raporttiWidget);

    Tilikausi nykykausi = Kirjanpito::db()->tilikausiPaivalle( Kirjanpito::db()->paivamaara() );
    if( !nykykausi.alkaa().isValid())
        nykykausi = kp()->tilikaudet()->tilikausiIndeksilla( kp()->tilikaudet()->rowCount(QModelIndex()) - 1 );

    ui->alkupvm->setDate(nykykausi.alkaa());
    ui->loppupvm->setDate(nykykausi.paattyy());

    ui->kohdennusCheck->hide();
    ui->kohdennusCombo->hide();
    ui->tiliBox->hide();
    ui->tiliCombo->hide();

    ui->ryhmittelelajeittainCheck->setChecked(true);
    ui->tositejarjestysRadio->setChecked(true);
    ui->tulostakohdennuksetCheck->setEnabled(false);
    connect( ui->tulostaviennitCheck, SIGNAL(toggled(bool)), ui->tulostakohdennuksetCheck, SLOT(setEnabled(bool)));


}

RaportinKirjoittaja TositeluetteloRaportti::raportti()
{
    return kirjoitaRaportti( ui->alkupvm->date(), ui->loppupvm->date(),
                             ui->tositejarjestysRadio->isChecked(),
                             ui->ryhmittelelajeittainCheck->isChecked() ,
                             ui->tulostakohdennuksetCheck->isChecked() && ui->tulostaviennitCheck->isChecked(),
                             ui->tulostaviennitCheck->isChecked(),
                             ui->tulostasummat->isChecked() );
}

RaportinKirjoittaja TositeluetteloRaportti::kirjoitaRaportti(QDate mista, QDate mihin, bool tositejarjestys, bool ryhmittelelajeittain, bool tulostakohdennukset, bool tulostaviennit, bool tulostasummat)
{
    RaportinKirjoittaja kirjoittaja;

    if( tulostaviennit)
        kirjoittaja.asetaOtsikko("TOSITEPÄIVÄKIRJA");
    else
        kirjoittaja.asetaOtsikko("TOSITELUETTELO");

    kirjoittaja.asetaKausiteksti(QString("%1 - %2").arg( mista.toString("dd.MM.yyyy") )
                                             .arg( mihin.toString("dd.MM.yyyy") ) );

    kirjoittaja.lisaaSarake("ABC1234/99 ");    // Tositetunniste
    kirjoittaja.lisaaPvmSarake();           // Pvm
    kirjoittaja.lisaaSarake("999999 Tilinimi tarkenteilla");
    if( tulostakohdennukset )
        kirjoittaja.lisaaSarake("Kohdennusnimi ");
    kirjoittaja.lisaaVenyvaSarake();    // Selite
    kirjoittaja.lisaaEurosarake();      // Debet
    kirjoittaja.lisaaEurosarake();      // Kredit

    RaporttiRivi tositeOtsikko;
    tositeOtsikko.lisaa("Tosite");
    tositeOtsikko.lisaa("Pvm");
    tositeOtsikko.lisaa("Otsikko", 2 + (int) tulostakohdennukset);
    tositeOtsikko.lisaa("Liitteitä");
    if( !tulostaviennit)
        tositeOtsikko.lisaa("Summa €", 1, true);
    kirjoittaja.lisaaOtsake(tositeOtsikko);

    if( tulostaviennit )
    {
        RaporttiRivi vientiOtsikko;
        vientiOtsikko.lisaa(" ");
        vientiOtsikko.lisaa("Pvm");
        vientiOtsikko.lisaa("Tili");
        if( tulostakohdennukset)
            vientiOtsikko.lisaa("Kohdennus");
        vientiOtsikko.lisaa("Selite");
        vientiOtsikko.lisaa("Debet €", 1, true);
        vientiOtsikko.lisaa("Kredit €", 1, true);
        kirjoittaja.lisaaOtsake(vientiOtsikko);
    }

    // Sitten kysellään

    QString jarjestys = "pvm, id";
    if( tositejarjestys )
        jarjestys = "laji, tunniste";
    else if( ryhmittelelajeittain )
        jarjestys = "laji, pvm, id";

    QString kysymys = QString("SELECT id, pvm, otsikko, tunniste, laji FROM tosite "
                              "WHERE pvm BETWEEN \"%1\" AND \"%2\" ORDER BY %3")
            .arg(mista.toString(Qt::ISODate))
            .arg(mihin.toString(Qt::ISODate))
            .arg(jarjestys);

    QSqlQuery kysely(kysymys);

    int edellinenTositelajiId = -1;
    qlonglong debetYht = 0;
    qlonglong kreditYht = 0;
    qlonglong debetKaikki = 0;
    qlonglong kreditKaikki = 0;

    while( kysely.next() )
    {

        int tositeId = kysely.value("id").toInt();
        QDate tositePvm = kysely.value("pvm").toDate();
        QString otsikko = kysely.value("otsikko").toString();
        int tunniste = kysely.value("tunniste").toInt();
        Tositelaji laji = kp()->tositelajit()->tositelajiVanha( kysely.value("laji").toInt());

        if( ryhmittelelajeittain && edellinenTositelajiId != laji.id())
        {
            if( edellinenTositelajiId > -1 && tulostasummat)
            {
                kirjoitaSummaRivi(kirjoittaja, debetYht, kreditYht, 4 + (int) tulostakohdennukset );
                debetYht = 0;
                kreditYht = 0;
            }


            // Ryhmitellään
            edellinenTositelajiId = laji.id();
            RaporttiRivi rr(RaporttiRivi::EICSV);
            kirjoittaja.lisaaRivi();    // Tyhjä
            rr.lisaa( laji.nimi(), 4);
            rr.lihavoi();
            kirjoittaja.lisaaRivi( rr );
        }


        qlonglong debetSumma = 0;
        qlonglong kreditSumma = 0;
        qlonglong summa = 0;
        int liitteita = 0;

        // Tässä välissä tositelajikohtaisia toimia...

        QSqlQuery lisakysely( QString("SELECT SUM(debetsnt), SUM(kreditsnt) FROM vienti WHERE tosite=%1 ").arg(tositeId));
        if( lisakysely.next())
        {
            // Tositteen summa: debet ja kredit yleensä yhtä suuret :)
            debetSumma = lisakysely.value(0).toLongLong();
            kreditSumma = lisakysely.value(1).toLongLong();
            if( kreditSumma > debetSumma)
                summa = kreditSumma;
            else
                summa = debetSumma;

            if( tulostaviennit )
            {
                debetYht += debetSumma;
                debetKaikki += debetSumma;

                kreditYht += kreditSumma;
                kreditKaikki += kreditSumma;
            }
            else
            {
                kreditYht += summa;
                kreditKaikki += summa;
            }

        }

        lisakysely.exec( QString("SELECT COUNT(liiteno) FROM liite WHERE tosite=%1").arg(tositeId));
        if( lisakysely.next())
        {
            liitteita = lisakysely.value(0).toInt();
        }

        RaporttiRivi tositerivi;
        tositerivi.lisaaLinkilla( RaporttiRiviSarake::TOSITE_ID, tositeId,
                                  QString("%1%2/%3").arg(laji.tunnus())
                                  .arg(tunniste).arg( kp()->tilikaudet()->tilikausiPaivalle(tositePvm).kausitunnus() ) );
        tositerivi.lisaa(tositePvm);
        tositerivi.lisaa(otsikko, 2 + (int) tulostakohdennukset );

        if(liitteita)
            tositerivi.lisaa( QString::number(liitteita) + " kpl");
        else
            tositerivi.lisaa(" ");

        if( !tulostaviennit)            // Summat vain, jos vientejä ei tulostettu (jotta selkeämpi)
            tositerivi.lisaa( summa );

        kirjoittaja.lisaaRivi( tositerivi );

        // Vientien haku ja tulostus, jos sellaista halutaan
        if( tulostaviennit)
        {

            lisakysely.exec(QString("SELECT pvm, tili, kohdennus, selite, debetsnt, kreditsnt "
                                    "FROM vienti WHERE tosite=%1 ORDER BY id")
                            .arg(tositeId));
            while( lisakysely.next())
            {
                if( !lisakysely.value("tili").toInt())
                    continue;   // Ei tulosteta maksuperusteisen laskun lisärivejä

                RaporttiRivi vientirivi;
                vientirivi.lisaa("");
                vientirivi.lisaa( lisakysely.value("pvm").toDate() );
                Tili tili = kp()->tilit()->tiliIdllaVanha( lisakysely.value("tili").toInt());
                vientirivi.lisaaLinkilla(RaporttiRiviSarake::TILI_NRO, tili.numero(), QString("%1 %2").arg(tili.numero()).arg(tili.nimi()));

                if( tulostakohdennukset  )
                {
                    if( lisakysely.value("kohdennus").toInt())
                        vientirivi.lisaa( kp()->kohdennukset()->kohdennus( lisakysely.value("kohdennus").toInt()).nimi() );
                    else
                        vientirivi.lisaa(" ");  // Ei kohdennusta
                }

                vientirivi.lisaa( lisakysely.value("selite").toString());
                vientirivi.lisaa( lisakysely.value("debetsnt").toLongLong());
                vientirivi.lisaa( lisakysely.value("kreditsnt").toLongLong());
                kirjoittaja.lisaaRivi( vientirivi );
            }
        }

    }

    if( ryhmittelelajeittain && tulostasummat )
        kirjoitaSummaRivi(kirjoittaja, debetYht, kreditYht, 4 + (int)  tulostakohdennukset );

    if(  tulostasummat )
    {
        // Lopuksi vielä kaikki yhteensä -summarivi
        kirjoittaja.lisaaRivi(RaporttiRivi::EICSV);
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

void TositeluetteloRaportti::kirjoitaSummaRivi(RaportinKirjoittaja &rk, qlonglong debet, qlonglong kredit, int sarakeleveys)
{
    RaporttiRivi rivi(RaporttiRivi::EICSV);
    rivi.lisaa("Yhteensä", sarakeleveys );
    rivi.lisaa( debet );
    rivi.lisaa( kredit );
    rivi.viivaYlle(true);
    rk.lisaaRivi(rivi);
}
