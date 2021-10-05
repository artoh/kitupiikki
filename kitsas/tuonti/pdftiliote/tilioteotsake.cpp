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

#include <iostream>

namespace Tuonti {

TilioteOtsake::TilioteOtsake()
{

}

bool TilioteOtsake::alkaakoOtsake(const PdfAnalyzerRow &row)
{
    if( row.textCount() < 3)
        return false;
    const QString teksti = row.text();
    return teksti.contains("Arkistointitunnus", Qt::CaseInsensitive) ||
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
                    Sarake sarake( Sarake( rect.x(), rect.right(), puskuri.toUpper()));
                    sarakkeet_.append(sarake);
                    puskuri.clear();
                    rect = QRectF();
#ifdef KITSAS_DEVEL
                    std::cerr << sarake.tyyppi() << " " << puskuri.toStdString() << " _ ";
#endif
                }
            }
            if( rect.isValid() && puskuri.trimmed().length() > 0) {
                Sarake sarake( Sarake( rect.x(), rect.right(), puskuri.toUpper()));
                sarakkeet_.append(sarake);
#ifdef KITSAS_DEVEL
                std::cerr << sarake.tyyppi() << " " << puskuri.toStdString() << " _ ";
#endif
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


TilioteOtsake::Sarake::Sarake()
{

}

TilioteOtsake::Sarake::Sarake(double alku, double loppu, QString teksti)
    : alku_(alku), loppu_(loppu)
{
    lisaaTeksti(teksti.toUpper());
}

void TilioteOtsake::Sarake::lisaaTeksti(const QString &teksti)
{
    if( tyyppi_ == TUNTEMATON) {
        if( teksti.contains("KIRJAUS") || teksti.contains("MAKSUP") || teksti.contains("ARVOP"))
            tyyppi_ = PVM;
        else if(teksti.contains("VIITE"))
            tyyppi_ = VIITE;
        else if(teksti.contains("PANO") ||
                teksti.contains("MÄÄR") ||
                teksti.contains("EUR"))
            tyyppi_ = EURO;
        else if( teksti.contains("SELIT"))
            tyyppi_ = SELITE;
        else if( teksti.contains("MAKSAJA"))
            tyyppi_ = SAAJAMAKSAJA;
        else if( teksti.contains("ARKISTOINTI"))
            tyyppi_ = ARKISTOTUNNUS;
        else if(teksti.contains("TAP") ||
                teksti.contains("SALDO") ||
                teksti.contains("TILIÖINTI"))
            tyyppi_ = OHITA;
    }

    tekstit_.append(teksti);
}

}
