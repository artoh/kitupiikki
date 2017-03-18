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
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>

#include <QDebug>

#include "tilinpaatostulostaja.h"
#include "db/kirjanpito.h"

#include "raportti/raportoija.h"

bool TilinpaatosTulostaja::tulostaTilinpaatos(Tilikausi tilikausi, QTextDocument *document, QPrinter *printer)
{
    QString tiedosto =  kp()->hakemisto().absoluteFilePath("arkisto/" + tilikausi.arkistoHakemistoNimi() + "/tilinpaatos.pdf" );
    if( QFile::exists(tiedosto))
            QFile(tiedosto).remove();

    // Pdf-tiedoston nimi
    printer->setOutputFileName( tiedosto );
    QPainter painter( printer );

    tulostaKansilehti( tilikausi, &painter);
    int sivulla = 1;
    printer->newPage();

    // Vertailutietoja varten
    Tilikausi edellinenKausi = kp()->tilikaudet()->tilikausiPaivalle( tilikausi.alkaa().addDays(-1) );

    // Tase
    Raportoija tase("Tase");
    tase.lisaaTasepaiva( tilikausi.paattyy() );
    if( edellinenKausi.paattyy().isValid())
        tase.lisaaTasepaiva( edellinenKausi.paattyy());

    RaportinKirjoittaja tasekirjoittaja = tase.raportti();
    tasekirjoittaja.asetaOtsikko("TILINPÄÄTÖS (Tase)");
    tasekirjoittaja.asetaKausiteksti( tilikausi.kausivaliTekstina());
    sivulla += tasekirjoittaja.tulosta(printer, &painter, false, sivulla);

    printer->newPage();

    // Tuloslaskelma
    Raportoija tuloslaskelma("Tuloslaskelma");
    tuloslaskelma.lisaaKausi( tilikausi.alkaa(), tilikausi.paattyy());
    if( edellinenKausi.paattyy().isValid())
        tuloslaskelma.lisaaKausi( edellinenKausi.alkaa(), edellinenKausi.paattyy());


    RaportinKirjoittaja tuloskirjoittaja = tuloslaskelma.raportti();
    tuloskirjoittaja.asetaOtsikko("TILINPÄÄTÖS (Tuloslaskelma)");
    tuloskirjoittaja.asetaKausiteksti( tilikausi.kausivaliTekstina());

    sivulla += tuloskirjoittaja.tulosta(printer, &painter, false, sivulla);

    // Liitetiedot, allekirjoitukset yms
    painter.setFont( QFont("Sans",10));
    int rivinkorkeus = painter.fontMetrics().height();
    tuloskirjoittaja.asetaOtsikko("TILINPÄÄTÖS");

    QTextDocument doc;
    // Sivutetaan niin, että ylätunniste mahtuu
    QSize sivunkoko( printer->pageRect().width(), printer->pageRect().height() - rivinkorkeus * 4 );

    doc.documentLayout()->setPaintDevice( painter.device());
    doc.setPageSize( sivunkoko );
    doc.setHtml( document->toHtml());


    int pages = doc.size().height() / sivunkoko.height() + 1;
    for( int i=0; i < pages; i++)
    {
        printer->newPage();
        painter.save();
        tuloskirjoittaja.tulostaYlatunniste( &painter, sivulla);
        painter.drawLine(0,0,sivunkoko.width(),0);
        painter.translate(0, rivinkorkeus );

        painter.translate(0, 0 - i * sivunkoko.height() );

        doc.drawContents( &painter, QRectF(0, i * sivunkoko.height(),
                                           doc.textWidth(), sivunkoko.height() ) );
        painter.restore();
        sivulla++;
    }
    return true;
}

void TilinpaatosTulostaja::tulostaKansilehti(Tilikausi tilikausi, QPainter *painter)
{

    painter->save();
    painter->setFont(QFont("Sans",24,QFont::Bold));

    int sivunleveys = painter->window().width();
    int rivinkorkeus = painter->fontMetrics().height();
    int sivunkorkeus = painter->window().height();

    if( QFile::exists(kp()->hakemisto().absoluteFilePath("logo.png")))
    {
        painter->drawPixmap( sivunleveys/2 - rivinkorkeus*2, sivunkorkeus / 3 - rivinkorkeus * 4, rivinkorkeus*4, rivinkorkeus*4, QPixmap( kp()->hakemisto().absoluteFilePath("logo.png") ) );
    }
    painter->drawText( QRectF(0, sivunkorkeus/3, sivunleveys, rivinkorkeus * 2), Qt::TextWordWrap | Qt::AlignCenter | Qt::AlignHCenter, kp()->asetukset()->asetus("Nimi"));

    painter->setFont(QFont("Sans",24));
    painter->drawText( QRectF(0, sivunkorkeus/3 + rivinkorkeus * 4, sivunleveys, rivinkorkeus ), Qt::TextWordWrap | Qt::AlignCenter | Qt::AlignHCenter, kp()->tr("Tilinpäätös") );
    painter->drawText( QRectF(0, sivunkorkeus/3 + rivinkorkeus * 5, sivunleveys, rivinkorkeus ), Qt::TextWordWrap | Qt::AlignCenter | Qt::AlignHCenter , tilikausi.kausivaliTekstina());
    painter->restore();
}
