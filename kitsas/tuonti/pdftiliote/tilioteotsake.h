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
#ifndef TILIOTEOTSAKE_H
#define TILIOTEOTSAKE_H

#include <QStringList>

namespace Tuonti {

class PdfRivi;
class PdfPala;
class PdfTilioteTuonti;

class TilioteOtsake
{
public:
    enum Tyyppi { TUNTEMATON, SAAJAMAKSAJA, SELITE, PVM,
                  VIITE, ARKISTOTUNNUS, EURO, OHITA, YLEINEN};

protected:
    class Sarake {
    public:
        Sarake();
        Sarake(int alku, int loppu, Tyyppi tyyppi);

        double alku() const { return alku_;}
        double loppu() const { return loppu_;}
        Tyyppi tyyppi() const { return tyyppi_;}

    private:
        int alku_;
        int loppu_;
        Tyyppi tyyppi_ = TUNTEMATON;
    };

public:
    TilioteOtsake(PdfTilioteTuonti *tuonti);
    bool alkaakoOtsake(PdfRivi* rivi);
    void kasitteleRivi(PdfRivi* rivi);
    bool tarkastaRivi(PdfRivi* rivi);

    int sarakkeita() const;
    Sarake sarake(int indeksi) const;
    int rivia() const { return otsakeRivia_;}

    int indeksiSijainnilla(int sijainti);
    Tyyppi tyyppi(int paikka);

    QString debugInfo() const;
    static QString tyyppiTeksti(Tyyppi tyyppi);

    Tyyppi tyyppiTekstilla(const QString& teksti);

protected:
    PdfTilioteTuonti* tuonti_;
    QList<Sarake> sarakkeet_;
    int otsakeRivia_ = 0;
    int tarkastusRivia_ = 0;


};

}

#endif // TILIOTEOTSAKE_H
