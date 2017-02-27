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

#include <QRegularExpression>
#include <QRegularExpressionMatch>

#include "raportoija.h"
#include "raporttirivi.h"

#include "db/kirjanpito.h"
#include "db/tilikausi.h"


Raportoija::Raportoija(const QString &raportinNimi) :
    otsikko_(raportinNimi),
    tyyppi_ ( VIRHEELLINEN )
{
    kaava_ = kp()->asetukset()->lista("Raportti/" + raportinNimi);
    optiorivi_ = kaava_.takeFirst();

    if( optiorivi_.startsWith(":tulos"))
        tyyppi_ = TULOSLASKELMA;
    else if( optiorivi_.startsWith(":tase"))
        tyyppi_ = TASE;
    else if( optiorivi_.startsWith(":kohdennus"))
        tyyppi_ = KOHDENNUSLASKELMA;
    else if( optiorivi_.startsWith(":projekti"))
        tyyppi_ = PROJEKTITASE;
}

void Raportoija::lisaaKausi(const QDate &alkaa, const QDate &paattyy)
{
    alkuPaivat_.append(alkaa);
    loppuPaivat_.append(paattyy);
}

void Raportoija::lisaaTasepaiva(const QDate &pvm)
{
    loppuPaivat_.append(pvm);
}

void Raportoija::valitseProjektit(const QDate &paivasta, const QDate &paivaan)
{
    // Haetaan projektit, jotka ulottuvat pyydetylle päivämäärävälille
    // Tarvitaan laskettaessa projektitasetta, kustannuslaskelmassa haetaan ne
    // kohdennukset, joille kirjauksia haluttuina kausina

    QString kysymys = QString("SELECT id FROM kohdennus WHERE (alkaa IS NULL OR ( alkaa < \"%1\" AND loppuu > \"%2\" )) AND tyyppi = 2")
            .arg(paivaan.toString(Qt::ISODate))
            .arg(paivasta.toString(Qt::ISODate));
    QSqlQuery kysely(kysymys);

    while( kysely.next())
    {
        kohdennusKaytossa_.insert( kysely.value(0).toInt(), true);
    }

}

RaportinKirjoittaja Raportoija::raportti()
{
    RaportinKirjoittaja rk;
    kirjoitaYlatunnisteet(rk);

    if( tyyppi() == TULOSLASKELMA)
    {
        laskeTulosData();
        kirjoitaDatasta(rk);
    }
    else if( tyyppi() == TASE )
    {
        laskeTaseDate();
        kirjoitaDatasta(rk);
    }
    else if( tyyppi() == KOHDENNUSLASKELMA || tyyppi() == PROJEKTITASE)
    {
        if( tyyppi() == KOHDENNUSLASKELMA)
            etsiKohdennukset();

        // Projektitaseessa projektit pitää olla jo rajattuina!

        foreach (int kohdennusId, kohdennusKaytossa_)
        {
            Kohdennus kohdennus = kp()->kohdennukset()->kohdennus(kohdennusId);
            RaporttiRivi rr;
            rr.lisaa( kohdennus.nimi().toUpper() );
            rr.lihavoi(true);
            rk.lisaaRivi(rr);
            rk.lisaaRivi( RaporttiRivi());

            laskeKohdennusData( kohdennusId );
            kirjoitaDatasta(rk);
            rk.lisaaRivi( RaporttiRivi());
        }
    }
    return rk;
}


void Raportoija::kirjoitaYlatunnisteet(RaportinKirjoittaja &rk)
{
    rk.asetaOtsikko( otsikko_);

    rk.lisaaSarake(40);
    for( int i=0; i < loppuPaivat_.count(); i++)
        rk.lisaaEurosarake();

    // Kausiraportissa ylemmällä rivillä alkupäivä 1.1.2017 -
    if( onkoKausiraportti())
    {
        RaporttiRivi orivi;
        orivi.lisaa("");
        for(int i=0; i < alkuPaivat_.count(); i++)
            orivi.lisaa( QString("%1 -").arg( alkuPaivat_.at(i).toString(Qt::SystemLocaleShortDate) ) );
        rk.lisaaOtsake(orivi);
    }
    // Tasepäivät tai loppupäivät
    RaporttiRivi olrivi;
    olrivi.lisaa("");
    for(int i=0; i < loppuPaivat_.count(); i++)
        olrivi.lisaa( loppuPaivat_.at(i) );
    rk.lisaaOtsake(olrivi);
}

void Raportoija::kirjoitaDatasta(RaportinKirjoittaja &rk)
{

    QRegularExpression tiliRe("[\\s\\t](?<alku>\\d{1,8})(\\.\\.)?(?<loppu>\\d{0,8})(?<menotulo>[\\+-])?");
    QRegularExpression maareRe("(?<maare>[A-Za-z=]+)(?<sisennys>[0-9]?)");

    // Välisummien käsittelyä = varten
    QVector<int> kokosumma( data_.count());

    foreach (QString rivi, kaava_)
    {
        int tyhjanpaikka = rivi.indexOf('\t');

        if( tyhjanpaikka < 0 )
            tyhjanpaikka = rivi.indexOf("    ");

        RaporttiRivi rr;

        if( tyhjanpaikka < 0 )
        {
            // Jos pelkkää tekstiä, niin se on sitten otsikko
            rr.lisaa(rivi);
            rk.lisaaRivi(rr);
            continue;
        }

        QString loppurivi = rivi.mid(tyhjanpaikka+1);

        // Lasketaan summat
        QVector<int> summat( data_.count() );

        int sisennys = 0;

        RivinTyyppi rivityyppi = SUMMA;
        bool naytaTyhjarivi = false;
        bool laskevalisummaan = true;
        bool lisaavalisumma = false;

        // Haetaan määreet
        QRegularExpressionMatchIterator mri = maareRe.globalMatch( loppurivi );
        while( mri.hasNext())
        {
            QRegularExpressionMatch maareMats = mri.next();
            QString maare = maareMats.captured("maare");
            int uusisisennys = maareMats.captured("sisennys").toInt();
            if( uusisisennys)       // Määritellään rivin sisennys
                sisennys = uusisisennys;

            if( maare == "S" || maare == "SUM")
            {
                naytaTyhjarivi = true;
            }
            else if( maare == "H" || maare=="HEADING")
            {
                rivityyppi = OTSIKKO;
                naytaTyhjarivi = true;
            }
            else if( maare == "d" || maare == "details")
                rivityyppi = ERITTELY;
            else if( maare == "h" || maare == "heading")
                rivityyppi = OTSIKKO;
            else if( maare == "=")
                lisaavalisumma = true;
            else if( maare == "==")
                laskevalisummaan = false;
            else if( maare == "bold")
                rr.lihavoi(true);
        }

        // Sisennys paikoilleen!
        QString sisennysStr;
        for( int i=0; i < sisennys; i++)
            sisennysStr.append(' ');

        rr.lisaa( sisennysStr + rivi.left(tyhjanpaikka) );   // Lisätään teksti


        QRegularExpressionMatchIterator ri = tiliRe.globalMatch(loppurivi );
        // ri hakee numerot 1..1
        bool haettuTileja = ri.hasNext();   // Onko tiliväli määritelty (ellei, niin kyse on otsikosta)

        while( ri.hasNext())
        {
            QRegularExpressionMatch tiliMats = ri.next();
            int alku = Tili::ysiluku( tiliMats.captured("alku").toInt(), false);
            int loppu;

            if( !tiliMats.captured("loppu").isEmpty())
                loppu = Tili::ysiluku(tiliMats.captured("loppu").toInt(), true);
            else
                loppu = Tili::ysiluku( tiliMats.captured("alku").toInt(), true);
            bool vainTulot = tiliMats.captured("menotulo") == "+";
            bool vainMenot = tiliMats.captured("tulomeno") == "-";


            if( rivityyppi == ERITTELY )
            {
                // details-tuloste: kaikkien välille kuuluvien tilien nimet ja summat
                QMapIterator<int,bool> iter( tilitKaytossa_ );
                while( iter.hasNext())
                {
                    iter.next();
                    if( iter.key() >= alku && iter.key() <= loppu )
                    {
                        RaporttiRivi rr;
                        Tili tili = kp()->tilit()->tiliNumerolla( iter.key() / 10);

                        // Ohitetaan, jos haluttu vain tulot ja menot eikä ole niitä
                        if( (vainTulot && !tili.onkoTulotili()) || (vainMenot && !tili.onkoMenotili()))
                                continue;

                        // Erittelyriville tilin numero ja nimi sekä summat
                        rr.lisaa( QString("%1%2 %3").arg(sisennysStr).arg(tili.numero()).arg(tili.nimi()));
                        for( int sarake=0; sarake < data_.count(); sarake++)
                        {
                            rr.lisaa( data_.at(sarake).value(iter.key(), 0) );
                        }
                        rk.lisaaRivi( rr );
                    }

                }

            }
            else
            {
                // Lasketaan summa joka sarakkeelle
                for( int sarake = 0; sarake < data_.count(); sarake++)
                {
                    QMapIterator<int,int> iter( data_.at(sarake));
                    while( iter.hasNext())
                    {
                        iter.next();
                        if( iter.key() >= alku && iter.key() <= loppu )
                        {
                            if( vainTulot && vainMenot)
                            {
                                Tili tili = kp()->tilit()->tiliNumerolla( iter.key() / 10);

                                // Ohitetaan, jos haluttu vain tulot ja menot eikä ole niitä
                                if( (vainTulot && !tili.onkoTulotili()) || (vainMenot && !tili.onkoMenotili()))
                                        continue;
                            }

                            summat[sarake] += iter.value();

                            if( laskevalisummaan)
                                kokosumma[sarake] += iter.value();  // Lisätään välisummaan
                        }
                    }
                }

            }
        }

        if( rivityyppi == ERITTELY )    // Erittelyn rivit on jo tulostettu ;)
            continue;

        if( lisaavalisumma )
        {
            // Välisumman lisääminen
            for(int sarake=0; sarake < data_.count(); sarake++)
                summat[sarake] += kokosumma.at(sarake);
        }

        bool kirjauksia = false;
        // Selvitetään, jääkö summa nollaan
        for( int sarake = 0; sarake < data_.count(); sarake++)
        {
            if( summat.at(sarake))
            {
                kirjauksia = true;
                break;
            }

        }

        if( !naytaTyhjarivi && !kirjauksia && haettuTileja && !lisaavalisumma)
            continue;       // Ei tulosteta tyhjää riviä ollenkaan

        // header tulostaa vain otsikon
        if( rivityyppi != OTSIKKO )
        {
            // Sitten kirjoitetaan summat riville
            for( int sarake=0; sarake < data_.count(); sarake++)
                rr.lisaa( summat.at(sarake) );
        }

        rk.lisaaRivi(rr);
    }
}

void Raportoija::sijoitaTulosKyselyData(const QString &kysymys, int i)
{
    QSqlQuery query(kysymys);
    while( query.next())
    {
        int ysiluku = query.value(0).toInt();
        int debet = query.value(1).toInt();
        int kredit = query.value(2).toInt();

        data_[i].insert( ysiluku, kredit - debet  );
        tilitKaytossa_.insert( ysiluku, true);
    }
}

void Raportoija::laskeTulosData()
{
    // Tuloslaskelman summien laskemista
    for( int i = 0; i < alkuPaivat_.count(); i++)
    {
        QString kysymys = QString("SELECT ysiluku, sum(debetsnt), sum(kreditsnt) "
                                  "from vienti,tili where vienti.tili = tili.id and ysiluku > 300000000 "
                                  "and pvm between \"%1\" and \"%2\" "
                                  "group by ysiluku").arg( alkuPaivat_.at(i).toString(Qt::ISODate)).arg(loppuPaivat_.at(i).toString(Qt::ISODate));


        sijoitaTulosKyselyData( kysymys , i);
    }

}

void Raportoija::laskeTaseDate()
{
    // Taseen summien laskeminen
    for( int i=0; i < loppuPaivat_.count(); i++)
    {
        // 1) Tasetilien summat
        QString kysymys = QString("SELECT ysiluku, sum(debetsnt), sum(kreditsnt) "
                                  "from vienti,tili where vienti.tili = tili.id and ysiluku < 300000000 "
                                  "and pvm <= \"%1\" "
                                  "group by ysiluku").arg(loppuPaivat_.at(i).toString(Qt::ISODate));
        QSqlQuery query(kysymys);
        while (query.next())
        {
            int ysiluku = query.value(0).toInt();
            int debet = query.value(1).toInt();
            int kredit = query.value(2).toInt();

            if( ysiluku < 200000000)    // Vastaavaa
                data_[i].insert( ysiluku, debet - kredit );
            else                        // Vastattavaa
                data_[i].insert( ysiluku, kredit - debet );

            tilitKaytossa_.insert( ysiluku, true);
        }

        // 2)  Sijoitetaan "edellisten tilikausien alijäämä/ylijäämä" ko.tilille
        Tilikausi tilikausi = kp()->tilikaudet()->tilikausiPaivalle( loppuPaivat_.at(i) );

        kysymys = QString("SELECT sum(debetsnt), sum(kreditsnt) FROM vienti, tili WHERE vienti.tili=tili.id "
                          " AND ysiluku > 300000000 AND pvm < \"%1\" ").arg( tilikausi.alkaa().toString(Qt::ISODate));
        query.exec(kysymys);
        if( query.next())
        {
            int edYlijaama = query.value(1).toInt() - query.value(0).toInt();

            int kertymaTilinYsiluku = kp()->tilit()->edellistenYlijaamaTili().ysivertailuluku();
            if( kertymaTilinYsiluku )
                data_[i][ kertymaTilinYsiluku] = edYlijaama + data_[i].value( kertymaTilinYsiluku, 0);

        }

        // 3) Sijoitetaan tämän tilikauden tulos "tulostilille" 0
        kysymys = QString("SELECT sum(debetsnt), sum(kreditsnt) FROM vienti, tili WHERE vienti.tili=tili.id"
                          " AND ysiluku > 300000000 AND pvm BETWEEN \"%1\" AND \"%2\"")
                .arg( tilikausi.alkaa().toString(Qt::ISODate) ).arg( loppuPaivat_.at(i).toString(Qt::ISODate));

        query.exec(kysymys);
        if( query.next() )
        {
            int debet = query.value(1).toInt();
            int kredit = query.value(0).toInt();
            data_[i].insert(0, debet - kredit);
        }

    }
}

void Raportoija::laskeKohdennusData(int kohdennus)
{
    data_.clear();
    tilitKaytossa_.clear();

    // Kohdennuksen summien laskemista
    for( int i = 0; i < alkuPaivat_.count(); i++)
    {
        QString kysymys = QString("SELECT ysiluku, sum(debetsnt), sum(kreditsnt) "
                                  "from vienti,tili where vienti.tili = tili.id and ysiluku > 300000000 "
                                  "and pvm between \"%1\" and \"%2\" and kohdennus=%3"
                                  "group by ysiluku")
                .arg( alkuPaivat_.at(i).toString(Qt::ISODate))
                .arg(loppuPaivat_.at(i).toString(Qt::ISODate))
                .arg(kohdennus);

        sijoitaTulosKyselyData(kysymys, i);
    }
}

void Raportoija::laskeProjektiData(int kohdennus)
{
    data_.clear();
    tilitKaytossa_.clear();
    // Kohdennuksen summien laskemista
    for( int i = 0; i < alkuPaivat_.count(); i++)
    {
        QString kysymys = QString("SELECT ysiluku, sum(debetsnt), sum(kreditsnt) "
                                  "from vienti,tili where vienti.tili = tili.id and ysiluku > 300000000 "
                                  "and pvm <= \"%2\" and kohdennus=%2"
                                  "group by ysiluku")
                .arg(loppuPaivat_.at(i).toString(Qt::ISODate))
                .arg(kohdennus);

        sijoitaTulosKyselyData(kysymys, i);
    }
}

void Raportoija::etsiKohdennukset()
{
    for( int i = 0; i < loppuPaivat_.count(); i++)
    {
        QString kysymys = QString("SELECT kohdennus from vienti where pvm between \"%1\" and \"%2\" group by kohdennus")
                .arg( alkuPaivat_.at(i).toString(Qt::ISODate))
                .arg( loppuPaivat_.at(i).toString( Qt::ISODate));
        QSqlQuery kysely(kysymys);

        while( kysely.next())
            kohdennusKaytossa_.insert( kysely.value(0).toInt(), true);
    }
}
