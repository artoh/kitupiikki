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
#include "db/tositetyyppimodel.h"


namespace Tuonti {

PdfTilioteTuonti::PdfTilioteTuonti() :
    otsake_(this),
    ibanRe("\\b[A-Z]{2}\\d{2}\\w{6,30}\\b"),
    kauttaRe("\\d+/20\\d\\d"),
    valiReViivalla("(?<p1>\\d{1,2})\\.(?<k1>\\d{1,2})\\.(?<v1>\\d{2,4})?\\W{0,3}-\\W{0,3}(?<p2>\\d{1,2})\\.(?<k2>\\d{1,2})\\.(?<v2>\\d{2,4})"),
    rahaRe("^(?<etu>[+-])?\\s*(?<eur>\\d{0,3}([,. ]?\\d{3})*|\\d{1,3})[,.](?<snt>\\d{2})\\s?(?<taka>[+-])?$"),
    omaIbanRe(R"(FI\d{2}(\s?\d{4}\s?){3}\s?\d{2})")
{

}

QVariantMap PdfTilioteTuonti::tuo(const QList<PdfAnalyzerPage> pages)
{        

    for( auto & sivu : pages) {
         lueSivu(sivu);
        tila_ = TOINENSIVU;
    }
    nykyinenValmis();

    qDebug() << otsake_.debugInfo();

    QVariantMap map;
    map.insert("tyyppi", TositeTyyppi::TILIOTE);
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

void PdfTilioteTuonti::lueSivu(const PdfAnalyzerPage &page)
{
    edellinenAlalaita_ = 100000;
    for(const auto& rivi : page.rows()) {
        lueRivi(rivi);        
    }
}

void PdfTilioteTuonti::lueRivi(const PdfAnalyzerRow &row)
{

    if( tila_ == ALKU)
        lueAlkuRivi(row);
    if( tila_ == OTSAKE)
        lueOtsakeRivi(row);
    if( tila_ == TAULU)
        lueTaulukkoRivi(row);
    if( tila_ == TOINENSIVU)
        lueToisenAlkua(row);
}

void PdfTilioteTuonti::lueAlkuRivi(const PdfAnalyzerRow &row)
{
    if( otsake_.alkaakoOtsake(row)) {
       tila_ = OTSAKE;
    } else {
        QString rivinTeksti = row.text();
        if( !iban_.isValid()) {
            QRegularExpressionMatchIterator ibanIter = omaIbanRe.globalMatch(rivinTeksti);
            while( ibanIter.hasNext())
            {
                QRegularExpressionMatch mats = ibanIter.next();
                Iban ehdokas(mats.captured());
                if( ehdokas.isValid()) {
                    iban_ = ehdokas;
                    break;
                }
            }
        }
        if( kausiTeksti_.isEmpty()) {
            QRegularExpressionMatch mats = kauttaRe.match(rivinTeksti);
            if( mats.hasMatch()) {
                kausiTeksti_ = mats.captured();
            }
        }
        if( !alkupvm_.isValid()) {
            QRegularExpressionMatch valiMats = valiReViivalla.match(rivinTeksti);
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

void PdfTilioteTuonti::lueOtsakeRivi(const PdfAnalyzerRow &row)
{
    if( row.text().contains(QRegularExpression("\\d"))) {
        tila_ = TAULU;
    } else {
        otsake_.kasitteleRivi(row);
    }
}

void PdfTilioteTuonti::lueTaulukkoRivi(const PdfAnalyzerRow &row)
{
    // Ensin pitäisi tarkistaa, mennäänkö taulukosta ulos
    // Mennään ulos, jos iso väli tai epämääräisiä tekstejä
    QString kokoRivi = row.text();

    if( row.boundingRect().top() > edellinenAlalaita_ + 18 ||
        kokoRivi.contains("Yhteystiedot", Qt::CaseInsensitive) ||
        kokoRivi.toUpper() == "JATKUU" || kokoRivi.toUpper() == "TRANSP" || kokoRivi.toUpper() == "* JATKUU *") {
        tila_ = LOPPU;        
        return;
    }
    edellinenAlalaita_ = row.boundingRect().bottom();

    if(kokoRivi.startsWith("KIRJAUSPÄIVÄ", Qt::CaseInsensitive) ||
       kokoRivi.startsWith("REGISTR. DAG", Qt::CaseInsensitive)) {
        kirjausPvm_ = OteRivi::strPvm(kokoRivi, loppupvm_);
        nykyinenValmis();
        qDebug() << "\n *** KIRJAUSPÄIVÄ *** " << kirjausPvm_.toString("dd.MM.yyyy") << " \n";
        return;
    }

    // Tekstin lukeminen ja taulukkosarakkeisiin sijoittaminen
    // Taulukoiden laittaminen paikoilleen
    kasitteleTaulukkoRivi(row);


}

void PdfTilioteTuonti::lueToisenAlkua(const PdfAnalyzerRow &row)
{
    if( otsake_.tarkastaRivi(row))
        tila_ = TAULU;
}

void PdfTilioteTuonti::kasitteleTaulukkoRivi(const PdfAnalyzerRow &row)
{
    // Laitetaan sanat yhteen listaan
    QList<PdfAnalyzerWord> words_;

    for(const auto& teksti : row.textList()) {
        words_.append(teksti.words());
    }


    for(const auto& sana : words_) {
        if( otsake_.tyyppi(sana.boundingRect().right()) == TilioteOtsake::EURO) {
            nykyinenValmis();
        }
    }
    rivilla_++;

#ifdef KITSAS_DEVEL
    std::cerr << "\n(" << rivilla_ << ") ";
    for(const auto& sana : words_)
        std::cerr << sana.text().toStdString() << " ";
    std::cerr << "\n   ";
#endif


    QString puskuri;
    int sarake = -1;
    TilioteOtsake::Tyyppi saraketyyppi = TilioteOtsake::TUNTEMATON;
    QString kokoTeksti;
    qreal edellinenLoppuu = 0;


    for(const auto& sana : words_) {
        if( sana.boundingRect().left() > edellinenLoppuu + 10 &&
            sana.text().trimmed() != "-" &&
            saraketyyppi != TilioteOtsake::ARKISTOTUNNUS) {            
            taulukkoSarakeValmis(saraketyyppi, puskuri);
            puskuri.clear();
            saraketyyppi = TilioteOtsake::TUNTEMATON;
        }
        edellinenLoppuu = sana.boundingRect().right();


        const QString& sanateksti = sana.text().trimmed();
        kokoTeksti.append(sanateksti);
        if(sanateksti.length() == 2 && sanateksti.startsWith(QChar('/')))
            continue;

        int uusisarake = otsake_.indeksiSijainnilla(sana.boundingRect().left());
        if( uusisarake != sarake && sana.text().trimmed() != "-") {
            taulukkoSarakeValmis(saraketyyppi, puskuri);
            sarake = uusisarake;
            saraketyyppi = otsake_.sarake(sarake).tyyppi();
            puskuri.clear();
        }
        puskuri.append(sanateksti);
        if(sana.hasSpaceAfter()) {
            if( saraketyyppi == TilioteOtsake::PVM && puskuri.length() > 3) {
                taulukkoSarakeValmis(saraketyyppi, puskuri);
                saraketyyppi = TilioteOtsake::YLEINEN;
                if( puskuri.contains(QRegularExpression("\\s")))
                    puskuri = puskuri.mid( puskuri.indexOf(QRegularExpression("\\s")) + 1 );
                else
                    puskuri.clear();
            } else {
                puskuri.append(" ");
            }
        }
    }


    // Rivin lopussa käsitellään viimeinen sarake
    taulukkoSarakeValmis(saraketyyppi, puskuri);

}

void PdfTilioteTuonti::taulukkoSarakeValmis(TilioteOtsake::Tyyppi tyyppi, const QString &arvo)
{
    if( arvo.trimmed().length() < 2)
        return;

#ifdef KITSAS_DEVEL
    std::cerr << TilioteOtsake::tyyppiTeksti(tyyppi).toStdString() << "=" <<  arvo.toStdString() << " * ";
#endif

    switch (tyyppi) {
    case TilioteOtsake::SAAJAMAKSAJA: nykyinen_.lisaaYleinen(arvo); break;
    case TilioteOtsake::SELITE: nykyinen_.setSelite(arvo); break;
    case TilioteOtsake::PVM: if(rivilla_==1)  nykyinen_.setPvm(arvo, loppupvm_); break;
    case TilioteOtsake::VIITE: if(rivilla_==1) nykyinen_.setViite(arvo); break;
    case TilioteOtsake::ARKISTOTUNNUS: nykyinen_.setArkistotunnus(arvo); break;
    default:
    {
        QRegularExpressionMatch mats = rahaRe.match(arvo.trimmed());

        if( rivilla_ == 1 && !nykyinen_.euro() && mats.hasMatch() )
        {
            QString eurot = mats.captured("eur");
            eurot.replace(QRegularExpression("\\D"),"");
            qlonglong sentit = eurot.toInt() * 100 + mats.captured("snt").toInt();
            if( mats.captured("etu") == '-'  || mats.captured("taka") == '-')
                sentit = 0 - sentit;

            nykyinen_.setEuro(Euro(sentit));

#ifdef KITSAS_DEVEL
            std::cerr <<( mats.hasMatch() ?  (QString(" %1 -> %2").arg(sentit).arg( nykyinen_.euro().display() )).toStdString() : "(?) ");
#endif

        } else {
            nykyinen_.lisaaYleinen(arvo);
        }        
    }
    }
}

void PdfTilioteTuonti::nykyinenValmis()
{
    if( nykyinen_.valmis()) {
        QVariantMap map = nykyinen_.map(kirjausPvm_);
        if(!map.isEmpty()) {
            tapahtumat_.append(map);
        }
    } else {
        qWarning() <<  kirjausPvm_.toString("dd.MM.yyyy") << "  " << nykyinen_.euro().display(true) << "  "
                  << nykyinen_.arkistotunnus();
    }    
    nykyinen_.tyhjenna();
    rivilla_ = 0;
}

}
