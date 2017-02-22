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

#include "raporittitesti.h"

#include <QSqlQuery>
#include <QStringList>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include "db/kirjanpito.h"

#include <QDebug>

/*
Entäpä jos raporttimuoto olisin näin päin niin saataisiin ehkä vielä selkeämmin?

Varsinainen toiminta                H   B
1. Tulot                            S       3
2. Kulut                            S       4,8
 a) Henkilöstökulut                 s       4..41
 b) Poistot                         s       8
 c) Muut kulut                      s       42..49

*/

RaporttiTesti::RaporttiTesti(const QString &nimike)
    : nimi(nimike)
{

}

RaportinKirjoittaja RaporttiTesti::raportti()
{
    QStringList lista = kp()->asetukset()->lista( "Raportti/" + nimi);
    QString optiot = lista.takeFirst();

    RaportinKirjoittaja kirjoittaja;
    kirjoittaja.asetaOtsikko( nimi );
    kirjoittaja.asetaKausiteksti("Kaikki - testiä");

    kirjoittaja.lisaaSarake(25);
    kirjoittaja.lisaaEurosarake();

    QMap<int,int> summat;
    if( optiot.contains("tulos"))
    {
        // Tehdään tulospuolta

        QSqlQuery query("SELECT ysiluku, sum(debetsnt), sum(kreditsnt) from vienti,tili where vienti.tili = tili.id and ysiluku > 300000000 group by ysiluku");
        while (query.next())
        {
            int ysiluku = query.value(0).toInt();
            int debet = query.value(1).toInt();
            int kredit = query.value(2).toInt();

            summat.insert( ysiluku, kredit - debet );
        }


        // Sitten raporttia
        // H/h S/s d   bold 2..3 1..3 4510    Tekstiä

        QRegularExpression re("(.+?)(\\s{3,}|\\t)(.+)");
        QRegularExpression valiRe("(\\d+)\\.\\.(\\d+)");

        // Vaihtoehtoinen välire, jolla myös yksinäinen: (\d+)(\.\.)?(\d*)


        foreach (QString rivi, lista)
        {
            if( rivi.isEmpty() )
            {
                // Lisätään tyhjä rivi
                kirjoittaja.lisaaRivi( RaporttiRivi() );
                continue;
            }

            QRegularExpressionMatch match = re.match(rivi);
            QStringList params = match.captured(1).split(' ');
            QString teksti = match.captured(3);

            int summa = 0;

            // Tehdään laskelma
            foreach (QString param, params)
            {

                if( !param.isEmpty() && param.at(0).isDigit())
                {

                    int alkaen = 0;
                    int loppuen = 0;

                    // On lukema
                    if( param.contains(".."))
                    {
                        QRegularExpressionMatch valiMatch = valiRe.match(param);
                        alkaen = Tili::ysiluku(valiMatch.captured(1).toInt(), false);
                        loppuen = Tili::ysiluku( valiMatch.captured(2).toInt(), true);
                    }
                    else
                    {
                        alkaen = Tili::ysiluku( param.toInt(), false);
                        loppuen = Tili::ysiluku( param.toInt(), true);
                    }




                    QMapIterator<int,int> iter(summat);
                    while( iter.hasNext())
                    {
                        iter.next();
                        if( iter.key() >= alkaen && iter.key() <= loppuen)
                            summa += iter.value();      // Lisättiin summa summarumiin!
                    }

                }
                // Tähän tulee kaikkiin mahdollisiin muihin valintoihin ja fonttimääreisiin ja muihin liittyvät leikit
            }
            // Nyt rivi on valmis

            if( params.first().at(0).isLower() )
            {
                // Tulostetaan vain jos on summaa
                if( summa == 0)
                    continue;
            }

            // Ok, sitten lisätään
            RaporttiRivi rrivi;
            rrivi.lisaa(teksti);
            rrivi.lisaa( summa );
            kirjoittaja.lisaaRivi(rrivi);

        }


    }

    return kirjoittaja;
}
