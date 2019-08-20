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

#include <QJsonDocument>

Raportoija::Raportoija(const QString &raportinNimi, const QString &kieli, QObject *parent) :
    Raportteri (parent),
    kieli_(kieli),
    otsikko_(raportinNimi),
    tyyppi_ ( VIRHEELLINEN )
{
    if( raportinNimi.startsWith("tase/"))
        tyyppi_ = TASE;
    else if( raportinNimi.startsWith("tulos/"))
        tyyppi_ = TULOSLASKELMA;

    QString kaava = kp()->asetukset()->asetus(raportinNimi);
    QJsonDocument doc = QJsonDocument::fromJson( kaava.toUtf8() );
    kmap_ = doc.toVariant().toMap();

    rk.asetaOtsikko( kmap_.value("nimi").toMap().value(kieli).toString() );

    kaava_ = kp()->asetukset()->lista("Raportti/" + raportinNimi);

}

void Raportoija::lisaaKausi(const QDate &alkaa, const QDate &paattyy, int tyyppi)
{
    alkuPaivat_.append(alkaa);
    loppuPaivat_.append(paattyy);
    sarakeTyypit_.append(tyyppi);
}

void Raportoija::lisaaTasepaiva(const QDate &pvm)
{
    loppuPaivat_.append(pvm);
    sarakeTyypit_.append(TOTEUTUNUT);
}

RaportinKirjoittaja Raportoija::raportti(bool tulostaErittelyt)
{
    data_.resize( loppuPaivat_.count() );

    kirjoitaYlatunnisteet();

    if( tyyppi() == TULOSLASKELMA)
    {
        if( kohdennusKaytossa_.size())
        {
            for(int kohdennus : kohdennusKaytossa_)
            {
                laskeKohdennusData(kohdennus);
                sijoitaBudjetti(kohdennus);
            }
        }
        else
        {
                laskeTulosData();
                sijoitaBudjetti();
        }

        kirjoitaDatasta();
    }
    else if( tyyppi() == TASE )
    {
        if( kohdennusKaytossa_.size())
        {
            for( int kohdennus : kohdennusKaytossa_)
                laskeKohdennusData(kohdennus, true);
        }

        laskeTaseDate();
        budjetti_.resize( loppuPaivat_.count() );

        kirjoitaDatasta();
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
            sijoitaBudjetti( kohdennus.id() );

            kirjoitaDatasta();
            rk.lisaaRivi( RaporttiRivi());
        }
    }
    return rk;
}

void Raportoija::dataSaapuu(int sarake, QVariant *variant)
{
    QVariantMap map = variant->toMap();
    QMapIterator<QString,QVariant> iter(map);
    while( iter.hasNext()) {
        iter.next();
        int tili = iter.key().toInt();
        int ysiluku = Tili::ysiluku(tili, false);
        qlonglong sentit = qRound( iter.value().toDouble() * 100 );
        data_[sarake].insert( ysiluku, data_.at(sarake).value(ysiluku,0) + sentit );
    }

    qDebug() << data_.at(sarake);

    tilausLaskuri_--;

    // Jos ollaan nollassa, niin sitten päästään kirjoittamaan ;)
    if( !tilausLaskuri_)
    {
        kirjoitaDatasta();
        emit valmis( rk );
    }
}


void Raportoija::kirjoitaYlatunnisteet()
{

    if( tyyppi() != KOHDENNUSLASKELMA && kohdennusKaytossa_.size() )
    {
        // Jos poimittu kohdennuksia, niin näyttään ne otsikossa jotta näkee että tämä on ote
        QStringList kohdennukset;
        for(int kohdId : kohdennusKaytossa_)
            kohdennukset.append( kp()->kohdennukset()->kohdennus( kohdId ).nimi() );
        rk.asetaOtsikko( rk.otsikko() + " (" + kohdennukset.join(",") + ")" );
    }

    // Jos raportissa erikoissarakkeita "budjetti", "budjettiero", "budjettiero%", niin niille oma rivi
    bool erikoissarakkeita = false;
    for(int i=0; i < sarakeTyypit_.count(); i++)
        if( sarakeTyypit_.value(i) != TOTEUTUNUT)
            erikoissarakkeita = true;

    rk.lisaaVenyvaSarake();
    for( int i=0; i < loppuPaivat_.count(); i++)
        rk.lisaaEurosarake();

    // CSV-kausiraportissa kuitenkin kaikki yhdelle riville
    if( onkoKausiraportti() )
    {
        RaporttiRivi csvrivi(RaporttiRivi::CSV);
        csvrivi.lisaa("");
        for(int i=0; i < alkuPaivat_.count(); i++)
        {
            QString tyyppiteksti = erikoissarakkeita ? sarakeTyyppiTeksti(i) : QString();
            csvrivi.lisaa( QString("%1 - %2 %3").arg( alkuPaivat_.at(i).toString("dd.MM.yyyy"))
                                              .arg( loppuPaivat_.at(i).toString("dd.MM.yyyy"))
                                              .arg( tyyppiteksti ), 1, true );
        }

        rk.lisaaOtsake(csvrivi);

        RaporttiRivi orivi(RaporttiRivi::EICSV);
        orivi.lisaa("");
        for(int i=0; i < alkuPaivat_.count(); i++)
            orivi.lisaa( QString("%1 -").arg( alkuPaivat_.at(i).toString("dd.MM.yyyy") ), 1, true );
        rk.lisaaOtsake(orivi);

    }
    // Tasepäivät tai loppupäivät
    RaporttiRivi olrivi(RaporttiRivi::EICSV);
    olrivi.lisaa("");
    for(int i=0; i < loppuPaivat_.count(); i++)
        olrivi.lisaa( loppuPaivat_.at(i).toString("dd.MM.yyyy"), 1, true );
    rk.lisaaOtsake(olrivi);


    if( erikoissarakkeita )
    {
        RaporttiRivi tyyppirivi(RaporttiRivi::EICSV);
        tyyppirivi.lisaa("");
        for(int i=0; i < sarakeTyypit_.count(); i++)
            tyyppirivi.lisaa( sarakeTyyppiTeksti(i), 1, true );
        rk.lisaaOtsake( tyyppirivi);
    }

}

void Raportoija::kirjoitaDatasta()
{

    QRegularExpression tiliRe("(?<alku>\\d{1,8})(\\.\\.)?(?<loppu>\\d{0,8})(?<menotulo>[+-]?)");

    // Välisummien käsittelyä = varten
    QVector<qlonglong> kokosumma( loppuPaivat_.count());
    QVector<qlonglong> budjettikokosumma( loppuPaivat_.count());

    QVariantList rivilista = kmap_.value("rivit").toList();

    for(QVariant riviVariant : rivilista)
    {
        QVariantMap map = riviVariant.toMap();

        QString kaava = map.value("L").toString();
        QString teksti = map.value(kieli_).toString();

        for(int i=0; i < map.value("V").toInt(); i++)
            rk.lisaaTyhjaRivi();

        qDebug() << "*" << kaava << "  |  " << teksti;

        RaporttiRivi rr;

        if( kaava.isEmpty() )
        {
            // Jos pelkkää tekstiä, niin se on sitten otsikko
            rr.lisaa( teksti );
            rk.lisaaRivi(rr);
            continue;
        }

        QString loppurivi = kaava;     // Aloittava tyhjä mukaan!

        // Lasketaan summat
        QVector<qlonglong> summat( loppuPaivat_.count() );
        QVector<qlonglong> budjetit( loppuPaivat_.count());

        int sisennys = map.value("S").toInt() * 4;

        RivinTyyppi rivityyppi = SUMMA;
        bool naytaTyhjarivi = kaava.contains('S') || kaava.contains('h');
        bool laskevalisummaan = !kaava.contains("==");
        bool lisaavalisumma = kaava.contains("=") && !kaava.contains("==");
        bool naytaErittely = kaava.contains('*');
        int erittelySisennys = 4;

        if( map.value("M").toString().contains("bold"))
            rr.lihavoi();
        if( kaava.contains("h"))
            rivityyppi = OTSIKKO;

        // Sisennys paikoilleen!
        QString sisennysStr;
        for( int i=0; i < sisennys; i++)
            sisennysStr.append(' ');

        rr.lisaa( sisennysStr + teksti );   // Lisätään teksti




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
            // Tässä voitaisiin käyttää vähän tehokkaampaa algoritmiä..

            for( int sarake = 0; sarake < data_.count(); sarake++)
            {
                QMapIterator<int,qlonglong> iter( data_.at(sarake));
                while( iter.hasNext())
                {
                    iter.next();
                    if( iter.key() >= alku )
                    {
                        if( iter.key() > loppu) // Päästy jo yli
                            break;

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

                QMapIterator<int,qlonglong> budjettiIter( budjetti_.value(sarake));
                while( budjettiIter.hasNext())
                {
                    budjettiIter.next();
                    if( budjettiIter.key() >= alku && budjettiIter.key() <= loppu )
                    {
                        if( vainTulot || vainMenot)
                        {
                            Tili tili = kp()->tilit()->tiliNumerolla( budjettiIter.key() / 10);

                            // Ohitetaan, jos haluttu vain tulot ja menot eikä ole niitä
                            if( (vainTulot && !tili.onko(TiliLaji::TULO) ) || (vainMenot && !tili.onko(TiliLaji::MENO) ))
                                    continue;
                        }

                        budjetit[sarake] += budjettiIter.value();

                        if( laskevalisummaan)
                            budjettikokosumma[sarake] += budjettiIter.value();  // Lisätään välisummaan
                    }
                }

            }
            if( lisaavalisumma )
            {
                // Välisumman lisääminen
                for(int sarake=0; sarake < data_.count(); sarake++)
                {
                    summat[sarake] += kokosumma.at(sarake);
                    budjetit[sarake] += budjettikokosumma.at(sarake);
                }
            }

            bool kirjauksia = false;
            // Selvitetään, jääkö summa nollaan
            for( int sarake = 0; sarake < data_.count(); sarake++)
            {
                if( summat.at(sarake) || budjetit.at(sarake))
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
            {
                // Since 1.1: Saraketyypin mukaisesti

                switch (sarakeTyypit_.at(sarake)) {

                case TOTEUTUNUT :
                    rr.lisaa( summat.at(sarake) , true );
                    break;
                case BUDJETTI:
                    rr.lisaa( budjetit.at(sarake), false);
                    break;
                case BUDJETTIERO:                    
                    rr.lisaa( summat.at(sarake) - budjetit.at(sarake), true );
                    break;
                case TOTEUMAPROSENTTI:
                    if( !budjetit.at(sarake))
                        rr.lisaa("");
                    else
                        rr.lisaa( 10000 * summat.at(sarake) / budjetit.at(sarake), true );

                }

            }
        }

        rk.lisaaRivi(rr);

        if( naytaErittely && erittelyt_ )
        {
            // eriSisennysStr on erittelyrivin aloitussisennys, joka *-rivillä kasvaa edellisen rivin sisennyksestä
            QString eriSisennysStr = sisennysStr;
            for( int i=0; i < erittelySisennys; i++)
                eriSisennysStr.append(' ');

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
                            switch (sarakeTyypit_.at(sarake)) {

                            case TOTEUTUNUT :
                                rr.lisaa( data_.at(sarake).value(iter.key(), 0) , true );
                                break;
                            case BUDJETTI:
                                rr.lisaa( budjetti_.at(sarake).value(iter.key(), 0), false);
                                break;
                            case BUDJETTIERO:
                                rr.lisaa( data_.at(sarake).value(iter.key(), 0) - budjetti_.at(sarake).value(iter.key(), 0), true );
                                break;
                            case TOTEUMAPROSENTTI:
                                if( !budjetti_.at(sarake).value(iter.key(), 0))
                                    rr.lisaa("");
                                else
                                    rr.lisaa( 10000 * data_.at(sarake).value(iter.key(), 0) / budjetti_.at(sarake).value(iter.key(), 0), true );
                            }

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
        if( sarakeTyypit_.value(i) != BUDJETTI )
        {

            QString kysymys = QString("SELECT ysiluku, sum(debetsnt), sum(kreditsnt) "
                                      "from vienti,tili where vienti.tili = tili.id and ysiluku > 300000000 "
                                      "and pvm between \"%1\" and \"%2\" "
                                      "group by ysiluku").arg( alkuPaivat_.at(i).toString(Qt::ISODate)).arg(loppuPaivat_.at(i).toString(Qt::ISODate));


            sijoitaTulosKyselyData( kysymys , i);
        }        
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

QString Raportoija::sarakeTyyppiTeksti(int sarake)
{
    switch (sarakeTyypit_.value(sarake))
    {
        case TOTEUTUNUT:
            return tr("Toteutunut");
        case BUDJETTI:
            return tr("Budjetti");
        case BUDJETTIERO:
            return tr("Budjettiero €");
        case TOTEUMAPROSENTTI:
            return tr("Toteutunut %");
    }
    return  QString();
}

void Raportoija::sijoitaBudjetti(int kohdennus)
{
    budjetti_.clear();
    budjetti_.resize( sarakeTyypit_.count() );

    for(int i=0; i < sarakeTyypit_.count(); i++)
    {
        qlonglong summa = 0;

        if( sarakeTyypit_.value(i) == TOTEUTUNUT)
        {
            continue;
        }

        for(int kausi=0; kausi < kp()->tilikaudet()->rowCount(QModelIndex()); kausi++ )
        {
            Tilikausi tilikausi = kp()->tilikaudet()->tilikausiIndeksilla(kausi);
            if( tilikausi.alkaa() > loppuPaivat_.value(i) || tilikausi.paattyy() < alkuPaivat_.value(i))
                continue;

            // Lisätään tämä tilikausi budjettiin
            QVariantMap kohdennusMap = tilikausi.json()->variant("Budjetti").toMap();
            QMapIterator<QString,QVariant> kohdennusIter(kohdennusMap);
            while( kohdennusIter.hasNext())
            {
                kohdennusIter.next();
                if( kohdennus > -1 && kohdennusIter.key().toInt() != kohdennus)
                    continue;   // Ohitetaan väärä kohdennus

                QVariantMap tiliMap = kohdennusIter.value().toMap();
                QMapIterator<QString, QVariant> tiliIter(tiliMap);

                while( tiliIter.hasNext())
                {
                    tiliIter.next();
                    int tilille = Tili::ysiluku( tiliIter.key().toInt() ) + 9;
                    qlonglong ennen = budjetti_[i].value( tilille, 0 );
                    qlonglong lisattava = tiliIter.value().toLongLong();
                    budjetti_[i].insert(tilille, ennen +  lisattava );
                    summa += lisattava;
                    tilitKaytossa_.insert( tilille, true );
                }
            }
        }
        budjetti_[i].insert(0, summa);
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

    for( int sarakeTyyppi : sarakeTyypit_ )
    {
        // Jos budjettiin liittyvä sarake, haetaan kaikki ne kohdennukset, joissa näinä aikoina
        // budjetti
        if( sarakeTyyppi != TOTEUTUNUT)
        {
            for(int i=0; i < loppuPaivat_.count(); i++)
            {
                // Lisätään budjetin kohdennukset näistä tilikausista
                for(int kausi=0; kausi < kp()->tilikaudet()->rowCount(QModelIndex()); kausi++ )
                {
                    Tilikausi tilikausi = kp()->tilikaudet()->tilikausiIndeksilla(kausi);
                    if( tilikausi.alkaa() > loppuPaivat_.value(i) || tilikausi.paattyy() < alkuPaivat_.value(i))
                        continue;

                    // Lisätään tämä tilikausi budjettiin
                    QVariantMap kohdennusMap = tilikausi.json()->variant("Budjetti").toMap();
                    QMapIterator<QString,QVariant> kohdennusIter(kohdennusMap);
                    while( kohdennusIter.hasNext())
                    {
                        kohdennusIter.next();
                        kohdennusKaytossa_.push_back(kohdennusIter.key().toInt());
                    }
                }
            }
            break;
        }
    }
}

void Raportoija::lisaaKohdennus(int kohdennusId)
{
    kohdennusKaytossa_.push_back( kohdennusId);
}

void Raportoija::kirjoita(bool tulostaErittelyt)
{
    erittelyt_ = tulostaErittelyt;
    data_.resize( loppuPaivat_.count() );
    kirjoitaYlatunnisteet();

    // Sitten tilataan tarvittava data
    if( tyyppi() == TASE ) {
        for(int i=0; i < loppuPaivat_.count(); i++) {
            tilausLaskuri_++;
            KpKysely* kysely = kpk("/saldot");
            kysely->lisaaAttribuutti("pvm",loppuPaivat_.at(i));
            kysely->lisaaAttribuutti("tase");
            connect(kysely, &KpKysely::vastaus,
                    [this,i] (QVariant* vastaus) { this->dataSaapuu(i, vastaus); });
            kysely->kysy();
        }
    } else if( tyyppi() == TULOSLASKELMA) {
        for( int i=0; i < loppuPaivat_.count(); i++) {
            tilausLaskuri_++;
            KpKysely* kysely = kpk("/saldot");
            kysely->lisaaAttribuutti("alkupvm", alkuPaivat_.value(i));
            kysely->lisaaAttribuutti("pvm", loppuPaivat_.value(i));
            kysely->lisaaAttribuutti("tuloslaskelma");
            connect(kysely, &KpKysely::vastaus,
                    [this,i] (QVariant* vastaus) { this->dataSaapuu(i, vastaus); });
            kysely->kysy();
        }
    }
}
