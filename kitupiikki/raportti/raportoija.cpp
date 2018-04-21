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
#include <QDebug>
#include <QSqlError>

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
    // Jos raporttia ei ole, jää VIRHEELLINEN-raportti
    if( kaava_.length() > 2)
    {
        optiorivi_ = kaava_.takeFirst();

        if( optiorivi_.startsWith(":tulos"))
            tyyppi_ = TULOSLASKELMA;
        else if( optiorivi_.startsWith(":tase"))
            tyyppi_ = TASE;
        else if( optiorivi_.startsWith(":kohdennus"))
            tyyppi_ = KOHDENNUSLASKELMA;
    }

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

RaportinKirjoittaja Raportoija::raportti(bool tulostaErittelyt)
{
    data_.resize( loppuPaivat_.count() );

    RaportinKirjoittaja rk;
    kirjoitaYlatunnisteet(rk);

    if( tyyppi() == TULOSLASKELMA)
    {
        if( kohdennusKaytossa_.size())
        {
            for(int kohdennus : kohdennusKaytossa_)
                laskeKohdennusData(kohdennus);
        }
        else
                laskeTulosData();

        kirjoitaDatasta(rk, tulostaErittelyt);
    }
    else if( tyyppi() == TASE )
    {
        if( kohdennusKaytossa_.size())
        {
            for( int kohdennus : kohdennusKaytossa_)
                laskeKohdennusData(kohdennus, true);
        }

        laskeTaseDate();
        kirjoitaDatasta(rk, tulostaErittelyt);
    }
    else if( tyyppi() == KOHDENNUSLASKELMA )
    {
        // Lajitellaan kohdennukset aakkosiin
        // kohdennuksen nimen mukaan mutta kuintekin niin, että Yleinen on alussa

        kohdennusKaytossa_.sort( [](int &a, int &b)     {
                                                Kohdennus ka = kp()->kohdennukset()->kohdennus(a);
                                                Kohdennus kb = kp()->kohdennukset()->kohdennus(b);
                                                if( ka.tyyppi() == Kohdennus::EIKOHDENNETA)
                                                    return true;
                                                else if(kb.tyyppi() == Kohdennus::EIKOHDENNETA)
                                                    return false;
                                                else
                                                    return ka.nimi().localeAwareCompare( kb.nimi() ) < 0;
                                            });
        kohdennusKaytossa_.unique();    // Poistetaan tuplat


        for( int kohdennusId : kohdennusKaytossa_)
        {
            Kohdennus kohdennus = kp()->kohdennukset()->kohdennus( kohdennusId );

            RaporttiRivi rr;
            rr.lihavoi();
            rr.lisaa( kohdennus.nimi().toUpper() );
            rk.lisaaRivi(rr);

            laskeKohdennusData( kohdennus.id() );

            kirjoitaDatasta(rk, tulostaErittelyt);
            rk.lisaaRivi( RaporttiRivi());
        }
    }
    return rk;
}


void Raportoija::kirjoitaYlatunnisteet(RaportinKirjoittaja &rk)
{
    QString otsikko = otsikko_;
    // Jos otsikko päättyy tarkenteeseen /Yleinen, ei sitä tartte tulostaa
    if( otsikko.endsWith("/Yleinen") )
        otsikko.truncate( otsikko_.lastIndexOf(QChar('/') ) ) ;

    if( tyyppi() != KOHDENNUSLASKELMA && kohdennusKaytossa_.size() )
    {
        // Jos poimittu kohdennuksia, niin näyttään ne otsikossa jotta näkee että tämä on ote
        QStringList kohdennukset;
        for(int kohdId : kohdennusKaytossa_)
            kohdennukset.append( kp()->kohdennukset()->kohdennus( kohdId ).nimi() );
        otsikko.append( " (" + kohdennukset.join(",") + ")" );
    }

    rk.asetaOtsikko( otsikko);


    rk.lisaaSarake(40);
    for( int i=0; i < loppuPaivat_.count(); i++)
        rk.lisaaEurosarake();

    // Kausiraportissa ylemmällä rivillä alkupäivä 1.1.2017 -
    if( onkoKausiraportti())
    {
        RaporttiRivi orivi;
        orivi.lisaa("");
        for(int i=0; i < alkuPaivat_.count(); i++)
            orivi.lisaa( QString("%1 -").arg( alkuPaivat_.at(i).toString("dd.MM.yyyy") ), 1, true );
        rk.lisaaOtsake(orivi);
    }
    // Tasepäivät tai loppupäivät
    RaporttiRivi olrivi;
    olrivi.lisaa("");
    for(int i=0; i < loppuPaivat_.count(); i++)
        olrivi.lisaa( loppuPaivat_.at(i).toString("dd.MM.yyyy"), 1, true );
    rk.lisaaOtsake(olrivi);
}

void Raportoija::kirjoitaDatasta(RaportinKirjoittaja &rk, bool tulostaErittelyt)
{

    QRegularExpression tiliRe("[\\s\\t,](?<alku>\\d{1,8})(\\.\\.)?(?<loppu>\\d{0,8})(?<menotulo>[+-]?)");
    QRegularExpression maareRe("(?<maare>([A-Za-z=]+|\\*))(?<sisennys>[0-9]?)");

    // Välisummien käsittelyä = varten
    QVector<qlonglong> kokosumma( data_.count());

    foreach (QString rivi, kaava_)
    {
        if( !rivi.length() )
        {
            rk.lisaaTyhjaRivi();
            continue;
        }

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

        QString loppurivi = rivi.mid(tyhjanpaikka);     // Aloittava tyhjä mukaan!

        // Lasketaan summat
        QVector<qlonglong> summat( data_.count() );

        int sisennys = 0;

        RivinTyyppi rivityyppi = SUMMA;
        bool naytaTyhjarivi = false;
        bool laskevalisummaan = true;
        bool lisaavalisumma = false;
        bool naytaErittely = false;
        int erittelySisennys = 4;

        // Haetaan määreet
        QRegularExpressionMatchIterator mri = maareRe.globalMatch( loppurivi );
        while( mri.hasNext())
        {
            QRegularExpressionMatch maareMats = mri.next();
            QString maare = maareMats.captured("maare");

            // Sisennys
            if( !maareMats.captured("sisennys").isEmpty())
            {
                int uusisisennys = maareMats.captured("sisennys").toInt();
                if( maare == "*")
                    erittelySisennys = uusisisennys;
                else
                    sisennys = uusisisennys;
            }
            if( maare == "*")
            {
                naytaErittely = true;
            }
            else if( maare == "S" || maare == "SUM" || maare == "SUMMA")
            {
                naytaTyhjarivi = true;
            }
            else if( maare == "H" || maare=="HEADING" || maare == "OTSIKKO")
            {
                rivityyppi = OTSIKKO;
                naytaTyhjarivi = true;
            }
            else if( maare == "d" || maare == "details" || maare == "erittely")
                rivityyppi = ERITTELY;
            else if( maare == "h" || maare == "heading" || maare == "otsikko")
                rivityyppi = OTSIKKO;
            else if( maare == "=")
                lisaavalisumma = true;
            else if( maare == "==")
                laskevalisummaan = false;
            else if( maare == "bold" || maare == "lihava")
                rr.lihavoi(true);
            else if( maare == "viiva" || maare == "line")
                rr.viivaYlle(true);
        }

        // Sisennys paikoilleen!
        QString sisennysStr;
        for( int i=0; i < sisennys; i++)
            sisennysStr.append(' ');

        rr.lisaa( sisennysStr + rivi.left(tyhjanpaikka) );   // Lisätään teksti


        if( rivityyppi != ERITTELY)
        {

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
                bool vainMenot = tiliMats.captured("menotulo") == "-";

                // Lasketaan summa joka sarakkeelle
                for( int sarake = 0; sarake < data_.count(); sarake++)
                {
                    QMapIterator<int,qlonglong> iter( data_.at(sarake));
                    while( iter.hasNext())
                    {
                        iter.next();
                        if( iter.key() >= alku && iter.key() <= loppu )
                        {
                            if( vainTulot || vainMenot)
                            {
                                Tili tili = kp()->tilit()->tiliNumerolla( iter.key() / 10);

                                // Ohitetaan, jos haluttu vain tulot ja menot eikä ole niitä
                                if( (vainTulot && !tili.onko(TiliLaji::TULO) ) || (vainMenot && !tili.onko(TiliLaji::MENO) ))
                                        continue;
                            }

                            summat[sarake] += iter.value();

                            if( laskevalisummaan)
                                kokosumma[sarake] += iter.value();  // Lisätään välisummaan
                        }
                    }
                }

            }
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
            else if( !haettuTileja && !lisaavalisumma)
                rivityyppi = OTSIKKO;
        }

        // header tulostaa vain otsikon
        if( rivityyppi != OTSIKKO  )
        {
            // Sitten kirjoitetaan summat riville
            for( int sarake=0; sarake < data_.count(); sarake++)
                rr.lisaa( summat.at(sarake) , true );
        }

        if( rivityyppi != ERITTELY)
            rk.lisaaRivi(rr);

        if( rivityyppi == ERITTELY || (naytaErittely && tulostaErittelyt ))
        {
            // eriSisennysStr on erittelyrivin aloitussisennys, joka *-rivillä kasvaa edellisen rivin sisennyksestä
            QString eriSisennysStr = sisennysStr;
            if( naytaErittely )
            {
                for( int i=0; i < erittelySisennys; i++)
                    eriSisennysStr.append(' ');
            }

            // details-tuloste: kaikkien välille kuuluvien tilien nimet ja summat
            // sama, mikäli tavallista summariviä seuraa *-merkillä tulostuva erittely

            QRegularExpressionMatchIterator ri = tiliRe.globalMatch(loppurivi );

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
                bool vainMenot = tiliMats.captured("menotulo") == "-";


                QMapIterator<int,bool> iter( tilitKaytossa_ );
                while( iter.hasNext())
                {
                    iter.next();
                    if( iter.key() >= alku && iter.key() <= loppu )
                    {
                        RaporttiRivi rr;
                        Tili tili = kp()->tilit()->tiliNumerolla( iter.key() / 10);

                        // Ohitetaan, jos haluttu vain tulot ja menot eikä ole niitä
                        if( (vainTulot && !tili.onko(TiliLaji::TULO) ) || (vainMenot && !tili.onko(TiliLaji::MENO)))
                                continue;

                        // Erittelyriville tilin numero ja nimi sekä summat
                        rr.lisaaLinkilla( RaporttiRiviSarake::TILI_NRO, tili.numero(), QString("%1%2 %3").arg(eriSisennysStr).arg(tili.numero()).arg(tili.nimi()));
                        for( int sarake=0; sarake < data_.count(); sarake++)
                        {
                            rr.lisaa( data_.at(sarake).value(iter.key(), 0) );
                        }
                        rk.lisaaRivi( rr );
                    }
                }

            }

        }

    }
}

void Raportoija::sijoitaTulosKyselyData(const QString &kysymys, int i)
{
    QSqlQuery query(kysymys);

    qDebug() << query.lastQuery() << " " << query.lastError().text();

    qlonglong tulossumma = 0;

    while( query.next())
    {
        int ysiluku = query.value(0).toInt();
        qlonglong debet = query.value(1).toLongLong();
        qlonglong kredit = query.value(2).toLongLong();

        data_[i].insert( ysiluku, kredit - debet  );
        tilitKaytossa_.insert( ysiluku, true);

        tulossumma += kredit - debet;
    }

    // Sijoitetaan vielä summa "tilille" 0
    data_[i].insert( 0, tulossumma );
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
            qlonglong debet = query.value(1).toLongLong();
            qlonglong kredit = query.value(2).toLongLong();

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
            qlonglong edYlijaama = query.value(1).toLongLong() - query.value(0).toLongLong();

            int kertymaTilinYsiluku = kp()->tilit()->edellistenYlijaamaTili().ysivertailuluku();
            if( kertymaTilinYsiluku )
            {
                data_[i][ kertymaTilinYsiluku] = edYlijaama + data_[i].value( kertymaTilinYsiluku, 0);
                tilitKaytossa_.insert(kertymaTilinYsiluku, true);
            }

        }

        // 3) Sijoitetaan tämän tilikauden tulos "tulostilille" 0 ja määritellylle tulostilille
        kysymys = QString("SELECT sum(debetsnt), sum(kreditsnt) FROM vienti, tili WHERE vienti.tili=tili.id"
                          " AND ysiluku > 300000000 AND pvm BETWEEN \"%1\" AND \"%2\"")
                .arg( tilikausi.alkaa().toString(Qt::ISODate) ).arg( loppuPaivat_.at(i).toString(Qt::ISODate));

        query.exec(kysymys);
        if( query.next() )
        {
            qlonglong debet = query.value(0).toLongLong();
            qlonglong kredit = query.value(1).toLongLong();
            data_[i].insert(0, kredit - debet);
            if( kp()->tilit()->tiliTyypilla(TiliLaji::KAUDENTULOS).onkoValidi())
            {
                data_[i].insert(kp()->tilit()->tiliTyypilla(TiliLaji::KAUDENTULOS).ysivertailuluku(), kredit - debet);
                tilitKaytossa_.insert(kp()->tilit()->tiliTyypilla(TiliLaji::KAUDENTULOS).ysivertailuluku(), true  );
            }
        }

    }
}

void Raportoija::laskeKohdennusData(int kohdennusId, bool poiminnassa)
{
    data_.clear();
    data_.resize( loppuPaivat_.count());

    tilitKaytossa_.clear();
    Kohdennus kohdennus = kp()->kohdennukset()->kohdennus(kohdennusId);

    // Kohdennuksen summien laskemista
    for( int i = 0; i < alkuPaivat_.count(); i++)
    {
        QString kysymys;

        if( kohdennus.tyyppi() == Kohdennus::MERKKAUS)
            kysymys = QString("SELECT ysiluku, sum(debetsnt), sum(kreditsnt) "
                                  "from merkkaus, vienti,tili where merkkaus.kohdennus=%3 "
                                  "AND merkkaus.vienti=vienti.id AND vienti.tili = tili.id and ysiluku > 300000000 "
                                  "and pvm between \"%1\" and \"%2\"  "
                                  "group by ysiluku")
                .arg( alkuPaivat_.at(i).toString(Qt::ISODate))
                .arg(loppuPaivat_.at(i).toString(Qt::ISODate))
                .arg(kohdennusId);
        else
            kysymys = QString("SELECT ysiluku, sum(debetsnt), sum(kreditsnt) "
                                  "from vienti,tili where vienti.tili = tili.id and ysiluku > 300000000 "
                                  "and pvm between \"%1\" and \"%2\" and kohdennus=%3 "
                                  "group by ysiluku")
                .arg( alkuPaivat_.at(i).toString(Qt::ISODate))
                .arg(loppuPaivat_.at(i).toString(Qt::ISODate))
                .arg(kohdennusId);

        sijoitaTulosKyselyData(kysymys, i);

        // Tasetilien summat
        if( kohdennus.tyyppi() == Kohdennus::MERKKAUS)
            kysymys = QString("SELECT ysiluku, sum(debetsnt), sum(kreditsnt) "
                                      "from merkkaus, vienti,tili where merkkaus.kohdennus=%2 AND "
                                      "merkkaus.vienti=vienti.id AND vienti.tili = tili.id and ysiluku < 300000000 "
                                      "and pvm <= \"%1\"  "
                                      "group by ysiluku").arg(loppuPaivat_.at(i).toString(Qt::ISODate))
                                                         .arg(kohdennusId);

        kysymys = QString("SELECT ysiluku, sum(debetsnt), sum(kreditsnt) "
                                  "from vienti,tili where vienti.tili = tili.id and ysiluku < 300000000 "
                                  "and pvm <= \"%1\" and kohdennus=%2 "
                                  "group by ysiluku").arg(loppuPaivat_.at(i).toString(Qt::ISODate))
                                                     .arg(kohdennusId);
        QSqlQuery query(kysymys);
        while (query.next())
        {
            int ysiluku = query.value(0).toInt();
            qlonglong debet = query.value(1).toLongLong();
            qlonglong kredit = query.value(2).toLongLong();

            if( poiminnassa && ysiluku > 200000000 )
                data_[i].insert( ysiluku, kredit - debet);
            else
                data_[i].insert( ysiluku, debet - kredit );

            tilitKaytossa_.insert( ysiluku, true);
        }
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
            kohdennusKaytossa_.push_back( kysely.value(0).toInt());
    }
}

void Raportoija::lisaaKohdennus(int kohdennusId)
{
    kohdennusKaytossa_.push_back( kohdennusId);
}
