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

#include "paakirjaraportti.h"

#include "db/kirjanpito.h"
#include "db/tilikausi.h"

#include "raportinkirjoittaja.h"

#include "paakirja.h"

PaakirjaRaportti::PaakirjaRaportti()
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


    ui->jarjestysRyhma->hide();
    ui->ryhmittelelajeittainCheck->hide();
    ui->tulostaviennitCheck->hide();

    connect( ui->alkupvm, &QDateEdit::dateChanged, this, &PaakirjaRaportti::haeTilitComboon);
    connect( ui->loppupvm, &QDateEdit::dateChanged, this, &PaakirjaRaportti::haeTilitComboon);
    haeTilitComboon();

}

RaportinKirjoittaja PaakirjaRaportti::raportti()
{
    int kohdennuksella = -1;
    if( ui->kohdennusCheck->isChecked())
        kohdennuksella = ui->kohdennusCombo->currentData( KohdennusModel::IdRooli).toInt();
    int tililta = 0;
    if( ui->tiliBox->isChecked())
        tililta = ui->tiliCombo->currentData().toInt();

    return kirjoitaRaportti(ui->alkupvm->date(), ui->loppupvm->date(), kohdennuksella,
                            ui->tulostakohdennuksetCheck->isChecked(),
                            ui->tulostasummat->isChecked(),
                            tililta);
}

RaportinKirjoittaja PaakirjaRaportti::kirjoitaRaportti(QDate mista, QDate mihin, int kohdennuksella, bool tulostakohdennus, bool tulostaSummarivi, int tililta)
{
    RaportinKirjoittaja rk;

    Kohdennus kohdennus = kp()->kohdennukset()->kohdennus(kohdennuksella);

    if( kohdennuksella > -1 )
        // Tulostetaan vain yhdestä kohdennuksesta
        rk.asetaOtsikko( tr("PÄÄKIRJAN OTE \n%1").arg( kohdennus.nimi()));
    else if( tililta)
        rk.asetaOtsikko( tr("PÄÄKIRJAN OTE"));
    else
        rk.asetaOtsikko( "PÄÄKIRJA");

    rk.asetaKausiteksti(QString("%1 - %2").arg( mista.toString("dd.MM.yyyy") )
                                             .arg( mihin.toString("dd.MM.yyyy") ) );

    rk.lisaaPvmSarake();        // Pvm
    rk.lisaaSarake("ABC1234/99 "); // Tosite
    rk.lisaaVenyvaSarake();     // Selite
    if( tulostakohdennus)
        rk.lisaaSarake("Kohdennusnimi"); // Kohdennus
    rk.lisaaEurosarake();   // Debet
    rk.lisaaEurosarake();   // Kredit
    rk.lisaaEurosarake();   // Saldo

    RaporttiRivi otsikko;
    otsikko.lisaa("Pvm");
    otsikko.lisaa("Tosite");
    otsikko.lisaa("Selite");
    if( tulostakohdennus )
        otsikko.lisaa("Kohdennus");
    otsikko.lisaa("Debet €",1,true);
    otsikko.lisaa("Kredit €",1,true);
    otsikko.lisaa("Saldo €",1, true);
    rk.lisaaOtsake(otsikko);

    // Haetaan ensin alkusaldot
    QMap<int,qlonglong> alkusaldot;   // ysiluku, sentit

    Tilikausi tilikausi = kp()->tilikaudet()->tilikausiPaivalle( mista );

    QString kysymys;
    QSqlQuery kysely;

    // 1) Tasetilit
    if( kohdennuksella > -1)
    {
        if( kohdennus.tyyppi() == Kohdennus::MERKKAUS)
            kysymys = QString("SELECT ysiluku, tyyppi, SUM(debetsnt), SUM(kreditsnt) "
                                 "FROM vienti, tili, merkkaus WHERE vienti.tili=tili.id AND tili.ysiluku < 300000000 AND "
                                 "pvm < \"%1\" AND vienti.id=merkkaus.vienti AND merkkaus.kohdennus=%2 GROUP BY nro").arg(mista.toString(Qt::ISODate)).arg(kohdennuksella);
        else
            kysymys = QString("SELECT ysiluku, tyyppi, SUM(debetsnt), SUM(kreditsnt) "
                                 "FROM vienti, tili WHERE vienti.tili=tili.id AND tili.ysiluku < 300000000 AND "
                                 "pvm < \"%1\" AND vienti.kohdennus=%2 GROUP BY nro").arg(mista.toString(Qt::ISODate)).arg(kohdennuksella);
    }
    else
        kysymys = QString("SELECT ysiluku, tyyppi, SUM(debetsnt), SUM(kreditsnt) "
                             "FROM vienti, tili WHERE vienti.tili=tili.id AND tili.ysiluku < 300000000 AND "
                             "pvm < \"%1\" GROUP BY nro").arg(mista.toString(Qt::ISODate));

    kysely.exec(kysymys);
    while( kysely.next())
    {
        int ysiluku = kysely.value(0).toInt();
        QString tyyppi = kysely.value(1).toString();
        qlonglong debet = kysely.value(2).toLongLong();
        qlonglong kredit = kysely.value(3).toLongLong();

        if( tyyppi.startsWith('A') )
            alkusaldot.insert(ysiluku, debet - kredit);
        else
            alkusaldot.insert(ysiluku, kredit - debet);
    }

    // Lisätään aiempien tilikausien tulos (ei kuitenkaan kohdennusotteelle)
    if( kohdennuksella < 0)
    {
        kysymys = QString("SELECT sum(debetsnt), sum(kreditsnt) FROM vienti, tili "
                          "WHERE vienti.tili=tili.id AND ysiluku > 300000000 AND "
                          "pvm < \"%1\" ").arg(tilikausi.alkaa().toString(Qt::ISODate));
        kysely.exec( kysymys );
        if( kysely.next())
        {
            qlonglong edYlijaama = kysely.value(1).toLongLong() - kysely.value(0).toLongLong();
            int kertymaTiliNro = kp()->tilit()->edellistenYlijaamaTili().ysivertailuluku();
            alkusaldot[kertymaTiliNro] = alkusaldot.value(kertymaTiliNro, 0) + edYlijaama;
        }
    }


    // 2) Tulostilit - jos ei haeta tilikauden alusta saakka
    QDate alkupaiva = tilikausi.alkaa();


    if( alkupaiva != mista )
    {
        if( kohdennuksella > -1 && kohdennus.tyyppi() == Kohdennus::MERKKAUS)
            kysymys = QString("SELECT ysiluku, SUM(debetsnt), SUM(kreditsnt) "
                                     "FROM merkkaus,vienti, tili WHERE merkkaus.kohdennus=%3 AND merkkaus.vienti=vienti.id AND "
                                     "vienti.tili=tili.id AND tili.ysiluku > 300000000 AND "
                                     "pvm BETWEEN \"%1\" AND \"%2\" GROUP BY nro")
                    .arg(alkupaiva.toString(Qt::ISODate))
                    .arg(mista.addDays(-1).toString(Qt::ISODate))
                    .arg(kohdennuksella);
        else if( kohdennuksella > -1)
            kysymys = QString("SELECT ysiluku, SUM(debetsnt), SUM(kreditsnt) "
                                     "FROM vienti, tili WHERE vienti.tili=tili.id AND tili.ysiluku > 300000000 AND "
                                     "pvm BETWEEN \"%1\" AND \"%2\" AND vienti.kohdennus=%3 GROUP BY nro")
                    .arg(alkupaiva.toString(Qt::ISODate))
                    .arg(mista.addDays(-1).toString(Qt::ISODate))
                    .arg(kohdennuksella);
        else
            kysymys = QString("SELECT ysiluku, SUM(debetsnt), SUM(kreditsnt) "
                                     "FROM vienti, tili WHERE vienti.tili=tili.id AND tili.ysiluku > 300000000 AND "
                                     "pvm BETWEEN \"%1\" AND \"%2\" GROUP BY nro")
                    .arg(alkupaiva.toString(Qt::ISODate))
                    .arg(mista.addDays(-1).toString(Qt::ISODate));

        kysely.exec(kysymys);
        while( kysely.next())
        {
            int ysiluku = kysely.value(0).toInt();
            qlonglong debet = kysely.value(1).toLongLong();
            qlonglong kredit = kysely.value(2).toLongLong();
            alkusaldot.insert(ysiluku, kredit - debet);
        }
    }


    // Varmistetaan, että kaikilla tämän tilikauden tileillä on tietue
    if( kohdennuksella > -1 && kohdennus.tyyppi() == Kohdennus::MERKKAUS)
        kysymys = QString("SELECT ysiluku FROM merkkaus,vienti, tili WHERE "
                          "merkkaus.kohdennus=%3 AND merkkaus.vienti=vienti.id AND vienti.tili = tili.id AND "
                          "pvm BETWEEN \"%1\" AND \"%2\" GROUP BY nro")
                .arg(mista.toString(Qt::ISODate)).arg(mihin.toString(Qt::ISODate)).arg(kohdennuksella);
    else if( kohdennuksella > -1)
        kysymys = QString("SELECT ysiluku FROM vienti, tili WHERE vienti.tili = tili.id AND "
                          "pvm BETWEEN \"%1\" AND \"%2\" AND vienti.kohdennus=%3 GROUP BY nro")
                .arg(mista.toString(Qt::ISODate)).arg(mihin.toString(Qt::ISODate)).arg(kohdennuksella);
    else
        kysymys = QString("SELECT ysiluku FROM vienti, tili WHERE vienti.tili = tili.id AND "
                      "pvm BETWEEN \"%1\" AND \"%2\" GROUP BY nro")
                .arg(mista.toString(Qt::ISODate)).arg(mihin.toString(Qt::ISODate));

    kysely.exec(kysymys);
    while( kysely.next() )
    {
        if( !alkusaldot.contains( kysely.value(0).toInt()))
            alkusaldot.insert(kysely.value(0).toInt(), 0);
    }

    // Sitten päästäänkin tulostamaan pääkirjaa
    QMapIterator<int,qlonglong> iter( alkusaldot );

    qlonglong kokoDebetYht = 0;
    qlonglong kokoKreditYht = 0;


    while(iter.hasNext())
    {
        iter.next();

        const Tili& tili = kp()->tilit()->tiliYsiluvulla( iter.key() );            

        if( tililta && tili.numero() != tililta)
            continue;

        qlonglong debetYht = 0;
        qlonglong kreditYht = 0;

        RaporttiRivi tiliotsikko;
        tiliotsikko.lisaaLinkilla( RaporttiRiviSarake::TILI_LINKKI, tili.numero(),  QString("%1 %2").arg(tili.numero()).arg( tili.nimi()) , 5 + (int) tulostakohdennus );
        tiliotsikko.lisaa( iter.value());
        tiliotsikko.lihavoi();
        rk.lisaaRivi( tiliotsikko);

        qlonglong saldo = iter.value();
        QString kysymys;

        if( kohdennuksella > -1 && kohdennus.tyyppi() == Kohdennus::MERKKAUS)
            kysymys = QString("SELECT pvm, tositelaji, tunniste, kohdennusId, tositeId, "
                                      "vientivw.kohdennus as kohdennusnimi, selite, debetsnt, kreditsnt FROM merkkaus, vientivw "
                                      "WHERE merkkaus.kohdennus=%4 AND merkkaus.vienti=vientiId AND tilinro=%1 AND pvm BETWEEN \"%2\" AND \"%3\" "
                                      "ORDER BY pvm, vientiId ")
                    .arg(tili.numero()).arg(mista.toString(Qt::ISODate)).arg(mihin.toString(Qt::ISODate)).arg(kohdennuksella);
        else if( kohdennuksella > -1)
            kysymys = QString("SELECT pvm, tositelaji, tunniste, kohdennusId, tositeId, "
                                      "kohdennus as kohdennusnimi, selite, debetsnt, kreditsnt FROM vientivw "
                                      "WHERE tilinro=%1 AND pvm BETWEEN \"%2\" AND \"%3\" AND kohdennusId=%4 "
                                      "ORDER BY pvm, vientiId ")
                    .arg(tili.numero()).arg(mista.toString(Qt::ISODate)).arg(mihin.toString(Qt::ISODate)).arg(kohdennuksella);
        else
            kysymys = QString("SELECT pvm, tositelaji, tunniste, kohdennusId, tositeId, "
                                      "kohdennus as kohdennusnimi, selite, debetsnt, kreditsnt FROM vientivw "
                                      "WHERE tilinro=%1 AND pvm BETWEEN \"%2\" AND \"%3\" "
                                      "ORDER BY pvm, vientiId ")
                    .arg(tili.numero()).arg(mista.toString(Qt::ISODate)).arg(mihin.toString(Qt::ISODate));


        kysely.exec(kysymys);

        while(kysely.next())
        {
            qlonglong debet = kysely.value("debetsnt").toLongLong();
            qlonglong kredit = kysely.value("kreditsnt").toLongLong();

            debetYht += debet;
            kreditYht += kredit;

            if( tili.onko(TiliLaji::VASTAAVAA) )
                saldo += debet - kredit;
            else
                saldo += kredit - debet;

            RaporttiRivi rr;
            QDate pvm = kysely.value("pvm").toDate();
            rr.lisaa( pvm );
            rr.lisaaLinkilla( RaporttiRiviSarake::TOSITE_ID, kysely.value("tositeId").toInt() ,
                              QString("%1%2/%3").arg(kysely.value("tositelaji").toString()).arg(kysely.value("tunniste").toInt())
                              .arg( kp()->tilikaudet()->tilikausiPaivalle(pvm).kausitunnus() ));
            rr.lisaa( kysely.value("selite").toString());
            if( tulostakohdennus)
            {
                if( kysely.value("kohdennusId").toInt())
                    rr.lisaa( kysely.value("kohdennusnimi").toString());
                else
                    rr.lisaa("");   // Ei kohdenneta-tekstiä ei tulosteta
            }
            rr.lisaa( debet );
            rr.lisaa( kredit );
            rr.lisaa( saldo, true);
            rk.lisaaRivi( rr);
        }

        kokoDebetYht += debetYht;
        kokoKreditYht += kreditYht;

        if( tulostaSummarivi && (debetYht || kreditYht))
        {
            RaporttiRivi summarivi;
            qlonglong erotus = kreditYht - debetYht;
            if( tili.onko(TiliLaji::TASE))
                erotus = debetYht - kreditYht;

            summarivi.lisaa("", 2 +static_cast<int>(tulostakohdennus));
            summarivi.lisaa( erotus , false, true);
            summarivi.lisaa( debetYht );
            summarivi.lisaa( kreditYht );
            summarivi.lisaa( saldo );
            summarivi.viivaYlle();
            summarivi.lihavoi();
            rk.lisaaRivi(summarivi);
        }

        rk.lisaaRivi(); // Tyhjä rivi tilien väliin

    }

    if( tulostaSummarivi && !tililta )
    {
        RaporttiRivi summarivi;
        summarivi.lisaa("Yhteensä", 2 +  static_cast<int>(tulostakohdennus)  );
        qlonglong erotus = kokoKreditYht - kokoDebetYht;

        summarivi.lisaa( erotus , false, true);

        summarivi.lisaa( kokoDebetYht );
        summarivi.lisaa( kokoKreditYht);
        summarivi.lihavoi();
        summarivi.viivaYlle();
        rk.lisaaRivi(summarivi);
    }

    return rk;

}

void PaakirjaRaportti::haeTilitComboon()
{
    QString kysymys = QString(R"(SELECT DISTINCT tili FROM vienti WHERE pvm BETWEEN "%1" AND "%2")").arg(ui->alkupvm->date().toString(Qt::ISODate)).arg(ui->loppupvm->date().toString(Qt::ISODate));
    QSqlQuery kysely(kysymys);
    ui->tiliCombo->clear();

    while( kysely.next())
    {
        const Tili& tili = kp()->tilit()->tiliIdllaVanha( kysely.value(0).toInt());
        ui->tiliCombo->addItem( QString("%1 %2").arg(tili.numero()).arg(tili.nimi()), tili.numero() );
    }
    ui->tiliCombo->model()->sort(0);
}

void PaakirjaRaportti::esikatsele()
{
    Paakirja *kirja = new Paakirja(this);
    connect( kirja, &Paakirja::valmis, this, &Raportti::nayta);

    int kohdennuksella = -1;
    if( ui->kohdennusCheck->isChecked())
        kohdennuksella = ui->kohdennusCombo->currentData( KohdennusModel::IdRooli).toInt();
    int tililta = 0;
    if( ui->tiliBox->isChecked())
        tililta = ui->tiliCombo->currentData().toInt();

    kirja->kirjoita(ui->alkupvm->date(), ui->loppupvm->date(), kohdennuksella,
                            ui->tulostakohdennuksetCheck->isChecked(),
                            ui->tulostasummat->isChecked(),
                            tililta);
}
