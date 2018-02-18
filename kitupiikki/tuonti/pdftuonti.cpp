/*
   Copyright (C) 2018 Arto Hyvättinen

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


#include <QDebug>
#include <QFile>
#include <QByteArray>
#include <QMap>
#include <QSet>

#include "pdftuonti.h"

#ifdef Q_OS_LINUX
    #include <poppler/qt5/poppler-qt5.h>
#elif defined(Q_OS_WIN)
    #include "poppler-qt5.h"
#endif


PdfTuonti::PdfTuonti()
{

}

bool PdfTuonti::tuoTiedosto(const QString &tiedostonnimi, KirjausWg *wg)
{
    QFile tiedosto(tiedostonnimi);
    tiedosto.open( QIODevice::ReadOnly );

    QByteArray bytes = tiedosto.readAll();
    tiedosto.close();

    Poppler::Document *pdfDoc = Poppler::Document::loadFromData( bytes );

    if( pdfDoc )
    {
        haeTekstit(pdfDoc);

        Poppler::Page *pdfsivu = pdfDoc->page(0);
        if( pdfsivu)
        {

            QString ylateksti = pdfsivu->text(QRectF(0, 0, pdfsivu->pageSize().width(), 72 * 5 ));
            if( ylateksti.contains("tiliote", Qt::CaseInsensitive))
                qDebug() << "Tiliote";
            else if( ylateksti.contains("lasku", Qt::CaseInsensitive))
                qDebug() << "Lasku";

        }
        delete pdfsivu;

        QMapIterator<int,QString> iter(tekstit_);
        while( iter.hasNext())
        {
            iter.next();
            int s = iter.key();
            qDebug() << s / 100 << " | " << s % 100 << "  " << iter.value();
        }

    }


    delete pdfDoc;

    return true;
}

void PdfTuonti::haeTekstit(Poppler::Document *pdfDoc)
{
    // Tuottaa taulukon, jossa pdf-tiedoston tekstit suhteellisessa koordinaatistossa

    for(int sivu = 0; sivu < pdfDoc->numPages(); sivu++)
    {
        Poppler::Page *pdfSivu = pdfDoc->page(sivu);

        qreal leveysKerroin = 100.0 / pdfSivu->pageSizeF().width();
        qreal korkeusKerroin = 200.0 / pdfSivu->pageSizeF().height();

        QSet<Poppler::TextBox*> kasitellyt;

        for( Poppler::TextBox* box : pdfSivu->textList())
        {
            if( kasitellyt.contains(box))
                continue;

            QString teksti = box->text();

            // Sivu jaetaan vaakasuunnassa 100 ja pystysuunnassa 200 loogiseen yksikköön

            int sijainti = sivu * 20000 +
                           int( box->boundingBox().y() * korkeusKerroin) * 100  +
                           int( box->boundingBox().x() * leveysKerroin );


            Poppler::TextBox *seuraava = box->nextWord();
            while( seuraava )
            {
                teksti.append(' ');
                kasitellyt.insert(seuraava);    // Jotta ei lisättäisi myös itsenäisesti
                teksti.append( seuraava->text());
                seuraava = seuraava->nextWord();
            }

            // Käsitellään vielä vähän tekstiä
            QString raaka = teksti.simplified();
            QString tulos;
            for(int i = 0; i < raaka.length(); i++)
            {
                // Poistetaan numeroiden välissä olevat välit
                // sekä numeron ja +/- merkin välissä oleva väli
                // Näin saadaan tilinumerot ja valuutasummat tiiviiksi

                QChar merkki = raaka.at(i);

                if( i > 0 && i < raaka.length() - 1 && merkki.isSpace())
                {
                    QChar ennen = raaka.at(i-1);
                    QChar jalkeen = raaka.at(i+1);

                    if( (ennen.isDigit() || jalkeen.isDigit()) &&
                        (ennen.isDigit() || ennen == '-' || ennen == '+') &&
                        (jalkeen.isDigit() || jalkeen == '-' || jalkeen == '+') )
                        continue;
                }
                tulos.append(merkki);
            }

            tekstit_.insert(sijainti, tulos );

        }
        delete pdfSivu;
    }
}


