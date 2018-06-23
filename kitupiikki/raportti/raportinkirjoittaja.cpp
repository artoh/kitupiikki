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

#include <QRect>
#include <QPainter>
#include <QFile>
#include <QFont>
#include <QPixmap>
#include <QSettings>
#include <QApplication>
#include "raportinkirjoittaja.h"

#include <QPdfWriter>

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
    lisaaSarake("-9 999 999,99€XX");
}

void RaportinKirjoittaja::lisaaPvmSarake()
{
    lisaaSarake("99.99.9999XX");
}

void RaportinKirjoittaja::lisaaOtsake(RaporttiRivi otsikkorivi)
{
    otsakkeet_.append(otsikkorivi);
}

void RaportinKirjoittaja::lisaaRivi(RaporttiRivi rivi)
{
    rivit_.append(rivi);
}

void RaportinKirjoittaja::lisaaTyhjaRivi()
{
    if( rivit_.count())
        if( rivit_.last().sarakkeita() )
            rivit_.append( RaporttiRivi());
}

int RaportinKirjoittaja::tulosta(QPagedPaintDevice *printer, QPainter *painter, bool raidoita, int alkusivunumero)
{
    if( rivit_.isEmpty())
        return 0;     // Ei tulostettavaa !

    QFont fontti("Sans", 10);
    painter->setFont(fontti);

    int rivinkorkeus = painter->fontMetrics().height();
    int sivunleveys = painter->window().width();
    int sivunkorkeus = painter->window().height();

    // Lasketaan sarakkeiden leveydet
    QVector<int> leveydet( sarakkeet_.count() );

    int tekijayhteensa = 0; // Lasketaan jäävän tilan jako
    int jaljella = sivunleveys;

    qDebug() << sivunleveys;

    for( int i=0; i < sarakkeet_.count(); i++)
    {
       int leveys = 0;
       if( !sarakkeet_[i].leveysteksti.isEmpty())
           leveys = painter->fontMetrics().width( sarakkeet_[i].leveysteksti );
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

    if( tekijayhteensa )
        jaljella = 0;   // Koko tila käytetty venyvällä sarakkeella

    // Nyt taulukosta löytyy sarakkeiden leveydet, ja tulostaminen
    // voidaan aloittaa

    int sivu = 1;
    int rivilla = 0;

    foreach (RaporttiRivi rivi, rivit_)
    {
        fontti.setPointSize( rivi.pistekoko() );
        fontti.setBold( rivi.onkoLihava() );
        painter->setFont(fontti);

        // Lasketaan ensin sarakkeiden rectit
        // ja samalla lasketaan taulukkoon liput

        QVector<QRect> laatikot( rivi.sarakkeita() );
        QVector<int> liput( rivi.sarakkeita() );
        QVector<QString> tekstit( rivi.sarakkeita() );

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
                sarakeleveys += leveydet.at(sarake);
                sarake++;
            }

            // Nyt saatu tämän sarakkeen leveys

            int lippu = Qt::TextWordWrap;
            QString teksti = rivi.teksti(i);
            if( rivi.tasattuOikealle(i))
            {
                lippu |= Qt::AlignRight;
                teksti.append("  ");
                // Ei tasata ihan oikealle vaan välilyönnin päähän
            }
            tekstit[i] = teksti;

            liput[i] = lippu;
            // Laatikoita ei asemoida korkeussuunnassa, vaan translatella liikutaan
            laatikot[i] = painter->boundingRect( x, 0,
                                                sarakeleveys, sivunkorkeus,
                                                lippu, teksti );

            x += sarakeleveys;
            if( laatikot[i].height() > korkeinrivi )
                korkeinrivi = laatikot[i].height();
        }

        if( painter->transform().dy() > sivunkorkeus - korkeinrivi)
        {
            // Sivu tulee täyteen
            printer->newPage();
            sivu++;
            rivilla = 0;
            painter->restore();
        }

        if( painter->transform().dy() == 0)
        {
            // Ollaan sivun alussa

            painter->save();

            // Tulostetaan ylätunniste
            if( !otsikko_.isEmpty())
                tulostaYlatunniste( painter, sivu + alkusivunumero - 1);

            if( !otsakkeet_.isEmpty())
                painter->translate(0, rivinkorkeus);

            // Otsikkorivit
            foreach (RaporttiRivi otsikkorivi, otsakkeet_)
            {
                x = 0;
                sarake = 0;

                for( int i = 0; i < otsikkorivi.sarakkeita(); i++)
                {                     
                    int lippu = 0;
                    QString teksti = otsikkorivi.teksti(i);

                    if( otsikkorivi.tasattuOikealle(i))
                    {
                        lippu = Qt::AlignRight;
                        teksti.append("  ");
                    }
                    int sarakeleveys = 0;

                    for( int ysind = 0; ysind < otsikkorivi.leveysSaraketta(i); ysind++ )
                    {
                        sarakeleveys += leveydet[sarake];
                        sarake++;
                    }
                    painter->drawText( QRect(x,0,sarakeleveys,rivinkorkeus),
                                      lippu, teksti );

                    x += sarakeleveys;
                }
                painter->translate(0, rivinkorkeus);
            } // Otsikkorivi
            if( !otsikko_.isEmpty() || !otsakkeet_.isEmpty())
                painter->drawLine(0,0,sivunleveys,0);
        }

        // Jos raidoitus, niin raidoitetaan eli osan rivien taakse harmaata
        if( raidoita && rivilla % 6 > 2)
        {
            painter->save();
            painter->setBrush(QBrush(QColor(222,222,222)));
            painter->setPen(Qt::NoPen);

            painter->drawRect(0,0,sivunleveys, korkeinrivi);

            painter->restore();

        }

        fontti.setPointSize( rivi.pistekoko());
        fontti.setBold( rivi.onkoLihava() );
        painter->setFont(fontti);

        // Sitten tulostetaan tämä varsinainen rivi
        for( int i=0; i < rivi.sarakkeita(); i++)
        {
            painter->drawText( laatikot[i], liput[i] , tekstit[i] );
        }
        if( rivi.onkoViivaa())  // Viivan tulostaminen rivin ylle
        {
            painter->drawLine(0,0, sivunleveys - jaljella , 0);
        }

        painter->translate(0, korkeinrivi);
        rivilla++;
    }

    painter->restore();

    return sivu;
}

QString RaportinKirjoittaja::html(bool linkit)
{
    QString txt;

    txt.append("<html><meta charset=\"utf-8\"><title>");
    txt.append( otsikko() );
    txt.append("</title>"
               "<style>"
               " body { font-family: Helvetica; }"
               " h1 { font-weight: normal; }"
               " .lihava { font-weight: bold; } "
               " tr.viiva td { border-top: 1px solid black; }"
               " td.oikealle { text-align: right; } "
               " th { text-align: left; color: darkgray;}"
               " a { text-decoration: none; color: black; }"
               " td { padding-right: 2em; }"
               " td:last-of-type { padding-right: 0; }"
               " table { border-collapse: collapse;}"
               " p.tulostettu { margin-top:2em; color: darkgray; }"
               "</style>"
               "</head><body>");

    txt.append("<h1>" + otsikko() + "</h1>");
    txt.append("<p>" + kp()->asetukset()->asetus("Nimi") + "<br>");
    txt.append( kausiteksti() + "</p>");
    txt.append("<table width=100%><thead>\n");

    // Otsikkorivit
    foreach (RaporttiRivi otsikkorivi, otsakkeet_ )
    {
        txt.append("<tr>");
        for(int i=0; i < otsikkorivi.sarakkeita(); i++)
        {
            txt.append(QString("<th colspan=%1>").arg( otsikkorivi.leveysSaraketta(i)));
            txt.append( otsikkorivi.teksti(i));
            txt.append("</th>");
        }
        txt.append("</tr>\n");
    }

    txt.append("</thead>\n");
    // Rivit
    foreach (RaporttiRivi rivi, rivit_)
    {
        QStringList trluokat;
        if( rivi.onkoLihava())
            trluokat << "lihava";
        if( rivi.onkoViivaa())
            trluokat << "viiva";

        if( trluokat.isEmpty())
            txt.append("<tr>");
        else
            txt.append("<tr class=\"" + trluokat.join(' ') + "\">");

        if( !rivi.sarakkeita())
            txt.append("<td>&nbsp;</td>"); // Tyhjätkin rivit näkyviin!

        for(int i=0; i < rivi.sarakkeita(); i++)
        {
            if( rivi.tasattuOikealle(i) )
                txt.append(QString("<td colspan=%1 class=oikealle>").arg(rivi.leveysSaraketta(i)));
            else
                txt.append(QString("<td colspan=%1>").arg(rivi.leveysSaraketta(i)));

            if(linkit)
            {
                if( rivi.sarake(i).linkkityyppi == RaporttiRiviSarake::TOSITE_ID)
                {
                    // Linkki tositteeseen
                    txt.append( QString("<a href=\"%1.html\">").arg( rivi.sarake(i).linkkidata , 8, 10 , QChar('0') ) );
                }
                else if( rivi.sarake(i).linkkityyppi == RaporttiRiviSarake::TILI_NRO)
                {
                    // Linkki tiliin
                    txt.append( QString("<a href=\"paakirja.html#%2\">").arg( rivi.sarake(i).linkkidata));
                }
                else if( rivi.sarake(i).linkkityyppi == RaporttiRiviSarake::TILI_LINKKI)
                {
                    // Nimiö dataan
                    txt.append( QString("<a name=\"%1\">").arg( rivi.sarake(i).linkkidata));
                }
            }
            QString tekstia = rivi.teksti(i);
            tekstia.replace(' ', "&nbsp;");
            tekstia.replace('\n', "<br>");

            txt.append(  tekstia );

            if( linkit && rivi.sarake(i).linkkityyppi )
                txt.append("</a>");

            txt.append("&nbsp;</td>");
        }
        txt.append("</tr>\n");

    }
    txt.append("</table>");
    txt.append("<p class=tulostettu>Tulostettu " + QDate::currentDate().toString("dd.MM.yyyy"));
    if( kp()->onkoHarjoitus())
        txt.append("<br>Kirjanpito on laadittu Kitupiikki-ohjelman harjoittelutilassa");

    txt.append("</p></body></html>\n");

    return txt;
}

QByteArray RaportinKirjoittaja::pdf(bool taustaraidat, bool tulostaA4)
{
    QByteArray array;
    QBuffer buffer(&array);
    buffer.open(QIODevice::WriteOnly);

    QPdfWriter writer(&buffer);
    writer.setCreator( QString("Kitupiikki %1").arg( qApp->applicationVersion() ) );
    writer.setTitle( otsikko() );

    if( tulostaA4 )
        writer.setPageSize( QPdfWriter::A4 );
    else
        writer.setPageLayout( kp()->printer()->pageLayout() );

    QPainter painter( &writer );

    tulosta( &writer, &painter, taustaraidat );
    painter.end();

    return array;

}

QByteArray RaportinKirjoittaja::csv()
{
    QSettings settings;
    QChar erotin = settings.value("CsvErotin", QChar(',')).toChar();

    QString txt;

    for( RaporttiRivi otsikko : otsakkeet_)
    {
        QStringList otsakkeet;
        for(int i=0; i < otsikko.sarakkeita(); i++)
            otsakkeet.append( otsikko.csv(i));
        txt.append( otsakkeet.join(erotin));
    }
    for( RaporttiRivi rivi : rivit_ )
    {
        if( rivi.sarakkeita() )
        {

            txt.append("\r\n");
            QStringList sarakkeet;
            for( int i=0; i < rivi.sarakkeita(); i++)
                sarakkeet.append( rivi.csv(i));
            txt.append( sarakkeet.join(erotin));
        }
    }

    if( settings.value("CsvKoodaus").toString() == "latin1")
    {
        txt.replace("€","EUR");
        return txt.toLatin1();
    }
    else
        return txt.toUtf8();
}

void RaportinKirjoittaja::tulostaYlatunniste(QPainter *painter, int sivu)
{

    painter->setFont(QFont("Sans",10));

    int sivunleveys = painter->window().width();
    int rivinkorkeus = painter->fontMetrics().height();

    QString nimi = Kirjanpito::db()->asetus("Nimi");
    QString paivays = kp()->paivamaara().toString("dd.MM.yyyy");

    int vasenreunus = 0;

    if( !kp()->logo().isNull() )
    {
        double skaala = ((double) kp()->logo().width()) / kp()->logo().height();
        double skaalattu = skaala < 5.0 ? skaala : 5.0;
        painter->drawPixmap( QRect(0,0,rivinkorkeus*2*skaalattu, rivinkorkeus*2), QPixmap::fromImage( kp()->logo() ) );
        vasenreunus = rivinkorkeus * 2 * skaalattu + painter->fontMetrics().width("A");
    }

    QRectF nimiRect = painter->boundingRect( vasenreunus, 0, sivunleveys / 3 - vasenreunus, painter->viewport().height(),
                                             Qt::TextWordWrap, nimi );
    QRectF otsikkoRect = painter->boundingRect( sivunleveys/3, 0, sivunleveys / 3, painter->viewport().height(),
                                                Qt::AlignHCenter | Qt::TextWordWrap, otsikko());

    painter->drawText( nimiRect, Qt::AlignLeft | Qt::TextWordWrap, nimi );
    painter->drawText( otsikkoRect, Qt::AlignHCenter | Qt::TextWordWrap, otsikko());
    painter->drawText( QRect(sivunleveys*2/3, 0, sivunleveys/3, rivinkorkeus), Qt::AlignRight, paivays);

    if( kp()->asetukset()->onko("Harjoitus")  )
    {
        painter->save();
        painter->setPen( QPen(Qt::red));
        painter->setFont( QFont("Sans",14));
        painter->drawText(QRect(vasenreunus + sivunleveys / 8 * 5,0,sivunleveys/4, rivinkorkeus*2 ), Qt::AlignHCenter | Qt::AlignVCenter, QString("HARJOITUS") );
        painter->restore();
    }

    painter->translate(0, nimiRect.height() > otsikkoRect.height() ? nimiRect.height() : otsikkoRect.height() );

    QString ytunnus = Kirjanpito::db()->asetus("Ytunnus") ;    

    painter->drawText(QRect(vasenreunus,0,sivunleveys/3, rivinkorkeus ), Qt::AlignLeft, ytunnus );

    painter->drawText(QRect(sivunleveys/3,0,sivunleveys/3, rivinkorkeus  ), Qt::AlignHCenter, kausiteksti_);
    if( sivu )
        painter->drawText(QRect(sivunleveys*2/3, 0, sivunleveys/3, rivinkorkeus), Qt::AlignRight, QString("Sivu %1").arg(sivu));



    painter->translate(0, rivinkorkeus );

    painter->setPen(QPen(QBrush(Qt::black),1.00));

}
