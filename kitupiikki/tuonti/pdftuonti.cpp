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

#include <QRegularExpression>
#include <QRegularExpressionMatch>

#ifdef Q_OS_LINUX
    #include <poppler/qt5/poppler-qt5.h>
#elif defined(Q_OS_WIN)
    #include "poppler-qt5.h"
#endif


#include "pdftuonti.h"
#include "validator/ibanvalidator.h"
#include "validator/viitevalidator.h"

#include "kirjaus/kirjauswg.h"

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

        if( etsi("lasku",0,30))
            tuoLasku( pdfDoc, wg);

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

void PdfTuonti::tuoLasku(Poppler::Document *pdfDoc, KirjausWg *wg)
{

    QString tilinro;
    QString saaja;
    QString viite;
    QDate erapvm;
    qlonglong sentit = 0;
    QDate laskupvm;

    // Tutkitaan, onko tässä tilisiirtolomaketta
    if( etsi("Saajan", 125, 150, 0, 15) &&
        etsi("IBAN", 125, 140, 8, 16) &&
        etsi("Saaja", 135, 155, 0, 15) &&
        etsi("Viitenumero", 150, 185, 40, 70) &&
        etsi("Eräpäivä", 155, 190, 40, 70) &&
        etsi("Euro", 155, 190, 67, 90) )
    {
        // Löytyy koko lailla sopivia kenttiä
        int ibansijainti = etsi("IBAN", 125, 140, 8, 16);


        IbanValidator ibanValidoija;
        int pos = 0;

        QRegularExpression ibanRe("[A-Z]{2}\\d{2}\\w{6,30}");

        for( QString t : haeLahelta( ibansijainti / 100 + 1, ibansijainti % 100 - 2, 20, 10))
        {
            if( ibanRe.match(t).hasMatch())
            {
                QString tilinro = ibanRe.match(t).captured(0);
                if( tilinro.isEmpty() && ibanValidoija.validate(tilinro,pos) == IbanValidator::Acceptable)
                    tilinro = t;
            }
            else if( saaja.isEmpty() )
                saaja = t;
        }


        int viitesijainti = etsi("Viitenumero", 150, 185, 40, 70);

        ViiteValidator viiteValidoija;
        for( QString t : haeLahelta( viitesijainti / 100, viitesijainti % 100, 20, 50) )
        {
            if( viite.isEmpty() && viiteValidoija.validate(t,pos) == ViiteValidator::Acceptable)
                viite = t;
        }

        // Eräpvm

        // Summa

        // Laskupvm - pudotetaan alas haettavaksi ;)

    }


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

QStringList PdfTuonti::haeLahelta(int y, int x, int dy, int dx)
{
    // Lähellä on: -3 < Y < 15, -3 < X < 30
    QMap<int, QString> loydetyt;

    QMapIterator<int, QString> iter(tekstit_);

    while( iter.hasNext())
    {
        iter.next();
        int sijainti = iter.key();
        int sy = sijainti / 100;
        int sx = sijainti % 100;

        if( sy >= y && sy < y + dy &&
            sx >= x && sx < x + dx )
        {
            int ero = qAbs(x - sx) + qAbs( y - sy);
            loydetyt.insert( ero, iter.value());
        }
    }

    return loydetyt.values();
}

QList<int> PdfTuonti::sijainnit(QString teksti, int alkukorkeus, int loppukorkeus, int alkusarake, int loppusarake)
{
    QList<int> loydetyt;


    QMapIterator<int, QString> iter(tekstit_);

    while( iter.hasNext())
    {
        iter.next();
        if( iter.key() < alkukorkeus * 100)
            continue;
        if( loppukorkeus && iter.key() > loppukorkeus * 100)
            break;
        if( iter.value().contains(teksti, Qt::CaseInsensitive) &&
            iter.key() % 100 >= alkusarake && iter.key() % 100 <= loppusarake)
            loydetyt.append( iter.key());

    }
    return loydetyt;
}

int PdfTuonti::etsi(QString teksti, int alkukorkeus, int loppukorkeus, int alkusarake, int loppusarake)
{
    QMapIterator<int, QString> iter(tekstit_);

    while( iter.hasNext())
    {
        iter.next();
        if( iter.key() >= loppukorkeus * 100)
             return 0;
        else if( iter.key() < alkukorkeus * 100)
            continue;
        else if( iter.value().contains(teksti, Qt::CaseInsensitive) &&
                 iter.key() % 100 >= alkusarake && iter.key() % 100 <= loppusarake)
            return iter.key();
    }
    return 0;
}


