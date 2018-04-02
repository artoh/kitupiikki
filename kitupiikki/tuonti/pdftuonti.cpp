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


PdfTuonti::PdfTuonti(KirjausWg *wg) :
    Tuonti( wg )
{

}

bool PdfTuonti::tuo(const QByteArray &data)
{
    Poppler::Document *pdfDoc = Poppler::Document::loadFromData( data );

    if( pdfDoc )
    {
        haeTekstit(pdfDoc);

        if( etsi("hyvityslasku",0,30))
            {;}    // Hyvityslaskulle ei automaattista käsittelyä
        else if( etsi("lasku",0,30))
            tuoPdfLasku();
        else if( etsi("tiliote",0,30))
            tuoPdfTiliote();

    }

    delete pdfDoc;

    return true;
}

void PdfTuonti::tuoPdfLasku()
{

    QString tilinro;
    QString saaja;
    QString viite;
    QDate erapvm;
    qlonglong sentit = 0;
    QDate laskupvm;
    QDate toimituspvm;
    QRegularExpression rahaRe("\\d{1,10}[,]\\d{2}");


    // Tutkitaan, onko tässä tilisiirtolomaketta
    if( etsi("Saajan", 125, 150, 0, 15) &&
        etsi("IBAN", 125, 140, 8, 16) &&
        etsi("Saaja", 135, 155, 0, 15) &&
        etsi("Viitenumero", 150, 185, 40, 70) &&
        etsi("Eräpäivä", 155, 190, 40, 70) &&
        etsi("Euro", 155, 190, 67, 90) )
    {
        // Löytyy koko lailla sopivia kenttiä
        int ibansijainti = etsi("IBAN", 125, 140, 8, 30);



        QRegularExpression ibanRe("\\b[A-Z]{2}\\d{2}[\\w\\s]{6,30}\\b");


        for( QString t : haeLahelta( ibansijainti / 100 + 1, ibansijainti % 100 - 2, 10, 50))
        {
            if( ibanRe.match(t).hasMatch())
            {
                QString poimittu = ibanRe.match(t).captured(0);
                if( tilinro.isEmpty() &&  IbanValidator::kelpaako(poimittu) )
                    tilinro = poimittu.remove(QRegularExpression("\\s"));
            }
        }

        for( QString t : haeLahelta( ibansijainti / 100 + 11, ibansijainti % 100 - 2, 10, 10))
        {
            if( t.length() > 5 && !t.contains(ibanRe))
            {
                saaja = t;
                break;
            }
        }


        int viitesijainti = etsi("Viitenumero", 150, 185, 40, 70);

        for( QString t : haeLahelta( viitesijainti / 100, viitesijainti % 100, 20, 60) )
        {
            if( viite.isEmpty() && ViiteValidator::kelpaako(t) )
                viite = t.remove(QRegularExpression("\\s"));
            else
            {
                QDate pvm = QDate::fromString(t,"dd.M.yyyy");
                if( pvm.isValid())
                    erapvm = pvm;
                if( rahaRe.match(t).hasMatch())
                {
                    QString rahaa = rahaRe.match(t).captured(0);
                    rahaa.remove(',');
                    // Jäljelle jää senttimäärä
                    sentit = rahaa.toLongLong();
                }
            }
        }

    }

    // Laskun päiväys: Etsitään erilaisilla otsikoilla
    // Ensin yritetään etsiä erillinen toimituspäivämäär
    int pvmsijainti;
    if(  (pvmsijainti = etsi("Toimituspäivä")) || (pvmsijainti = etsi("Toimituspvm")) )
    {
        for( QString t : haeLahelta( pvmsijainti / 100, pvmsijainti % 100, 10, 60) )
        {
            QDate pvm = QDate::fromString(t,"dd.M.yyyy");
            if( pvm.isValid())
            {
                toimituspvm = pvm;
                break;
            }
        }
    }
    // Sitten yritetään hakea laskun päivämäärää
    if(  (pvmsijainti = etsi("Päivämäärä")) || (pvmsijainti == etsi("pvm")) || (pvmsijainti = etsi("päiväys")))
    {
        for( QString t : haeLahelta( pvmsijainti / 100, pvmsijainti % 100, 10, 60) )
        {
            QDate pvm = QDate::fromString(t,"dd.M.yyyy");
            if( pvm.isValid())
            {
                laskupvm = pvm;
                break;
            }
        }
    }

    if( !erapvm.isValid() && etsi("Eräp"))
    {
        int erapvmsijainti = etsi("Eräp");
        for( QString t : haeLahelta( erapvmsijainti / 100, erapvmsijainti % 100, 10, 60) )
        {
            QDate pvm = QDate::fromString(t,"dd.M.yyyy");
            if( pvm.isValid())
            {
                erapvm = pvm;
                break;
            }
        }
    }
    if( viite.isEmpty() && etsi("viite"))
    {
        int viitesijainti = etsi("viite");        
        for( QString t : haeLahelta( viitesijainti / 100, viitesijainti % 100, 10, 60) )
        {
            if( ViiteValidator::kelpaako(t) )
            {
                viite = t.remove(QRegularExpression("\\s"));
                break;
            }
        }
    }
    if( tilinro.isEmpty() && etsi("IBAN"))
    {
        int ibansijainti = etsi("IBAN");

        QRegularExpression ibanRe("[A-Z]{2}\\d{2}[\\w\\s]{6,34}");

        for( QString t : haeLahelta( ibansijainti / 100, ibansijainti % 100, 20, 10))
        {
            if( ibanRe.match(t).hasMatch())
            {
                QString tilinro = ibanRe.match(t).captured(0);
                if( IbanValidator::kelpaako(t))
                {
                    tilinro = t.remove("\\s");
                    break;
                }
            }
        }
    }
    if( saaja.isEmpty() && tekstit_.isEmpty())
    {
        saaja = tekstit_.values().first();
    }
    if( !sentit )
    {
        // Etsitään isoin senttiluku
        for( QString teksti : tekstit_.values())
        {
            if( rahaRe.match(teksti).hasMatch())
            {
                QString rahaa = rahaRe.match(teksti).captured(0);
                rahaa.remove(',');
                // Jäljelle jää senttimäärä
                if( rahaa.toLongLong() > sentit)
                    sentit = rahaa.toLongLong();
            }
        }
    }

    tuoLasku( sentit, laskupvm, toimituspvm, erapvm, viite, tilinro, saaja);

}

void PdfTuonti::tuoPdfTiliote()
{
    // Ensin etsitään tilinumero ja tilikausi
    QString tilinumero;
    QDate mista;
    QDate mihin;

    QString kokoteksti = tekstit_.values().join(" ");

    QRegularExpression ibanRe("\\b[A-Z]{2}\\d{2}\\w{6,30}\\b");

    // Ensimmäinen tilinumero poimitaan

    QRegularExpressionMatchIterator ibanIter = ibanRe.globalMatch(kokoteksti);
    while( ibanIter.hasNext())
    {
        QRegularExpressionMatch mats = ibanIter.next();
        QString ehdokas = mats.captured();
        if( !ehdokas.startsWith("RF") &&  IbanValidator::kelpaako(ehdokas) )
        {
            tilinumero = ehdokas;
            break;
        }
    }

    if( tilinumero.isEmpty())
        return;     // Ei löydy tilinumeroa!

    QRegularExpression valiRe("(?<p1>\\d{1,2})\\.(?<k1>\\d{1,2})\\.(?<v1>\\d{4})?\\W{1,3}(?<p2>\\d{1,2})\\.(?<k2>\\d{1,2})\\.(?<v2>\\d{4})");
    QRegularExpressionMatch valiMats = valiRe.match(kokoteksti);

    if( valiMats.hasMatch())
    {
        qDebug()  << valiMats.captured();

        int vuosi = valiMats.captured("v1").toInt();
        if( !vuosi )
            vuosi = valiMats.captured("v2").toInt();
        mista = QDate( vuosi, valiMats.captured("k1").toInt(), valiMats.captured("p1").toInt() );
        mihin = QDate( valiMats.captured("v2").toInt(), valiMats.captured("k2").toInt(), valiMats.captured("p2").toInt());
    }

    // Alustetaan tiliote tilillä ja kauden tiedoilla

    if( !tiliote(tilinumero, mista, mihin))
        return;

    // Sitten tuodaan tiliotteen tiedot
    // Jos Kirjauspäivä xx.xx.xx -kenttiä, niin haetaan kirjauspäivät niistä

    QRegularExpression kirjausPvmRe("\\bKirjauspäivä\\W+(?<p>\\d{1,2})\\.(?<k>\\d{1,2})\\.(?<v>\\d{2,4})\\b");
    kirjausPvmRe.setPatternOptions(QRegularExpression::CaseInsensitiveOption);

    tuoTiliTapahtumat( kokoteksti.contains( kirjausPvmRe) );

}

void PdfTuonti::tuoTiliTapahtumat(bool kirjausPvmRivit = false)
{
    QMapIterator<int,QString> iter(tekstit_);

    QRegularExpression kirjausPvmRe("\\bKirjauspäivä\\W+(?<p>\\d{1,2})\\.(?<k>\\d{1,2})\\.(?<v>\\d{2,4})\\b");
    kirjausPvmRe.setPatternOptions(QRegularExpression::CaseInsensitiveOption);

    QRegularExpression rahaRe("(?<etu>[+-])?(?<eur>(\\d+[ .])*\\d+),(?<snt>\\d{2})(?<taka>[+-])?");
    QRegularExpression viiteRe("\\b(RF\\d{2}\\d{4,20}|\\d{4,20})\\b");
    QRegularExpression arkistoRe("\\b([A-Za-z0-9]+\\s?)*\\b");
    QRegularExpression seliteRe("\\b[A-ö ]{8,}\\b");
    QRegularExpression pvmRe("(?<p>\\d{1,2})\\.(?<k>\\d{1,2})\\.(?<v>\\d{2}\\d{2}?)");


    IbanValidator ibanValidoija;
    ViiteValidator viiteValidoija;
    QRegularExpression ibanRe("\\b[A-Z]{2}[\\d{2}\\w\\s]{6,30}\\b");

    QDate kirjauspvm;
    QString iban;
    QString viite;
    QString arkistotunnus;
    QString selite;
    qlonglong maara = 0;

    int position;   // Validaattorien käyttöä varten

    int arkistosarake = -1;
    int taulussa = false;
    int rivilla = -1;
    int sivulla = 0;

    QDate riviKirjauspvm;
    QString riviIban;
    QString riviViite;
    QString riviArkistotunnus;
    QString riviSelite;
    qlonglong riviMaara = 0;

    while( iter.hasNext())
    {
        iter.next();
        int rivi = iter.key() / 100;
        int sivu = rivi / 200;

        if( sivulla != sivu)
        {
            taulussa = false;
            sivulla = sivu;
        }

        QString teksti = iter.value();
        int sarake = iter.key() % 100;

        if( rivi != rivilla)
        {
            // Siirrytään uudelle riville

            if( riviMaara && !riviArkistotunnus.isEmpty() )
            {
                // Edellisestä rivistä alkaa uusi kirjaus
                maara = riviMaara;
                arkistotunnus = riviArkistotunnus;
            }
            if( maara )
            {
                if( riviKirjauspvm.isValid() && !kirjausPvmRivit)
                    kirjauspvm = riviKirjauspvm;
                if( !riviIban.isEmpty())
                    iban = riviIban;
                if( !riviViite.isEmpty())
                    viite = riviViite;
                if( !riviSelite.isEmpty() && selite.isEmpty())
                    selite = riviSelite;
            }
            riviMaara = 0;
            riviKirjauspvm = QDate();
            riviIban.clear();
            riviViite.clear();
            riviSelite.clear();
            riviArkistotunnus.clear();

            rivilla = rivi;
        }

        if( !taulussa )
        {
            if( teksti.contains("arkistointitunnus", Qt::CaseInsensitive))
            {
                // Arkistointitunnus-otsake tunnistetaan ja siirrytään tauluun
                arkistosarake = sarake;
                taulussa = true;
            }
        }
        else
        {
            // Jos kirjauspäivä-rivi tai rahamäärä, tulee edellinen kirjaus valmiiksi
            if( (teksti.contains(kirjausPvmRe) || teksti.contains(rahaRe) ) &&
                maara && !arkistotunnus.isEmpty())
            {
                oterivi(kirjauspvm, maara, iban, viite, arkistotunnus, selite);
                maara = 0;
                iban.clear();
                viite.clear();
                arkistotunnus.clear();
                selite.clear();
            }

            // Kirjaspäivä-rivi
            if( teksti.contains(kirjausPvmRe) )
            {
                QRegularExpressionMatch mats = kirjausPvmRe.match(teksti);
                int vuosi = mats.captured("v").toInt();
                if( vuosi < 100)
                    vuosi += QDate::currentDate().year() / 100 * 100;
                kirjauspvm = QDate( vuosi, mats.captured("k").toInt(), mats.captured("p").toInt());
                continue;
            }

            // Saraketietona oleva kirjauspäivä
            else if( teksti.contains(pvmRe) )
            {
                QRegularExpressionMatch mats = pvmRe.match(teksti);
                int vuosi = mats.captured("v").toInt();
                if( vuosi < 100)
                    vuosi += QDate::currentDate().year() / 100;
                riviKirjauspvm = QDate( vuosi, mats.captured("k").toInt(), mats.captured("p").toInt());
            }

            // Arkistointitunnuksen oltava oikeassa sarakkeessa
            if( sarake > arkistosarake - 3 && sarake < arkistosarake + 5 && riviArkistotunnus.isEmpty() &&
                teksti.contains(arkistoRe) && teksti.count(QRegularExpression("\\d")) > 4)
            {
                QRegularExpressionMatch mats = arkistoRe.match(teksti);
                QString tunnari = mats.captured();
                if( !tunnari.contains("KIRJAUSPÄIVÄ", Qt::CaseInsensitive) &&
                    !tunnari.contains("yhteen", Qt::CaseInsensitive))

                riviArkistotunnus = mats.captured();
            }
            else if( teksti.contains(viiteRe) && riviViite.isEmpty())
            {
                QRegularExpressionMatch mats = viiteRe.match(teksti);
                QString ehdokas = mats.captured(0);
                if( viiteValidoija.validate(ehdokas,position) == ViiteValidator::Acceptable)
                {
                    riviViite = ehdokas;
                }
            }
            if( teksti.contains( ibanRe ))
            {
                QRegularExpressionMatch mats = ibanRe.match(teksti);
                QString ehdokas = mats.captured(0);
                if( ibanValidoija.validate(ehdokas,position) == IbanValidator::Acceptable )
                {
                    riviIban = ehdokas;
                }
            }
            if( teksti.contains( rahaRe))
            {
                QRegularExpressionMatch mats = rahaRe.match(teksti);
                // +/- ennen tai jälkeen
                // qDebug() << mats.captured() << " " << mats.captured("etu") << "|" << mats.captured("taka");

                if( mats.captured("etu") != mats.captured("taka"))
                {
                    QString eurot = mats.captured("eur");
                    eurot.replace(QRegularExpression("\\D"),"");
                    riviMaara = eurot.toInt() * 100 + mats.captured("snt").toInt();
                    if( mats.captured("etu") == '-'  || mats.captured("taka") == '-')
                        riviMaara = 0 - riviMaara;
                }
            }
            if( riviSelite.isEmpty() && teksti.contains(seliteRe))
            {
                QRegularExpressionMatch mats = seliteRe.match(teksti);
                QString ehdokas = mats.captured().simplified();

                // Selitteeksi ei oteta pankkitermejä
                if( !ehdokas.contains("SIIRTO") &&
                    !ehdokas.contains("OSTO") &&
                    !ehdokas.contains("LASKU") &&
                    !ehdokas.contains("IBAN") &&
                    !ehdokas.contains("BIC") &&
                    !ehdokas.contains("ARKISTOINTITUNNUS", Qt::CaseInsensitive) &&
                    !ehdokas.contains("TILINUMERO", Qt::CaseInsensitive) &&
                     ehdokas.length() > 8)
                    riviSelite = ehdokas;
            }

        }   // Kirjaukset taulussa

    }  // Tekstien iterointi
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


