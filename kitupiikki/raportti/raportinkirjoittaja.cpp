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

#include <QRect>
#include <QPainter>
#include "raportinkirjoittaja.h"

#include "db/kirjanpito.h"

#include <QDebug>

RaportinKirjoittaja::RaportinKirjoittaja()
{

}

void RaportinKirjoittaja::asetaOtsikko(const QString &otsikko)
{
    otsikko_ = otsikko;
}

void RaportinKirjoittaja::asetaKausiteksti(const QString &kausiteksti)
{
    kausiteksti_ = kausiteksti;
}

void RaportinKirjoittaja::lisaaSarake(const QString &leveysteksti)
{
    RaporttiSarake uusi;
    uusi.leveysteksti = leveysteksti;
    sarakkeet_.append(uusi);
}

void RaportinKirjoittaja::lisaaSarake(int leveysprosentti)
{
    RaporttiSarake uusi;
    uusi.leveysprossa = leveysprosentti;
    sarakkeet_.append(uusi);
}

void RaportinKirjoittaja::lisaaVenyvaSarake(int tekija)
{
    RaporttiSarake uusi;
    uusi.jakotekija = tekija;
    sarakkeet_.append(uusi);
}

void RaportinKirjoittaja::lisaaEurosarake()
{
    lisaaSarake("-9 999 999,99€ ");
}

void RaportinKirjoittaja::lisaaPvmSarake()
{
    lisaaSarake("99.99.9999 ");
}

void RaportinKirjoittaja::lisaaOtsake(RaporttiRivi otsikkorivi)
{
    otsakkeet_.append(otsikkorivi);
}

void RaportinKirjoittaja::lisaaRivi(RaporttiRivi rivi)
{
    rivit_.append(rivi);
}

void RaportinKirjoittaja::tulosta(QPrinter *printer, bool raidoita)
{
    if( rivit_.isEmpty())
        return;     // Ei tulostettavaa !

    QPainter painter(printer);
    painter.setFont(QFont("Sans",10));

    int rivinkorkeus = painter.fontMetrics().height();
    int sivunleveys = painter.window().width();
    int sivunkorkeus = painter.window().height();

    // Lasketaan sarakkeiden leveydet
    QVector<int> leveydet( sarakkeet_.count() );

    int tekijayhteensa = 0; // Lasketaan jäävän tilan jako
    int jaljella = sivunleveys;

    qDebug() << sivunleveys;

    for( int i=0; i < sarakkeet_.count(); i++)
    {
       int leveys = 0;
       if( !sarakkeet_[i].leveysteksti.isEmpty())
           leveys = painter.fontMetrics().width( sarakkeet_[i].leveysteksti );
       else if( sarakkeet_[i].leveysprossa)
           leveys = sivunleveys * sarakkeet_[i].leveysprossa / 100;
       else
           tekijayhteensa += sarakkeet_[i].jakotekija;

       leveydet[i] = leveys;
       jaljella -= leveys;

    }

    // Jaetaan vielä jäljellä oleva tila
    for( int i=0; i<sarakkeet_.count(); i++)
    {
        if( sarakkeet_[i].jakotekija)
        {
            leveydet[i] = jaljella * sarakkeet_[i].jakotekija / tekijayhteensa;
        }
    }

    // Nyt taulukosta löytyy sarakkeiden leveydet, ja tulostaminen
    // voidaan aloittaa

    int sivu = 1;
    int rivilla = 0;

    foreach (RaporttiRivi rivi, rivit_)
    {
        // Lasketaan ensin sarakkeiden rectit
        // ja samalla lasketaan taulukkoon liput

        QVector<QRect> laatikot( rivi.sarakkeita() );
        QVector<int> liput( rivi.sarakkeita() );

        int korkeinrivi = rivinkorkeus;
        int x = 0;  // Missä kohtaa ollaan leveyssuunnassa
        int sarake = 0; // Missä taulukon sarakkeessa ollaan menossa

        for(int i=0; i < rivi.sarakkeita(); i++)
        {
            int sarakeleveys = 0;
            // ysind (Yhdistettyjen Sarakkeiden Indeksi) kelaa ne sarakkeet läpi,
            // jotka tällä riville yhdistetty toisiinsa
            for( int ysind = 0; ysind < rivi.leveysSaraketta(i); ysind++ )
            {
                sarakeleveys += leveydet[sarake];
                sarake++;
            }

            // Nyt saatu tämän sarakkeen leveys

            int lippu = Qt::TextWordWrap;
            if( rivi.tasattuOikealle(i))
                lippu |= Qt::AlignRight;

            liput[i] = lippu;
            // Laatikoita ei asemoida korkeussuunnassa, vaan translatella liikutaan
            laatikot[i] = painter.boundingRect( x, 0,
                                                sarakeleveys, sivunkorkeus,
                                                lippu, rivi.teksti(i) );

            x += sarakeleveys;
            if( laatikot[i].height() > korkeinrivi )
                korkeinrivi = laatikot[i].height();
        }

        if( painter.transform().dy() > sivunkorkeus - korkeinrivi)
        {
            // Sivu tulee täyteen
            printer->newPage();
            sivu++;
            rivilla = 0;
            painter.restore();
        }

        if( painter.transform().dy() == 0)
        {
            // Ollaan sivun alussa

            painter.save();

            // Tulostetaan ylätunniste
            if( !otsikko_.isEmpty())
                tulostaYlatunniste( &painter, sivu);

            if( !otsakkeet_.isEmpty())
                painter.translate(0, rivinkorkeus);

            // Otsikkorivit
            foreach (RaporttiRivi otsikkorivi, otsakkeet_)
            {
                x = 0;
                sarake = 0;

                for( int i = 0; i < otsikkorivi.sarakkeita(); i++)
                {                     
                    int lippu = 0;
                    if( otsikkorivi.tasattuOikealle(i))
                        lippu = Qt::AlignRight;
                    int sarakeleveys = 0;

                    for( int ysind = 0; ysind < otsikkorivi.leveysSaraketta(i); ysind++ )
                    {
                        sarakeleveys += leveydet[sarake];
                        sarake++;
                    }
                    painter.drawText( QRect(x,0,sarakeleveys,rivinkorkeus),
                                      lippu, otsikkorivi.teksti(i));

                    x += sarakeleveys;
                }
                painter.translate(0, rivinkorkeus);
            } // Otsikkorivi
            if( !otsikko_.isEmpty() || !otsakkeet_.isEmpty())
                painter.drawLine(0,0,sivunleveys,0);
        }

        // Jos raidoitus, niin raidoitetaan eli osan rivien taakse harmaata
        if( raidoita && rivilla % 6 > 2)
        {
            painter.save();
            painter.setBrush(QBrush(Qt::lightGray));
            painter.setPen(Qt::NoPen);

            painter.drawRect(0,0,sivunleveys, korkeinrivi);

            painter.restore();

        }

        // Sitten tulostetaan tämä varsinainen rivi
        for( int i=0; i < rivi.sarakkeita(); i++)
        {
            painter.drawText( laatikot[i], liput[i] , rivi.teksti(i) );
        }
        painter.translate(0, korkeinrivi);
        rivilla++;
    }

    painter.restore();


}

void RaportinKirjoittaja::tulostaYlatunniste(QPainter *painter, int sivu)
{

    painter->setFont(QFont("Sans",10));

    int sivunleveys = painter->window().width();
    int rivinkorkeus = painter->fontMetrics().height();

    QString nimi = Kirjanpito::db()->asetus("Nimi");
    QString paivays = QDate::currentDate().toString(Qt::SystemLocaleShortDate);


    painter->drawText( QRect(0,0,sivunleveys/4, rivinkorkeus ), Qt::AlignLeft, nimi );
    painter->drawText( QRect(sivunleveys/4,0,sivunleveys/2, rivinkorkeus  ), Qt::AlignHCenter, otsikko_);
    painter->drawText( QRect(sivunleveys*3/4, 0, sivunleveys/4, rivinkorkeus), Qt::AlignRight, paivays);

    painter->translate(0, rivinkorkeus);

    QString ytunnus = Kirjanpito::db()->asetus("Ytunnus") ;
    QString sivustr = QString("Sivu %1").arg(sivu);

    painter->drawText(QRect(0,0,sivunleveys/4, rivinkorkeus ), Qt::AlignLeft, ytunnus );
    painter->drawText(QRect(sivunleveys/4,0,sivunleveys/2, rivinkorkeus  ), Qt::AlignHCenter, kausiteksti_);
    painter->drawText(QRect(sivunleveys*3/4, 0, sivunleveys/4, rivinkorkeus), Qt::AlignRight, sivustr);

    painter->translate(0, rivinkorkeus );

    painter->setPen(QPen(QBrush(Qt::black),1.00));

}
