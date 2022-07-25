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
#include <cmath>

#include <QRegularExpression>
#include <QRegularExpressionMatch>

#include "pdftuonti.h"

#include "validator/ibanvalidator.h"
#include "validator/viitevalidator.h"
#include "validator/ytunnusvalidator.h"
#include "tuontiapu.h"

#include "db/kirjanpito.h"
#include "db/tositetyyppimodel.h"
#include "laskutus/iban.h"

#include "tools/pdf/pdftoolkit.h"
#include "tools/pdf/pdfanalyzerpage.h"
#include "tools/pdf/pdfanalyzertext.h"

#include "pdftiliote/pdftiliotetuonti.h"

namespace Tuonti {

PdfTuonti::PdfTuonti()
{

}

QVariantMap PdfTuonti::tuo(const QByteArray &data)
{    
    PdfTuonti tuonti;

    tuonti.haeTekstit(data);

    if( tuonti.etsi("hyvityslasku",0,30))
        {}    // Hyvityslaskulle ei automaattista käsittelyä
    else if( tuonti.etsi("lasku",0,30) || tuonti.etsi("kuitti",0,30))
        return tuonti.tuoPdfLasku();
    else if( tuonti.etsi("tiliote",0,100)  || tuonti.etsi("account statement",0,30))
    {
       PdfAnalyzerDocument* doc = PdfToolkit::analyzer(data);
       PdfTilioteTuonti tilioteTuonti;
       QVariantMap tulos = tilioteTuonti.tuo(doc->allPages());
       delete doc;
       return tulos;
    }
    //    return tuonti.tuoPdfTiliote();

    return QVariantMap();
}

QVariantMap PdfTuonti::tuoPdfLasku()
{    
    QVariantMap data;
    if( etsi("kululasku",0,30))
        data.insert("tyyppi", TositeTyyppi::KULULASKU);
    else
        data.insert("tyyppi", TositeTyyppi::MENO );
    QSet<QString> ibanit;

    // Tutkitaan, onko tässä tilisiirtolomaketta
    if( etsi("Saajan", 125, 150, 0, 15) &&
        etsi("IBAN", 125, 140, 8, 16) &&
        etsi("Saaja", 135, 155, 0, 15) &&
        etsi("Viite", 150, 185, 40, 70) &&
        etsi("Erä", 160, 190, 40, 70) &&
        etsi("Euro", 160, 190, 60, 90) )
    {
        // Löytyy koko lailla sopivia kenttiä
        int ibansijainti = etsi("IBAN", 125, 140, 8, 30);
        QRegularExpression ibanRe("\\b[A-Z]{2}\\d{2}[\\w\\s]{6,30}\\b");


        for( const QString& t : haeLahelta( ibansijainti / 100 + 1, ibansijainti % 100 - 2, 10, 50))
        {
            if( ibanRe.match(t).hasMatch())
            {
                QString poimittu = ibanRe.match(t).captured(0);
                if( !data.contains("tilinumero")  &&  IbanValidator::kelpaako(poimittu) )
                {
                    QString iban = poimittu.remove(QRegularExpression("\\s"));
                    if( kp()->tilit()->tiliIbanilla(iban).onkoValidi())
                        data.insert("tyyppi", TositeTyyppi::TULO);
                    else
                        ibanit.insert(iban);
                }
            }
        }

        QStringList haettu;
        if( data.value("tyyppi").toInt() == TositeTyyppi::TULO) {
            int maksajasijainti = etsi("Maksajan", 130, 160, 0, 30);
            haettu = haeLahelta(maksajasijainti / 100, maksajasijainti % 100 + 5, 10, 10);
        } else {
            haettu = haeLahelta( ibansijainti / 100 + 11, ibansijainti % 100 - 2, 10, 10);
        }
        if(haettu.value(0).length() > 6 )
        {
            data.insert("kumppaninimi", haettu.value(0));
            QRegularExpression postiosoiteRe("(?<nro>\\d{5})\\s(?<kaupunki>\\w+)");
            QRegularExpressionMatch match = postiosoiteRe.match(haettu.value(2));
            if( match.hasMatch()) {
                data.insert("kumppaniosoite", haettu.value(1));
                data.insert("kumppanipostinumero", match.captured("nro"));
            }
        }


        int viitesijainti = etsi("Viite", 150, 185, 40, 70);

        for( QString t : haeLahelta( viitesijainti / 100, viitesijainti % 100, 20, 60) )
        {
            if(  !data.contains("viite")  && ViiteValidator::kelpaako(t) && t.length() < 21 )
            {
                data.insert("viite", t.remove(QRegularExpression("\\s")));
                break;
            }
        }

        int erapvmsijainti = etsi("Eräpäivä", 160, 190, 40, 70);
        for( const QString& t : haeLahelta( erapvmsijainti / 100 - 2, erapvmsijainti % 100 + 2, 10, 25))
        {
            QDate pvm = QDate::fromString(t,"dd.M.yyyy");
            if( pvm.isValid())
            {
                data.insert("erapvm", pvm);
                break;
            }
        }

        int eurosijainti = etsi("Euro",160,190,60,90);
        for( const QString& t : haeLahelta( eurosijainti / 100 - 2, eurosijainti % 100 + 2, 10, 25))
        {
            if( TuontiApu::sentteina(t))
            {
                data.insert("summa", TuontiApu::sentteina(t) / 100.0);
                break;
            }
        }

    }   // Ruudukosta

    // Laskun päiväys: Etsitään erilaisilla otsikoilla
    // Ensin yritetään etsiä erillinen toimituspäivämäär
    int pvmsijainti;
    if(  (pvmsijainti = etsi("Toimituspäivä")) || (pvmsijainti = etsi("Toimituspvm")) )
    {
        for( const QString& t : haeLahelta( pvmsijainti / 100, pvmsijainti % 100, 10, 70) )
        {
            QDate pvm = QDate::fromString(t,"dd.M.yyyy");
            if( pvm.isValid())
            {
                data.insert("toimituspvm", pvm);
                break;
            }
        }
    }
    // Sitten yritetään hakea laskun päivämäärää
    if(  (pvmsijainti = etsi("Päivämäärä"))  || (pvmsijainti = etsi("päiväys")) || ( (etsi("Eräp") != etsi("pvm")) &&  (pvmsijainti = etsi("pvm")) ))
    {
        for( const QString& t : haeLahelta( pvmsijainti / 100, pvmsijainti % 100, 10, 80) )
        {
            QDate pvm = QDate::fromString(t,"dd.M.yyyy");
            if( pvm.isValid())
            {
                data.insert("tositepvm", pvm);
                break;
            }
        }
    }

    if( !data.contains("erapvm") && etsi("Eräp"))
    {
        int erapvmsijainti = etsi("Eräp");
        for( const QString& t : haeLahelta( erapvmsijainti / 100, erapvmsijainti % 100, 10, 60) )
        {
            QDate pvm = QDate::fromString(t,"dd.M.yyyy");
            if( pvm.isValid())
            {
                data.insert("erapvm", pvm);
                break;
            }
        }
    }
    if( !data.contains("viite")  && etsi("viite"))
    {
        int viitesijainti = etsi("viite");        
        for( QString t : haeLahelta( viitesijainti / 100, viitesijainti % 100, 10, 60) )
        {
            if( ViiteValidator::kelpaako(t) )
            {
                data.insert("viite", t.remove(QRegularExpression("\\s")));
                break;
            }
        }
    }
    if( etsi("IBAN"))
    {
        int ibansijainti = etsi("IBAN");

        QRegularExpression ibanRe("[A-Z]{2}\\d{2}[\\w\\s]{6,34}");

        for( QString t : haeLahelta( ibansijainti / 100, ibansijainti % 100, 20, 10))
        {
            if( ibanRe.match(t).hasMatch())
            {
                if( IbanValidator::kelpaako(t))
                {
                    ibanit.insert( t.remove("\\s"));
                }
            }
        }
    }

    int numerosijainti = etsi("Laskun numero");
    if(!numerosijainti) numerosijainti = etsi("Lasku numero");
    QRegularExpression lnroRe("^[A-Z]{0,3}\\d{3,20}$");
    for(const QString& t : haeLahelta(numerosijainti / 100, numerosijainti % 100, 10, 50)) {
        if( lnroRe.match(t).hasMatch() ) {
            data.insert("laskunnumero", t);
            break;
        }
    }


    if( !data.contains("kumppaninimi") )
    {
        QStringList haetut;
        if( data.value("tyyppi").toInt() == TositeTyyppi::TULO)
            haetut = haeLahelta(15,0,50,40);
        else
            haetut = haeLahelta(0,0,15,40);
        haetut.append(tekstit_.values());
        while (!haetut.isEmpty() &&
               ( haetut.first().contains("lasku",Qt::CaseInsensitive) || haetut.first().contains( kp()->asetukset()->asetus("Nimi"), Qt::CaseInsensitive )))
            haetut.removeAt(0);
        if( !haetut.isEmpty())
            data.insert("kumppaninimi", haetut.first());
    }

    QRegularExpression rahaRe("^\\d{1,10}[,.]\\d{2}(\\s?€)?$");
    // Etsitään ensin yhteensä-rahasummia
    if( !data.contains("summa") )
    {
        QMapIterator<int,QString> rIter(tekstit_);
        while(rIter.hasNext()) {
            rIter.next();
            if( rIter.value().contains("yht", Qt::CaseInsensitive) && rIter.hasNext()) {
                int edrivi = rIter.key() / 100;
                rIter.next();
                if( rahaRe.match(rIter.value()).hasMatch() && edrivi == rIter.key() / 100)
                {
                    double rahaa = TuontiApu::sentteina( rahaRe.match(rIter.value()).captured(0) ) / 100.0;
                    if( rahaa > data.value("summa").toDouble() )
                        data.insert("summa", rahaa);
                }
            }
        }
    }

    if( !data.contains("summa") )
    {
        // Etsitään isoin senttiluku
        for( const QString& teksti : tekstit_.values())
        {
            if( rahaRe.match(teksti).hasMatch())
            {
                double rahaa = TuontiApu::sentteina( rahaRe.match(teksti).captured(0) ) / 100.0;
                if( rahaa > data.value("summa").toDouble() )
                    data.insert("summa", rahaa);
            }
        }
    }

    // Etsitään vielä y-tunnus
    QRegularExpression ytunnusRe("\\d{7}-\\d");
    for(auto teksti : tekstit_.values())
    {
        if( ytunnusRe.match(teksti).hasMatch())
        {
            QString tunnari = ytunnusRe.match(teksti).capturedTexts().first();
            if( YTunnusValidator::kelpaako(tunnari) &&
                    tunnari != kp()->asetukset()->asetus("Ytunnus"))
            {
                data.insert("kumppaniytunnus", tunnari);
            }
        }
    }


    if( !ibanit.isEmpty()) {
        QVariantList ibanlista;
        for(QString str : ibanit.toList()) {
            ibanlista << str;
        }
        data.insert("iban", ibanlista);
    }

    return data;



}

void PdfTuonti::haeTekstit(const QByteArray &data)
{
    // Tuottaa taulukon, jossa pdf-tiedoston tekstit suhteellisessa koordinaatistossa

    PdfAnalyzerDocument *pdfDoc = PdfToolkit::analyzer(data);

    for(int sivu = 0; sivu < pdfDoc->pageCount(); sivu++)
    {
        PdfAnalyzerPage pdfSivu = pdfDoc->page(sivu);

        qreal leveysKerroin = 100.0 / pdfSivu.size().width();
        qreal korkeusKerroin = 200.0 / pdfSivu.size().height();

        for(const auto& row : pdfSivu.rows()) {

            for(const auto& text : row.textList()){
                int sijainti = sivu * 20000 +
                        int( text.boundingRect().y() * korkeusKerroin) * 100 +
                        int( text.boundingRect().x() * leveysKerroin);
                QString raaka = text.text().simplified();

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
        }
/*
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
*/

    }
    delete pdfDoc;
}

QStringList PdfTuonti::haeLahelta(int y, int x, int dy, int dx)
{

    QMultiMap<int, QString> loydetyt;

    QMapIterator<int, QString> iter(tekstit_);

    while( iter.hasNext())
    {
        iter.next();
        int sijainti = iter.key();
        int sy = sijainti / 100;
        int sx = sijainti % 100;

        if( sy >= y-2 && sy < y + dy &&
            sx >= x-2 && sx < x + dx )
        {
            int ero = qAbs(x-sx) + qAbs(y-sy);
            loydetyt.insert( ero, iter.value());
        }
    }

    return loydetyt.values();
}

QList<int> PdfTuonti::sijainnit(const QString& teksti, int alkukorkeus, int loppukorkeus, int alkusarake, int loppusarake)
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

int PdfTuonti::etsi(const QString& teksti, int alkukorkeus, int loppukorkeus, int alkusarake, int loppusarake)
{
    QMapIterator<int, QString> iter(tekstit_);

    while( iter.hasNext())
    {
        iter.next();
        if( loppukorkeus && iter.key() >= loppukorkeus * 100)
             return 0;
        else if( iter.key() < alkukorkeus * 100)
            continue;
        else if( iter.value().contains(teksti, Qt::CaseInsensitive) &&
                 iter.key() % 100 >= alkusarake && iter.key() % 100 <= loppusarake)
            return iter.key();
    }
    return 0;
}

}
