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

PaakirjaRaportti::PaakirjaRaportti(QPrinter *printer)
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

RaportinKirjoittaja PaakirjaRaportti::raportti()
{
    return kirjoitaRaportti(ui->alkupvm->date(), ui->loppupvm->date(),
                            ui->tulostakohdennuksetCheck->isChecked());
}

RaportinKirjoittaja PaakirjaRaportti::kirjoitaRaportti(QDate mista, QDate mihin, bool tulostakohdennus)
{
    RaportinKirjoittaja rk;

    rk.asetaOtsikko( "PÄÄKIRJA");

    rk.asetaKausiteksti(QString("%1 - %2").arg( mista.toString(Qt::SystemLocaleShortDate) )
                                             .arg( mihin.toString(Qt::SystemLocaleShortDate) ) );

    rk.lisaaPvmSarake();        // Pvm
    rk.lisaaSarake("ABC1234 "); // Tosite
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
    QMap<int,int> alkusaldot;   // tilino, sentit

    // 1) Tasetilit
    QString kysymys = QString("SELECT nro, tyyppi, SUM(debetsnt), SUM(kreditsnt) "
                             "FROM vienti, tili WHERE vienti.tili=tili.id AND tili.ysiluku < 300000000 AND "
                             "pvm < \"%1\" GROUP BY nro").arg(mista.toString(Qt::ISODate));
    QSqlQuery kysely(kysymys);
    while( kysely.next())
    {
        int tilinro = kysely.value(0).toInt();
        QString tyyppi = kysely.value(1).toString();
        int debet = kysely.value(2).toInt();
        int kredit = kysely.value(3).toInt();

        if( tyyppi.startsWith('A') )
            alkusaldot.insert(tilinro, debet - kredit);
        else
            alkusaldot.insert(tilinro, kredit - debet);
    }

    // Lisätään aiempien tilikausien tulos
    Tilikausi tilikausi = kp()->tilikaudet()->tilikausiPaivalle( mista );
    kysymys = QString("SELECT sum(debetsnt), sum(kreditsnt) FROM vienti, tili "
                      "WHERE vienti.tili=tili.id AND ysiluku > 300000000 AND "
                      "pvm < \"%1\" ").arg(tilikausi.alkaa().toString(Qt::ISODate));
    kysely.exec( kysymys );
    if( kysely.next())
    {
        int edYlijaama = kysely.value(1).toInt() - kysely.value(0).toInt();
        int kertymaTiliNro = kp()->tilit()->edellistenYlijaamaTili().numero();
        alkusaldot[kertymaTiliNro] = alkusaldot.value(kertymaTiliNro, 0) + edYlijaama;
    }

    // 2) Tulostilit - jos ei haeta tilikauden alusta saakka
    if( tilikausi.alkaa() != mista )
    {
        kysymys = QString("SELECT nro, SUM(debetsnt), SUM(kreditsnt) "
                                 "FROM vienti, tili WHERE vienti.tili=tili.id AND tili.ysiluku > 300000000 AND "
                                 "pvm BETWEEN \"%1\" AND \"%2\" GROUP BY nro")
                .arg(tilikausi.alkaa().toString(Qt::ISODate))
                .arg(mista.toString(Qt::ISODate));

        kysely.exec(kysymys);
        while( kysely.next())
        {
            int tilinro = kysely.value(0).toInt();
            int debet = kysely.value(1).toInt();
            int kredit = kysely.value(2).toInt();
            alkusaldot.insert(tilinro, kredit - debet);
        }
    }


    // Varmistetaan, että kaikilla tämän tilikauden tileillä on tietue
    kysymys = QString("SELECT nro FROM vienti, tili WHERE vienti.tili = tili.id AND "
                      "pvm BETWEEN \"%1\" AND \"%2\" GROUP BY nro")
            .arg(mista.toString(Qt::ISODate)).arg(mihin.toString(Qt::ISODate));

    kysely.exec(kysymys);
    while( kysely.next() )
    {
        if( !alkusaldot.contains( kysely.value(0).toInt()))
            alkusaldot.insert(kysely.value(0).toInt(), 0);
    }

    // Sitten päästäänkin tulostamaan pääkirjaa
    QMapIterator<int,int> iter( alkusaldot );
    int debetYht = 0;
    int kreditYht = 0;

    while(iter.hasNext())
    {
        iter.next();
        Tili tili = kp()->tilit()->tiliNumerolla( iter.key() );

        RaporttiRivi tiliotsikko;
        tiliotsikko.lisaa( QString("%1 %2").arg(tili.numero()).arg( tili.nimi()) , 5 + (int) tulostakohdennus );
        tiliotsikko.lisaa( iter.value());
        tiliotsikko.lihavoi();
        rk.lisaaRivi( tiliotsikko);

        int saldo = iter.value();

        QString kysymys = QString("SELECT pvm, tositelaji, tunniste, kohdennusId, "
                                  "kohdennus, selite, debetsnt, kreditsnt FROM vientivw "
                                  "WHERE tilinro=%1 AND pvm BETWEEN \"%2\" AND \"%3\" "
                                  "ORDER BY pvm, vientiId ")
                .arg(tili.numero()).arg(mista.toString(Qt::ISODate)).arg(mihin.toString(Qt::ISODate));
        kysely.exec(kysymys);

        while(kysely.next())
        {
            int debet = kysely.value("debetsnt").toInt();
            int kredit = kysely.value("kreditsnt").toInt();

            debetYht += debet;
            kreditYht += kredit;

            if( tili.onkoVastaavaaTili())
                saldo += debet - kredit;
            else
                saldo += kredit - debet;

            RaporttiRivi rr;
            rr.lisaa( kysely.value("pvm").toDate() );
            rr.lisaa( QString("%1%2").arg(kysely.value("tositelaji").toString()).arg(kysely.value("tunniste").toInt())  );
            rr.lisaa( kysely.value("selite").toString());
            if( tulostakohdennus)
            {
                if( kysely.value("kohdennusId").toInt())
                    rr.lisaa( kysely.value("kohdennus").toString());
                else
                    rr.lisaa("");   // Ei kohdenneta-tekstiä ei tulosteta
            }
            rr.lisaa( debet );
            rr.lisaa( kredit );
            rr.lisaa( saldo, true);
            rk.lisaaRivi( rr);
        }

        rk.lisaaRivi(); // Tyhjä rivi tilien väliin

    }

    RaporttiRivi summarivi;
    summarivi.lisaa("Yhteensä", 3 + (int) tulostakohdennus  );
    summarivi.lisaa( debetYht );
    summarivi.lisaa( kreditYht);
    summarivi.lihavoi();
    summarivi.viivaYlle();
    rk.lisaaRivi(summarivi);

    return rk;

}
