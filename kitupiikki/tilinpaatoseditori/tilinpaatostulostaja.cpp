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

#include <QDesktopServices>
#include <QFile>
#include <QPainter>

#include <QPdfWriter>

#include <QTextDocument>
#include <QAbstractTextDocumentLayout>

#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>

#include <QDebug>
#include <QApplication>

#include "tilinpaatostulostaja.h"
#include "db/kirjanpito.h"

#include "raportti/raportoija.h"

QByteArray TilinpaatosTulostaja::tulostaTilinpaatos(Tilikausi tilikausi, const QString& teksti)
{
    QByteArray barray;
    QBuffer buffer(&barray);
    buffer.open(QIODevice::WriteOnly);

    QPdfWriter writer( &buffer );

    writer.setPageSize( QPdfWriter::A4);
    writer.setCreator( QString("%1 %2").arg(qApp->applicationName()).arg(qApp->applicationVersion()) );
    writer.setTitle( Kirjanpito::tr("Tilinpäätös %1").arg(tilikausi.kausivaliTekstina()) );

    writer.setPageMargins( QMarginsF(25,10,10,10), QPageLayout::Millimeter );
    QPainter painter( &writer );

    tulostaKansilehti( tilikausi, &painter);
    int sivulla = 1;

    // Vertailutietoja varten
    Tilikausi edellinenKausi = kp()->tilikaudet()->tilikausiPaivalle( tilikausi.alkaa().addDays(-1) );

    // Raportit
    // Haetaan luetteloon merkityt raportit
    // Raportit on määritelty ensimmäisellä rivillä muodossa @Raportin nimi!Tulostettava otsikko@
    // Erittelyraportti puolestaan @Raportin nimi*Tulostettava otsikko@
    QString ekarivi = teksti.left( teksti.indexOf('\n') );
    QRegularExpression raporttiRe("@(?<raportti>.+?)(?<vertailu>\\$?)(?<erotin>[\\*!])(?<otsikko>.+?)@");
    raporttiRe.setPatternOptions(QRegularExpression::UseUnicodePropertiesOption);
    QRegularExpressionMatchIterator iter = raporttiRe.globalMatch(ekarivi);
    while( iter.hasNext() )
    {
        QRegularExpressionMatch mats = iter.next();
        QString raporttiNimi = mats.captured("raportti");
        QString otsikko = mats.captured("otsikko");

        Raportoija raportoija(raporttiNimi);
        if( raportoija.onkoKausiraportti() )
        {
            if( mats.captured("vertailu").isEmpty())
            {
                raportoija.lisaaKausi( tilikausi.alkaa(), tilikausi.paattyy());
                if( edellinenKausi.paattyy().isValid())
                    raportoija.lisaaKausi( edellinenKausi.alkaa(), edellinenKausi.paattyy());
            }
            else
            {
                if( !tilikausi.onkoBudjettia())
                    continue;   // Ei budjettivertailua, jos ei budjettia!

                // Budjettivertailu
                raportoija.lisaaKausi( tilikausi.alkaa(), tilikausi.paattyy(), Raportoija::TOTEUTUNUT);
                raportoija.lisaaKausi( tilikausi.alkaa(), tilikausi.paattyy(), Raportoija::BUDJETTI);
                raportoija.lisaaKausi( tilikausi.alkaa(), tilikausi.paattyy(), Raportoija::BUDJETTIERO);
                raportoija.lisaaKausi( tilikausi.alkaa(), tilikausi.paattyy(), Raportoija::TOTEUMAPROSENTTI);

            }

            if( raportoija.tyyppi() == Raportoija::KOHDENNUSLASKELMA)
                raportoija.etsiKohdennukset();
        }
        else
        {
            raportoija.lisaaTasepaiva( tilikausi.paattyy());
            if( edellinenKausi.paattyy().isValid())
                raportoija.lisaaTasepaiva(edellinenKausi.paattyy());

        }

        writer.newPage();

        RaportinKirjoittaja kirjoittaja = raportoija.raportti( mats.captured("erotin") == "*" );
        kirjoittaja.asetaOtsikko( otsikko );
        kirjoittaja.asetaKausiteksti( tilikausi.kausivaliTekstina() );
        sivulla += kirjoittaja.tulosta(&writer, &painter, false, sivulla);
    }

    // Liitetiedot, allekirjoitukset yms
    painter.setFont( QFont("Sans",10));
    int rivinkorkeus = painter.fontMetrics().height();
    RaportinKirjoittaja kirjoittaja;
    kirjoittaja.asetaOtsikko("TILINPÄÄTÖS");
    kirjoittaja.asetaKausiteksti( tilikausi.kausivaliTekstina());

    QTextDocument doc;
    // Sivutetaan niin, että ylätunniste mahtuu    

    QSizeF sivunkoko( painter.viewport().width()  ,  painter.viewport().height() - rivinkorkeus * 4 );

    doc.documentLayout()->setPaintDevice( painter.device() );
    doc.setPageSize( sivunkoko );
    doc.setHtml( teksti.mid(teksti.indexOf('\n')+1) );


    int pages = qRound( doc.size().height() / sivunkoko.height() + 1 );
    for( int i=0; i < pages; i++)
    {
        writer.newPage();
        painter.save();
        kirjoittaja.tulostaYlatunniste( &painter, sivulla);
        painter.drawLine(0,0,qRound(sivunkoko.width()),0);
        painter.translate(0, rivinkorkeus );

        painter.translate(0, 0 - i * sivunkoko.height() );

        doc.drawContents( &painter, QRectF(0, i * sivunkoko.height(),
                                           doc.textWidth(), sivunkoko.height() ) );
        painter.restore();
        sivulla++;
    }
    painter.end();

    return barray;
}

void TilinpaatosTulostaja::tulostaKansilehti(Tilikausi tilikausi, QPainter *painter)
{

    painter->save();
    painter->setFont(QFont("Sans",24,QFont::Bold));

    int sivunleveys = painter->window().width();
    int rivinkorkeus = painter->fontMetrics().height();
    int sivunkorkeus = painter->window().height();

    if( !kp()->logo().isNull()  )
    {
        double skaala = ((double) kp()->logo().width() ) / kp()->logo().height();
        double leveys = rivinkorkeus * 4 * skaala;
        double korkeus = rivinkorkeus * 4;

        if( leveys > sivunleveys * 10 / 11)
        {
            leveys = sivunleveys * 10 / 11;
            korkeus = leveys / skaala;
        }

        painter->drawImage( QRectF((sivunleveys - leveys) / 2, sivunkorkeus / 3 - rivinkorkeus * 4, leveys , korkeus),
                              kp()->logo() );
    }
    painter->drawText( QRectF(0, sivunkorkeus/3, sivunleveys, rivinkorkeus * 2), Qt::TextWordWrap | Qt::AlignCenter | Qt::AlignHCenter, kp()->asetukset()->asetus("Nimi"));

    painter->setFont(QFont("Sans",24));
    painter->drawText( QRectF(0, sivunkorkeus/3 + rivinkorkeus * 4, sivunleveys, rivinkorkeus ), Qt::TextWordWrap | Qt::AlignCenter | Qt::AlignHCenter, kp()->tr("Tilinpäätös") );
    painter->drawText( QRectF(0, sivunkorkeus/3 + rivinkorkeus * 5, sivunleveys, rivinkorkeus ), Qt::TextWordWrap | Qt::AlignCenter | Qt::AlignHCenter , tilikausi.kausivaliTekstina());

    painter->setFont(QFont("Sans",12));
    painter->drawText( QRectF(0,sivunkorkeus / 8 * 7, sivunleveys / 3, sivunkorkeus / 8), Qt::TextWordWrap, kp()->asetukset()->asetus("Osoite"));
    painter->drawText( QRectF( sivunleveys/3*2, sivunkorkeus / 8 * 7, sivunleveys / 3, rivinkorkeus), Qt::AlignLeft, kp()->tr("Y-tunnus: %1").arg(kp()->asetukset()->asetus("Ytunnus")));
    painter->drawText( QRectF( sivunleveys/3*2, sivunkorkeus / 8 * 7 + painter->fontMetrics().height(), sivunleveys / 3, rivinkorkeus), Qt::AlignLeft, kp()->tr("Kotipaikka: %1").arg(kp()->asetukset()->asetus("Kotipaikka")));


    painter->restore();
}
