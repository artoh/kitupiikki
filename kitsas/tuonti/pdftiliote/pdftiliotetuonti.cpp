/*
   Copyright (C) 2019 Arto Hyvättinen

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
#include "pdftiliotetuonti.h"
#include "laskutus/iban.h"
#include <QDate>

#include <QDebug>
#include <iostream>

#include "tuonti/pdf/pdftiedosto.h"
#include "tuonti/pdf/pdfsivu.h"
#include "tuonti/pdf/pdfrivi.h"
#include "tuonti/pdf/pdfpala.h"


namespace Tuonti {

PdfTilioteTuonti::PdfTilioteTuonti() :
    otsake_(this)
{

}

QVariantMap PdfTilioteTuonti::tuo(PdfTiedosto *tiedosto)
{        
    // Luetaan rivi kerrallaan ;)

    tila_ = ALKU;
    for(int s = 0; s < tiedosto->sivumaara(); s++) {
        PdfSivu* sivu = tiedosto->sivu(s);
        for(int r = 0; r < sivu->riveja(); r++ ) {
            PdfRivi* rivi = sivu->rivi(r);

//          qDebug() << r << " [" << tila_ << "] " << rivi->teksti();
            lueRivi(rivi);
        }
        tila_ = TOINENSIVU;
    }

    nykyinenValmis();
    return map();
}

QVariantMap PdfTilioteTuonti::map() const
{
    QVariantMap map;
    map.insert("tyyppi", TILIOTETYYPPI);
    if( iban_.isValid())
        map.insert("iban", iban_.valeitta());
    if( !kausiTeksti_.isEmpty())
        map.insert("kausitunnus", kausiTeksti_);
    if( alkupvm_.isValid()) {
        map.insert("alkupvm", alkupvm_);
        map.insert("loppupvm", loppupvm_);
    }
    map.insert("tapahtumat", tapahtumat_);
    return map;

}


void PdfTilioteTuonti::lueRivi(PdfRivi *rivi)
{    

    if( tila_ == ALKU)
        lueAlkuRivi(rivi);
    if( tila_ == OTSAKE)
        lueOtsakeRivi(rivi);
    if( tila_ == TAULU)
        lueTaulukkoRivi(rivi);
    if( tila_ == TOINENSIVU)
        lueToisenAlkua(rivi);
}

void PdfTilioteTuonti::lueAlkuRivi(PdfRivi *rivi)
{
    if( otsake_.alkaakoOtsake(rivi)) {
       tila_ = OTSAKE;
    } else {
        const QString& teksti = rivi->teksti();

        if( !iban_.isValid()) {
            QRegularExpressionMatch ibanMats = omaIbanRe__.match(teksti);
            if( ibanMats.hasMatch()) {
                Iban iban(ibanMats.captured());
                if(iban.isValid()) {
                    iban_ = iban;
                }
            }
        }
        if( kausiTeksti_.isEmpty()) {
            QRegularExpressionMatch mats = kauttaRe__.match(teksti);
            if( mats.hasMatch()) {
                kausiTeksti_ = mats.captured();
            }
        }
        if( !alkupvm_.isValid()) {
            QRegularExpressionMatch valiMats = valiReViivalla__.match(teksti);
            int alkuvuosi = valiMats.captured("v1").toInt();
            int loppuvuosi = valiMats.captured("v2").toInt();
            if( !alkuvuosi )
                alkuvuosi = loppuvuosi;
            if( alkuvuosi < 2000)
                alkuvuosi += 2000;
            if( loppuvuosi < 2000)
                loppuvuosi += 2000;
            alkupvm_ = QDate( alkuvuosi, valiMats.captured("k1").toInt(), valiMats.captured("p1").toInt() ) ;
            loppupvm_ = QDate( loppuvuosi, valiMats.captured("k2").toInt(), valiMats.captured("p2").toInt());
         }

    }
}

void PdfTilioteTuonti::lueOtsakeRivi(PdfRivi* rivi)
{
    if( rivi->teksti().contains(numeroRe__) ) {
        qDebug() << otsake_.debugInfo();
        tila_ = TAULU;
    } else {
        otsake_.kasitteleRivi(rivi);
    }
}

void PdfTilioteTuonti::lueTaulukkoRivi(PdfRivi *rivi)
{
    PdfPala* pala = rivi->pala();
    const QString& teksti = rivi->teksti();

    // Ensin pitäisi tarkistaa, mennäänkö taulukosta ulos    
    if(( pala->vasen() > 500 ||
       (otsake_.indeksiSijainnilla(pala->vasen()) == 0 &&
        pala->teksti().contains(pieniRe__)))
            && !teksti.contains("Kirjauspäivä", Qt::CaseInsensitive)
            && !teksti.startsWith("Registr. dag", Qt::CaseInsensitive)
            && !pala->teksti().startsWith("SALDO", Qt::CaseInsensitive) ) {
        tila_ = LOPPU;
        return;
    }
    for(const auto& txt : jatkuuTekstit__) {
        if(teksti.contains(txt, Qt::CaseInsensitive)) {
            tila_ = LOPPU;
            return;
        }
    }


    if(teksti.contains("KIRJAUSPÄIVÄ", Qt::CaseInsensitive) ||
       teksti.contains("REGISTR. DAG", Qt::CaseInsensitive)) {
        kirjausPvm_ = OteRivi::strPvm(teksti, loppupvm_);
        nykyinenValmis();
        return;
    }

    // Tekstin lukeminen ja taulukkosarakkeisiin sijoittaminen
    // Taulukoiden laittaminen paikoilleen
    kasitteleTaulukkoRivi(rivi);


}

void PdfTilioteTuonti::lueToisenAlkua(PdfRivi *rivi)
{
    if( otsake_.tarkastaRivi(rivi))
        tila_ = TAULU;
}

void PdfTilioteTuonti::kasitteleTaulukkoRivi(PdfRivi* rivi)
{
    PdfPala* pala = rivi->pala();

    // Tarkastetaan, onko tässä euroja
    while(pala) {
        if( otsake_.tyyppi(pala->vasen() + 5) == TilioteOtsake::EURO ||
            otsake_.tyyppi(pala->oikea() - 5) == TilioteOtsake::EURO) {
            nykyinenValmis();
        }
        pala = pala->seuraava();
    }

    // Aloitetaan alusta
    pala = rivi->pala();

    if( !rivilla_ && otsake_.indeksiSijainnilla(pala->vasen()) > 0 ) {
        // Aloittavaan riviin tarvitaan tavaraa vasemmasta laidasta
        return;
    }

    while( pala ) {
        QString teksti = pala->teksti();
        if( otsake_.sarakkeita() && pala->vasen() < otsake_.sarake(0).alku() && pala->korkeus() > 72) {
            // Vasemman laidan tekstiä
            qDebug() << "Vasemmalla " << teksti << " " << pala->korkeus();
        } else if( teksti.length() > 3) {
            TilioteOtsake::Tyyppi alkuTyyppi = otsake_.tyyppi(pala->vasen() + 5);
            TilioteOtsake::Tyyppi loppuTyyppi = otsake_.tyyppi(pala->oikea() - 5);

            if(alkuTyyppi != loppuTyyppi && alkuTyyppi != TilioteOtsake::OHITA) {
                if(alkuTyyppi == TilioteOtsake::EURO) {
                    int indeksi = teksti.indexOf(aakkosRe__);
                    nykyinen_.kasittele(teksti.left(indeksi), alkuTyyppi, rivilla_, loppupvm_);
                    nykyinen_.kasittele(teksti.mid(indeksi), loppuTyyppi, rivilla_, loppupvm_);
                } else {
                    int indeksi = teksti.indexOf(" ");
                    nykyinen_.kasittele(teksti.left(indeksi), alkuTyyppi, rivilla_, loppupvm_);
                    nykyinen_.kasittele(teksti.mid(indeksi+1), loppuTyyppi, rivilla_, loppupvm_);
                }
            } else if(loppuTyyppi == TilioteOtsake::TUNTEMATON) {
                while( pala->seuraava() &&
                       otsake_.tyyppi(pala->seuraava()->vasen()+5) == TilioteOtsake::TUNTEMATON &&
                       otsake_.tyyppi(pala->seuraava()->oikea()-5) == TilioteOtsake::TUNTEMATON) {
                    pala = pala->seuraava();
                    teksti.append(" " + pala->teksti());
                }
                nykyinen_.kasittele(teksti, TilioteOtsake::TUNTEMATON, rivilla_, loppupvm_);
            } else {
                nykyinen_.kasittele(teksti, loppuTyyppi, rivilla_, loppupvm_);
            }
        }
        pala = pala->seuraava();
    }
    rivilla_++;
}

void PdfTilioteTuonti::nykyinenValmis()
{
    if( nykyinen_.valmis()) {
        QVariantMap map = nykyinen_.map(kirjausPvm_);
        if(!map.isEmpty()) {
            tapahtumat_.append(map);
        } else {
            ;
        }
    } else {
//        qWarning() <<  "!EI VALMIS!" << kirjausPvm_.toString("dd.MM.yyyy") << "  " << nykyinen_.euro().display(true) << "  "
//                  << nykyinen_.arkistotunnus();
    }    
    nykyinen_.tyhjenna();
    rivilla_ = 0;
}

QRegularExpression PdfTilioteTuonti::kauttaRe__("\\d+/20\\d\\d");
QRegularExpression PdfTilioteTuonti::valiReViivalla__("(?<p1>\\d{1,2})\\.(?<k1>\\d{1,2})\\.(?<v1>\\d{2,4})?\\W{0,3}-\\W{0,3}(?<p2>\\d{1,2})\\.(?<k2>\\d{1,2})\\.(?<v2>\\d{2,4})");
QRegularExpression PdfTilioteTuonti::rahaRe__("^(?<etu>[+-])?\\s*(?<eur>\\d{0,3}([,. ]?\\d{3})*|\\d{1,3})[,.](?<snt>\\d{2})\\s?(?<taka>[+-])?$");
QRegularExpression PdfTilioteTuonti::omaIbanRe__(R"(FI\d{2}(\s*\d{4}){3}\s*\d{2})");
QRegularExpression PdfTilioteTuonti::numeroRe__("\\d+");
QRegularExpression PdfTilioteTuonti::pieniRe__("[a-z]");
QRegularExpression PdfTilioteTuonti::aakkosRe__("[A-Za-z]");

std::vector<QString> PdfTilioteTuonti::jatkuuTekstit__ = {
    "Postiosoite","LUOTTAMUKSELLINEN. Tämä viesti sisältää luottamuksellista tietoa ja on tarkoitettu vain valtuutetulle vastaanottajalle"
};

}
