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

#include <poppler/qt5/poppler-qt5.h>


#include "pdftuonti.h"

#include "validator/ibanvalidator.h"
#include "validator/viitevalidator.h"
#include "validator/ytunnusvalidator.h"
#include "tuontiapu.h"

#include "db/kirjanpito.h"
#include "db/tositetyyppimodel.h"

namespace Tuonti {



PdfTuonti::PdfTuonti()
{

}

QVariantMap PdfTuonti::tuo(const QByteArray &data)
{

    Poppler::Document *pdfDoc = Poppler::Document::loadFromData( data );
    PdfTuonti tuonti;

    if( pdfDoc )
    {
        tuonti.haeTekstit(pdfDoc);

        if( tuonti.etsi("hyvityslasku",0,30))
            {}    // Hyvityslaskulle ei automaattista käsittelyä
        else if( tuonti.etsi("lasku",0,30) || tuonti.etsi("kuitti",0,30))
            return tuonti.tuoPdfLasku();
        else if( tuonti.etsi("tiliote",0,30) )
            return tuonti.tuoPdfTiliote();

    }

    delete pdfDoc;
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
                QString tilinro = ibanRe.match(t).captured(0);
                if( IbanValidator::kelpaako(t))
                {
                    ibanit.insert( t.remove("\\s"));
                }
            }
        }
    }
    if( !data.contains("kumppaninimi") )
    {
        QStringList haetut;
        if( data.value("tyyppi").toInt() == TositeTyyppi::TULO)
            haetut = haeLahelta(15,0,50,50);
        else
            haetut = haeLahelta(0,0,50,50);
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

QVariantMap PdfTuonti::tuoPdfTiliote()
{
    QVariantMap map;
    map.insert("tyyppi", TositeTyyppi::TILIOTE);

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
            map.insert("iban",ehdokas);
            break;
        }
    }

    // Löytyy koko lailla sopivia kenttiä
    int kauttasijainti = etsi("/", 0, 30, 70, 95);
    QRegularExpression kauttaRe("\\d+/20\\d\\d");

    for( const QString& t : haeLahelta( kauttasijainti / 100 , kauttasijainti % 100, 2, 2))
    {
        if( kauttaRe.match(t).hasMatch())
        {
            map.insert("kausitunnus", kauttaRe.match(t).captured());
        }
    }

    QRegularExpression valiReViivalla("(?<p1>\\d{1,2})\\.(?<k1>\\d{1,2})\\.(?<v1>\\d{2,4})?\\W{0,3}-\\W{0,3}(?<p2>\\d{1,2})\\.(?<k2>\\d{1,2})\\.(?<v2>\\d{2,4})");
    QRegularExpression valiRe("(?<p1>\\d{1,2})\\.(?<k1>\\d{1,2})\\.(?<v1>\\d{2,4})?\\W{1,3}(?<p2>\\d{1,2})\\.(?<k2>\\d{1,2})\\.(?<v2>\\d{2,4})");
    QRegularExpressionMatch valiMats = valiReViivalla.match(kokoteksti);
    if( !valiMats.hasMatch())
        valiMats = valiRe.match(kokoteksti);

    if( valiMats.hasMatch())
    {

        int alkuvuosi = valiMats.captured("v1").toInt();
        int loppuvuosi = valiMats.captured("v2").toInt();
        if( !alkuvuosi )
            alkuvuosi = loppuvuosi;
        if( alkuvuosi < 2000)
            alkuvuosi += 2000;
        if( loppuvuosi < 2000)
            loppuvuosi += 2000;
        map.insert("alkupvm", QDate( alkuvuosi, valiMats.captured("k1").toInt(), valiMats.captured("p1").toInt() ) );
        map.insert("loppupvm", QDate( loppuvuosi, valiMats.captured("k2").toInt(), valiMats.captured("p2").toInt()));
    }

    // Sitten tuodaan tiliotteen tiedot
    // Jos Kirjauspäivä xx.xx.xx -kenttiä, niin haetaan kirjauspäivät niistä

    QRegularExpression kirjausPvmRe("\\b(Kirjauspäivä|Entry date)\\W+(?<p>\\d{1,2})\\.(?<k>\\d{1,2})\\.(?<v>(\\d{2})?(\\d{2})?)");
    kirjausPvmRe.setPatternOptions(QRegularExpression::CaseInsensitiveOption);

    map.insert("tapahtumat",tuoTiliTapahtumat( kokoteksti.contains( kirjausPvmRe) , map.value("loppupvm").toDate().year()));
    return map;
}

QVariantList PdfTuonti::tuoTiliTapahtumat(bool kirjausPvmRivit = false, int vuosiluku = QDate::currentDate().year())
{
    QVariantList tapahtumat;
    QMapIterator<int,QString> iter(tekstit_);
    QMapIterator<int,QString> riviIter(tekstit_);

    QRegularExpression kirjausPvmRe("\\b(Kirjauspäivä|Entry date)\\W+(?<p>\\d{1,2})\\.(?<k>\\d{1,2})\\.(?<v>(\\d{2})?(\\d{2})?)");
    kirjausPvmRe.setPatternOptions(QRegularExpression::CaseInsensitiveOption);

    QRegularExpression rahaRe("(?<etu>[+-])?(?<eur>(\\d+[ .])*\\d+),(?<snt>\\d{2})(?<taka>[+-])?");
    QRegularExpression viiteRe("((Viite|Reference)\\w*\\W*|\\b)(?<viite>(RF\\d{2}\\d{4,20}|\\d{4,20}))");
    QRegularExpression arkistoRe("\\b([A-Za-z0-9]+\\s?)+");
    QRegularExpression seliteRe("[A-ö& \\-]{6,}");
    QRegularExpression pvmRe("(?<p>\\d{1,2})\\.?(?<k>\\d{1,2})\\.?(?<v>\\d{2}\\d{2}?)");
    QRegularExpression ibanRe("\\b[A-Z]{2}\\d{2}[\\w\\s]{6,30}\\b");

    QDate kirjauspvm;

    int arkistosarake = -1;
    int maarasarake = -1;
    int arkistorivi = -1;

    int taulussa = false;
    int rivilla = -1;
    int sivulla = 0;
    int tapahtumanrivi = -1;

    bool saajaensin = false;
    int riviero = 0;

    QVariantMap tapahtuma;

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


        if( !taulussa )
        {
            if( teksti.contains("arkistointitunnus", Qt::CaseInsensitive) || teksti.contains("Filings code", Qt::CaseInsensitive))
            {
                // Arkistointitunnus-otsake tunnistetaan ja siirrytään tauluun
                arkistosarake = sarake;
                arkistorivi = rivi;
                taulussa = true;
            }

        } else if(arkistorivi == rivi) {
            if(teksti.contains("Määrä", Qt::CaseInsensitive)) {
                maarasarake = sarake;
            }
            if( teksti.contains("Saaja", Qt::CaseInsensitive)) {
                saajaensin = true;
            }
        } else {
            // Kirjauspäivä-rivi
            if( teksti.contains(kirjausPvmRe) )
            {
                QRegularExpressionMatch mats = kirjausPvmRe.match(teksti);
                int vuosi = mats.captured("v").toInt();
                if( !vuosi)
                    vuosi = vuosiluku;
                else if( vuosi < 100)
                    vuosi += QDate::currentDate().year() / 100 * 100;
                kirjauspvm = QDate( vuosi, mats.captured("k").toInt(), mats.captured("p").toInt());
                continue;
            }

            if( rivi != rivilla)
            {
                // Iteroidaan tämä rivi läpi ja tarkastetaan, alkaako tästä uusi rivi
                riviIter = iter;
                while( riviIter.hasNext() ) {
                    riviIter.next();
                    if (riviIter.key() / 100 != rivi) {
                        break;
                    } else if( riviIter.key() % 100 >= maarasarake-2 && riviIter.value().contains( rahaRe) ) {
                        // Tämä on rahamäärä, joten tästä alkaa uusi tilitapahtuma, ja edellinen
                        // tallennetaan
                        if (tapahtuma.contains("arkistotunnus") && tapahtuma.contains("euro"))
                            tapahtumat.append(tapahtuma);
                        tapahtuma.clear();

                        // Tallennetaan euromäärä
                        QRegularExpressionMatch mats = rahaRe.match(riviIter.value());
                        // +/- ennen tai jälkeen

                        if( mats.captured("etu") != mats.captured("taka"))
                        {
                            QString eurot = mats.captured("eur");
                            eurot.replace(QRegularExpression("\\D"),"");
                            qlonglong sentit = eurot.toInt() * 100 + mats.captured("snt").toInt();
                            if( mats.captured("etu") == '-'  || mats.captured("taka") == '-')
                                sentit = 0 - sentit;
                            tapahtuma.insert("euro", sentit / 100.0);
                            tapahtuma.insert("pvm", kirjauspvm);
                        }
                        tapahtumanrivi = 0;
                    }
                }
                if( tapahtumanrivi == 1)
                    riviero = rivi - rivilla;
                else if( tapahtumanrivi > 1 && riviero * 3 / 2 < rivi - rivilla) {
                    // Tyhjä rivi tapahtuman keskellä, tapahtuma valmistuu
                    if (tapahtuma.contains("arkistotunnus") && tapahtuma.contains("euro"))
                        tapahtumat.append(tapahtuma);
                    tapahtuma.clear();
                    continue;
                }
                rivilla = rivi;
                tapahtumanrivi++;


            }



            // Vasemmanpuoleisimmassa sarakkeessa Arkistointitunnus ja Saajan tilinumero
            if( sarake < arkistosarake + 5) {

                if( tapahtumanrivi == 1 && teksti.contains(arkistoRe) && teksti.count(QRegularExpression("\\d")) > 4)
                {
                    QRegularExpressionMatch mats = arkistoRe.match(teksti);
                    QString tunnari = mats.captured().left(20);
                    if( !tunnari.contains("KIRJAUSPÄIVÄ", Qt::CaseInsensitive) &&
                        !tunnari.contains("yhteen", Qt::CaseInsensitive))

                    tapahtuma.insert("arkistotunnus",mats.captured());
                    if( teksti.length() > tunnari.length() + 10) {
                        teksti = teksti.mid(tunnari.length() + 1);
                        sarake += 7;
                    }
                }
                if( !kirjausPvmRivit && teksti.contains(pvmRe)) {
                    QRegularExpressionMatch mats = pvmRe.match(teksti);
                    int vuosi = mats.captured("v").toInt();
                    if( !vuosi)
                        vuosi = vuosiluku;
                    else if( vuosi < 100)
                        vuosi += QDate::currentDate().year() / 100 * 100;
                    tapahtuma.insert("pvm",QDate( vuosi, mats.captured("k").toInt(), mats.captured("p").toInt()));
                }
                if( tapahtumanrivi == 2) {
                    QString alku = teksti.startsWith("*") ? teksti.mid(1,18) : teksti.left(18);
                    if( IbanValidator::kelpaako(alku)) {
                        tapahtuma.insert("iban", alku);
                        teksti = teksti.mid(18);
                        sarake += 7;
                    }
                }

            }
            // Toisessa sarakkeessa Saaja / Maksaja + Selite

            if( sarake > arkistosarake + 5 && sarake < maarasarake - 2) {

                // Poistetaan alusta mahdollinen päivämäärä
                QString alku = teksti.left(teksti.indexOf(' '));
                if( alku.contains(QRegularExpression("\\d{2}\\.?\\d{2}")))
                    teksti = teksti.mid(teksti.indexOf(' ')+1);

                if( ( tapahtumanrivi == 1 && !saajaensin) || (tapahtumanrivi == 2 && saajaensin) ) {
                    // Tältä riviltä poimitaan selite KTO-koodia varten
                    if( ktokoodi(teksti) && !tapahtuma.contains("ktokoodi"))
                        tapahtuma.insert("ktokoodi", ktokoodi(teksti));

                } else if( (tapahtumanrivi == 2 && !saajaensin) || (tapahtumanrivi == 1 && saajaensin)) {
                    // Tältä riviltä yritetään poimia saaja

                    if (teksti.contains(seliteRe) && !tapahtuma.contains("saajamaksaja"))
                        tapahtuma.insert("saajamaksaja", teksti.simplified());
                } else if( teksti.contains("Viite") && teksti.contains(viiteRe) && !tapahtuma.contains("viite")
                           && sarake > arkistosarake + 5 && sarake < maarasarake - 5) {
                    QRegularExpressionMatch mats = viiteRe.match(teksti);
                    QString ehdokas = mats.captured("viite");
                    if( ViiteValidator::kelpaako(ehdokas)) {
                        tapahtuma.insert("viite", ehdokas);
                        if( tapahtuma.value("euro").toDouble() > 1e-5)
                            tapahtuma.insert("ktokoodi", 705);
                    }
                } else if( tapahtumanrivi > 2) {
                    if( teksti.contains(ibanRe)) {
                        QRegularExpressionMatch mats = ibanRe.match(teksti);
                        if( IbanValidator::kelpaako(mats.captured()))
                            tapahtuma.insert("iban",mats.captured(0));
                    } else if ( !teksti.contains("viesti", Qt::CaseInsensitive)) {
                        if (!tapahtuma.value("selite").toString().isEmpty())
                            tapahtuma.insert("selite", tapahtuma.value("selite").toString() + " " + teksti);
                        else
                            tapahtuma.insert("selite",teksti);
                    }
                }
            }
        }
    }  // Tekstien iterointi
    return tapahtumat;
}

int PdfTuonti::ktokoodi(const QString &teksti)
{
    QRegularExpression ktoRe("(7[0-8][0-6])\\s\\w+");
    QRegularExpressionMatch mats = ktoRe.match(teksti);
    if( mats.hasMatch())
        return mats.captured(1).toInt();

    if( teksti.contains("TILISIIRTO"))
        return 700;
    else if(teksti.contains("PANO"))
        return 710;
    else if(teksti.contains("OTTO") || teksti.contains("NOSTO"))
        return 720;
    else if(teksti.contains("KORTTIOSTO"))
        return 721;
    else if(teksti.contains("PALVELUMAKSU"))
        return 730;
    else if(teksti.contains("KORKOHYVITYS"))
        return 750;
    return 0;
}


void PdfTuonti::haeTekstit(Poppler::Document *pdfDoc)
{
    // Tuottaa taulukon, jossa pdf-tiedoston tekstit suhteellisessa koordinaatistossa

    for(int sivu = 0; sivu < pdfDoc->numPages(); sivu++)
    {
        Poppler::Page *pdfSivu = pdfDoc->page(sivu);
        if( !pdfSivu)   // Jos sivu ei ole kelpo
            continue;

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
            int ero =  qRound( std::sqrt(  std::pow( (x - sx), 2) + std::pow( ( y - sy), 2)  ));
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
