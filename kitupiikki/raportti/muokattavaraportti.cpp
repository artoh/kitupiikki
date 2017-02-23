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

#include "muokattavaraportti.h"
#include "db/kirjanpito.h"
#include "db/tilikausi.h"

#include <QSqlQuery>

#include <QDebug>

RaporttiData::RaporttiData(QDate alkaaPvm, QDate paattyyPvm)
    : alkaa(alkaaPvm), paattyy(paattyyPvm)
{

}


MuokattavaRaportti::MuokattavaRaportti(const QString &raporttinimi, QPrinter *printer)
    : Raportti(printer), otsikko(raporttinimi)
{
    ui = new Ui::MuokattavaRaportti;
    ui->setupUi( raporttiWidget );

    kaava = kp()->asetukset()->lista( "Raportti/" + raporttinimi );
    optiorivi = kaava.takeFirst();

    tase = optiorivi.startsWith(":tase");
    tulos = optiorivi.startsWith(":tulos");

    // Jos tehdään taselaskelmaa, piilotetaan turhat tiedot!
    ui->alkaa1Date->setVisible( tulos );
    ui->alkaa2Date->setVisible( tulos );
    ui->alkaa3Date->setVisible( tulos );
    ui->alkaaLabel->setVisible( tulos );
    ui->paattyyLabel->setVisible( tulos );

    // Sitten laitetaan valmiiksi tilikausia nykyisestä taaksepäin
    int tilikausiIndeksi = kp()->tilikaudet()->indeksiPaivalle( kp()->paivamaara() );
    if( tilikausiIndeksi > -1 )
    {
        ui->alkaa1Date->setDate( kp()->tilikaudet()->tilikausiIndeksilla(tilikausiIndeksi).alkaa() );
        ui->loppuu1Date->setDate( kp()->tilikaudet()->tilikausiIndeksilla(tilikausiIndeksi).paattyy() );
    }
    if( tilikausiIndeksi > 0)
    {
        ui->alkaa2Date->setDate( kp()->tilikaudet()->tilikausiIndeksilla(tilikausiIndeksi-1).alkaa() );
        ui->loppuu2Date->setDate( kp()->tilikaudet()->tilikausiIndeksilla(tilikausiIndeksi-1).paattyy() );
    }
    ui->sarake2Box->setChecked(tilikausiIndeksi > 0);

    if( tilikausiIndeksi > 1)
    {
        ui->alkaa3Date->setDate( kp()->tilikaudet()->tilikausiIndeksilla(tilikausiIndeksi-2).alkaa() );
        ui->loppuu3Date->setDate( kp()->tilikaudet()->tilikausiIndeksilla(tilikausiIndeksi-2).paattyy() );
    }
    ui->sarake3Box->setChecked(tilikausiIndeksi > 1);




}

MuokattavaRaportti::~MuokattavaRaportti()
{
    delete ui;
}

RaportinKirjoittaja MuokattavaRaportti::raportti()
{
    alustaData();
    return kirjoitaRaportti();
}

RaportinKirjoittaja MuokattavaRaportti::raporttiDatalla(QVector<RaporttiData> raporttiData, bool taseRaportti)
{
    tase = taseRaportti;
    tulos = !taseRaportti;

    data = raporttiData;
    return kirjoitaRaportti();
}

void MuokattavaRaportti::alustaData()
{
    data.clear();
    data.append( RaporttiData( ui->alkaa1Date->date(), ui->loppuu1Date->date()));
    if( ui->sarake2Box->isChecked())
        data.append( RaporttiData(ui->alkaa2Date->date(), ui->loppuu2Date->date()));
    if( ui->sarake3Box->isChecked())
        data.append( RaporttiData(ui->alkaa3Date->date(), ui->loppuu3Date->date()));
}

RaportinKirjoittaja MuokattavaRaportti::kirjoitaRaportti()
{
    RaportinKirjoittaja rk;

    kirjoitaYlatunnisteet( &rk);

    if(tulos)
        laskeTulosData();
    else
        laskeTaseData();

    QRegularExpression tiliRe("(?<alku>\\d+)(\\.\\.)?(?<loppu>\\d*)");

    QVector<int> kokosumma( data.count());

    foreach (QString rivi, kaava)
    {
        int tyhjanpaikka = rivi.indexOf('\t');

        if( tyhjanpaikka < 0 )
            tyhjanpaikka = rivi.indexOf("    ");

        RaporttiRivi rr;

        if( tyhjanpaikka < 0 )
        {
            // Jos ei osu, niin rivillä enintään otsikko
            rr.lisaa(rivi);
            rk.lisaaRivi(rr);
            continue;
        }
        rr.lisaa( rivi.left(tyhjanpaikka) );   // Lisätään teksti
        QString loppurivi = rivi.mid(tyhjanpaikka+1);

        // Lasketaan summat
        QRegularExpressionMatchIterator ri = tiliRe.globalMatch(loppurivi );
        QVector<int> summat( data.count() );

        // ri hakee numerot 1..1
        while( ri.hasNext())
        {
            QRegularExpressionMatch tiliMats = ri.next();
            int alku = Tili::ysiluku( tiliMats.captured("alku").toInt(), false);
            int loppu;

            if( !tiliMats.captured("loppu").isEmpty())
                loppu = Tili::ysiluku(tiliMats.captured("loppu").toInt(), true);
            else
                loppu = Tili::ysiluku( tiliMats.captured("alku").toInt(), true);


            if( loppurivi.contains("details"))
            {
                // details-tuloste: kaikkien välille kuuluvien tilien nimet ja summat
                QMapIterator<int,int> iter( tilisummat );
                while( iter.hasNext())
                {
                    iter.next();
                    if( iter.key() >= alku && iter.key() <= loppu )
                    {
                        RaporttiRivi rr;
                        qDebug() << iter.key() << "  " << (float) iter.value() / 100.0;
                        Tili tili = kp()->tilit()->tiliNumerolla( iter.key() / 10);

                        rr.lisaa( QString("      %1 %2").arg(tili.numero()).arg(tili.nimi()));
                        for( int sarake=0; sarake < data.count(); sarake++)
                        {
                            rr.lisaa( data.at(sarake).summat.value(iter.key(), 0) );
                        }
                        rk.lisaaRivi( rr );
                    }

                }

            }
            else
            {
                // Lasketaan summa joka sarakkeelle
                for( int sarake = 0; sarake < data.count(); sarake++)
                {
                    QMapIterator<int,int> iter( data[sarake].summat);
                    while( iter.hasNext())
                    {
                        iter.next();
                        if( iter.key() >= alku && iter.key() <= loppu )
                        {
                            summat[sarake] += iter.value();
                            kokosumma[sarake] += iter.value();  // Lisätään välisummaan
                        }
                    }
                }

            }
        }

        if( loppurivi.contains("details"))

        // Laskenta tehty, muut valinnat

        if( loppurivi.contains("="))
        {
            // Välisumman lisääminen
            for(int sarake=0; sarake < data.count(); sarake++)
                summat[sarake] += kokosumma[sarake];
        }

        bool kirjauksia = false;

        // Selvitetään, jääkö summa nollaan
        for( int sarake = 0; sarake < data.count(); sarake++)
        {
            if( summat.at(sarake))
            {
                kirjauksia = true;
                break;
            }

        }

        if( !loppurivi.contains('S') && !loppurivi.contains('H') && !kirjauksia)
            continue;       // Ei tulosteta tyhjää riviä ollenkaan

        // header tulostaa vain otsikon
        if( !loppurivi.contains('h') && !loppurivi.contains('H'))
        {
            // Sitten kirjoitetaan summat riville
            for( int sarake=0; sarake < data.count(); sarake++)
                rr.lisaa( summat.at(sarake) );
        }

        rk.lisaaRivi(rr);
    }
    return rk;
}

void MuokattavaRaportti::kirjoitaYlatunnisteet(RaportinKirjoittaja *rk)
{
    rk->asetaOtsikko( otsikko );

    rk->lisaaSarake(40);
    rk->lisaaEurosarake();
    rk->lisaaEurosarake();
    rk->lisaaEurosarake();

    RaporttiRivi otsikkoRivi;
    otsikkoRivi.lisaa("");

    if( tulos )
    {
        // Tuloslaskelmassa koko kausi - pitää ensin lisätä ekalle riville alkupäivät
        for( int i=0; i<data.count(); i++)
            otsikkoRivi.lisaa( QString("%1 -").arg( data.at(i).alkaa.toString(Qt::SystemLocaleShortDate) ) );

        rk->lisaaOtsake(otsikkoRivi);
        otsikkoRivi.tyhjenna();
        otsikkoRivi.lisaa("");
    }
    // Lisätään päättävät päivät
    for( int i=0; i<data.count(); i++)
        otsikkoRivi.lisaa( data.at(i).paattyy );

    rk->lisaaOtsake(otsikkoRivi);
}

void MuokattavaRaportti::laskeTulosData()
{
    tilisummat.clear();

    for(int i=0; i < data.count(); i++)
    {
        QString kysymys = QString("SELECT ysiluku, sum(debetsnt), sum(kreditsnt) "
                                  "from vienti,tili where vienti.tili = tili.id and ysiluku > 300000000 "
                                  "and pvm between \"%1\" and \"%2\" "
                                  "group by ysiluku").arg(data[i].alkaa.toString(Qt::ISODate)).arg(data[i].paattyy.toString(Qt::ISODate));
        QSqlQuery query(kysymys);
        while (query.next())
        {
            int ysiluku = query.value(0).toInt();
            int debet = query.value(1).toInt();
            int kredit = query.value(2).toInt();

            data[i].summat.insert( ysiluku, kredit - debet );
            tilisummat[ysiluku] = (kredit - debet) + tilisummat.value(ysiluku);
        }
    }
}

void MuokattavaRaportti::laskeTaseData()
{
    tilisummat.clear();

    for(int i=0; i < data.count(); i++)
    {
        // 1) Sijoitetaan tasetilien saldot kysytylle päivälle

        QString kysymys = QString("SELECT ysiluku, sum(debetsnt), sum(kreditsnt) "
                                  "from vienti,tili where vienti.tili = tili.id and ysiluku < 300000000 "
                                  "and pvm <= \"%1\" "
                                  "group by ysiluku").arg(data[i].paattyy.toString(Qt::ISODate));
        QSqlQuery query(kysymys);
        while (query.next())
        {
            int ysiluku = query.value(0).toInt();
            int debet = query.value(1).toInt();
            int kredit = query.value(2).toInt();

            data[i].summat.insert( ysiluku, debet - kredit );
            tilisummat[ysiluku] = ( debet - kredit ) + tilisummat.value(ysiluku);
        }

        // 2)  Sijoitetaan "edellisten tilikausien alijäämä/ylijäämä" ko.tilille
        Tilikausi tilikausi = kp()->tilikaudet()->tilikausiPaivalle( data[i].paattyy );

        kysymys = QString("SELECT sum(debetsnt), sum(kreditsnt) FROM vienti, tili WHERE vienti.tili=tili.id "
                          " AND ysiluku > 300000000 AND pvm < \"%1\" ").arg( tilikausi.alkaa().toString(Qt::ISODate));
        query.exec(kysymys);
        if( query.next())
        {
            int edYlijaama = query.value(1).toInt() - query.value(0).toInt();

            int kertymaTilinYsiluku = kp()->tilit()->edellistenYlijaamaTili().ysivertailuluku();
            if( kertymaTilinYsiluku )
                data[i].summat[ kertymaTilinYsiluku] = edYlijaama + data[i].summat.value( kertymaTilinYsiluku, 0);

        }
        qDebug() << query.lastQuery();

        // 3) Sijoitetaan tämän tilikauden tulos "tulostilille" 0
        kysymys = QString("SELECT sum(debetsnt), sum(kreditsnt) FROM vienti, tili WHERE vienti.tili=tili.id"
                          " AND ysiluku > 300000000 AND pvm BETWEEN \"%1\" AND \"%2\"")
                .arg( tilikausi.alkaa().toString(Qt::ISODate) ).arg( data[i].paattyy.toString(Qt::ISODate));

        query.exec(kysymys);
        if( query.next() )
        {
            int debet = query.value(0).toInt();
            int kredit = query.value(1).toInt();
            data[i].summat.insert(0, kredit - debet);
        }

    }
}

