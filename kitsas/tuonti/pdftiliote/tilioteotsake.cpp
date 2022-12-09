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
#include "tilioteotsake.h"
#include "pdftiliotetuonti.h"

#include <iostream>

namespace Tuonti {

TilioteOtsake::TilioteOtsake(PdfTilioteTuonti *tuonti) :
    tuonti_(tuonti)
{

}

bool TilioteOtsake::alkaakoOtsake(const PdfAnalyzerRow &row)
{
    if( row.textCount() < 3)
        return false;
    const QString teksti = row.text();
    return teksti.contains("Arkistointitunnus", Qt::CaseInsensitive) ||
           teksti.contains("Arkiveringskod", Qt::CaseInsensitive) ||
           (teksti.contains("Kirjaus-", Qt::CaseInsensitive) && teksti.contains("Pano", Qt::CaseInsensitive)) ||
           (teksti.contains("Kirj.pvm", Qt::CaseInsensitive) && teksti.contains("Arvopvm.", Qt::CaseInsensitive));
}

void TilioteOtsake::kasitteleRivi(const PdfAnalyzerRow &row)
{
    if(sarakkeet_.isEmpty()) {        
        for(const auto& teksti : row.textList()) {
            QString puskuri;
            QRectF rect;
            for(const auto& sana : teksti.words()) {
                puskuri.append(sana.text());
                rect = rect.united(sana.boundingRect());
                if( sana.hasSpaceAfter() && puskuri.trimmed().length() > 2) {
                    Sarake sarake( Sarake( rect.x(), rect.right(), tyyppiTekstilla(puskuri.toUpper())));
                    sarakkeet_.append(sarake);
                    puskuri.clear();
                    rect = QRectF();
                }
            }
            if( rect.isValid() && puskuri.trimmed().length() > 0) {
                Sarake sarake( Sarake( rect.x(), rect.right(), tyyppiTekstilla(puskuri.toUpper())));
                sarakkeet_.append(sarake);
            }
        }
    }

    otsakeRivia_++;
}

bool TilioteOtsake::tarkastaRivi(const PdfAnalyzerRow &row)
{
    if( alkaakoOtsake(row) || tarkastusRivia_)
        tarkastusRivia_++;
    if( tarkastusRivia_ == otsakeRivia_) {
        tarkastusRivia_ = 0;
        return true;
    }
    return false;;
}

int TilioteOtsake::sarakkeita() const
{
    return sarakkeet_.count();
}

TilioteOtsake::Sarake TilioteOtsake::sarake(int indeksi) const
{
    return sarakkeet_.value(indeksi);
}

int TilioteOtsake::indeksiSijainnilla(double sijainti)
{
    for(int i=sarakkeet_.count()-1; i >= 0; i--) {
        const Sarake& sarake = sarakkeet_.at(i);
        if( sijainti + 2 >= sarake.alku() ) {

            // Korjaus
            if( i == 1 && sarake.tyyppi() == PVM && sijainti > sarake.alku() + 20 && ( tuonti_->bic() == "SBANFIHH" || tuonti_->bic() == "AABAFI22"  ))
                return i+1;

            return i;
        }
    }
    return -1;
}

TilioteOtsake::Tyyppi TilioteOtsake::tyyppi(double paikka)
{
    for(int i=sarakkeet_.count()-1; i >= 0; i--) {
        const Sarake& sarake = sarakkeet_.at(i);
        if( paikka > sarake.alku() ) {
            return sarake.tyyppi();
        }
    }
    return TUNTEMATON;
}

QString TilioteOtsake::debugInfo() const
{
    QString ulos;
    for(const Sarake& s : sarakkeet_) {
        ulos.append(QString("   %1 ... %2   %3").arg(s.alku(),6,'f',2)
                .arg(s.loppu(), 6, 'f', 2)
                .arg( tyyppiTeksti(s.tyyppi())));
    }
    return ulos;
}

QString TilioteOtsake::tyyppiTeksti(Tyyppi tyyppi)
{
    switch (tyyppi) {
        case TUNTEMATON: return "???";
        case SAAJAMAKSAJA: return "Saaja/Maksaja";
        case SELITE: return "Selite";
        case PVM: return "Pvm";
        case VIITE: return "Viite";
        case ARKISTOTUNNUS: return "Arkistotunnus";
        case EURO: return "Euro";
        case OHITA: return "Ohita";
        case YLEINEN: return "YLEINEN";
    }
    return QString();
}

TilioteOtsake::Tyyppi TilioteOtsake::tyyppiTekstilla(const QString &teksti)
{
    if( tuonti_->bic() == "HELSFIHH") {
        // Aktia pankki
        if( teksti == "KIRJ.PVM.")
            return ARKISTOTUNNUS;
        else if(teksti == "ARVOPVM.")
            return OHITA;
        else if(teksti == "SELITE")
            return YLEINEN;
    }



    if( teksti.contains("KIRJAUS") || teksti.contains("MAKSUP") || teksti.contains("ARVOP") || teksti.contains("DAG"))
        return PVM;
    else if(teksti.contains("VIITE"))
        return VIITE;
    else if(teksti.contains("PANO") ||
            teksti.contains("MÄÄR") ||
            teksti.contains("EUR"))
        return EURO;
    else if( teksti.contains("SELIT"))
        return SELITE;
    else if( teksti.contains("MAKSAJA") || teksti.contains("BETALARE"))
        return SAAJAMAKSAJA;
    else if( teksti.contains("ARKISTOINTI") || teksti.contains("ARKIVERINGS"))
        return ARKISTOTUNNUS;
    else if(teksti.contains("TAP") ||
            teksti.contains("SALDO") ||
            teksti.contains("TILIÖINTI"))
        return OHITA;
    return TUNTEMATON;
}


TilioteOtsake::Sarake::Sarake()
{

}

TilioteOtsake::Sarake::Sarake(double alku, double loppu, Tyyppi tyyppi)
    : alku_(alku), loppu_(loppu), tyyppi_(tyyppi)
{
}

}

