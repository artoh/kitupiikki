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

#include "tuonti/pdf/pdfrivi.h"
#include "tuonti/pdf/pdfpala.h"

#include <iostream>

namespace Tuonti {

TilioteOtsake::TilioteOtsake(PdfTilioteTuonti *tuonti) :
    tuonti_(tuonti)
{

}

bool TilioteOtsake::alkaakoOtsake(PdfRivi* rivi)
{
    if( rivi->paloja() < 2 )
        return false;

    const QString teksti = rivi->teksti();
    return teksti.contains("Arkistointitunnus", Qt::CaseInsensitive) ||
           teksti.contains("Arkiveringskod", Qt::CaseInsensitive) ||
           ( teksti.contains("Kirjaus", Qt::CaseInsensitive) && ( teksti.contains("Pano", Qt::CaseInsensitive) || teksti.contains("Maksutiedot", Qt::CaseInsensitive) )) ||
           (teksti.contains("Kirj.pvm", Qt::CaseInsensitive) && teksti.contains("Arvopvm.", Qt::CaseInsensitive));
}

void TilioteOtsake::kasitteleRivi(PdfRivi *rivi)
{
    if(sarakkeet_.isEmpty()) {
        PdfPala* pala = rivi->pala();
        while(pala) {
            Tyyppi tyyppi = tyyppiTekstilla( pala->teksti().toUpper() );
            Sarake sarake(pala->vasen(), pala->oikea(), tyyppi);
            sarakkeet_.append(sarake);
            pala = pala->seuraava();
        }
    } else {
        // Myöhemmillä riveillä lisätään sarake, jos
        // vastaavalla kohdalla ei ole saraketta yläpuolella
        PdfPala* pala = rivi->pala();
        while(pala) {
            Tyyppi tyyppi = tyyppiTekstilla( pala->teksti().toUpper() );
            Sarake sarake(pala->vasen(), pala->oikea(), tyyppi);
            for(int i=0; i < sarakkeet_.count(); i++) {
                Sarake vs = sarakkeet_.at(i);
                if( qAbs(vs.alku() - sarake.alku()) < 20 ) break;
                if( sarake.alku() >= vs.alku() - 5 && sarake.loppu() <= vs.loppu() + 5) break;
                if( sarake.alku() < vs.alku() ) {
                        sarakkeet_.insert(i, sarake);
                    break;
                }
            }
            if( sarake.alku() > sarakkeet_.last().loppu()) {
                sarakkeet_.append(sarake);
            }
            pala = pala->seuraava();
        }
    }

    otsakeRivia_++;
}

bool TilioteOtsake::tarkastaRivi(PdfRivi *rivi)
{
    if( alkaakoOtsake(rivi) || tarkastusRivia_)
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


int TilioteOtsake::indeksiSijainnilla(int sijainti)
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

TilioteOtsake::Tyyppi TilioteOtsake::tyyppi(int paikka)
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
        if( teksti.contains("KIRJ.PVM") )
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
    else if( teksti.contains("MAKSUTIEDOT"))
        return YLEINEN;
    else if(teksti.contains("TAP") ||
            teksti.contains("SALDO") ||
            teksti.contains("TILIÖINTI"))
        return OHITA;
    return TUNTEMATON;
}


TilioteOtsake::Sarake::Sarake()
{

}

TilioteOtsake::Sarake::Sarake(int alku, int loppu, Tyyppi tyyppi)
    : alku_(alku), loppu_(loppu), tyyppi_(tyyppi)
{
    if( alku_ > 510 && tyyppi == EURO) {
        alku_ = 510;
    }
}

}

