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
#include "tools/pdf/pdfanalyzerrow.h"

namespace Tuonti {

class TilioteOtsake
{
public:
    enum Tyyppi { TUNTEMATON, SAAJAMAKSAJA, SELITE, PVM,
                  VIITE, ARKISTOTUNNUS, EURO, OHITA, YLEINEN};

protected:
    class Sarake {
    public:
        Sarake();
        Sarake(double alku, double loppu, QString teksti);

        double alku() const { return alku_;}
        double loppu() const { return loppu_;}
        Tyyppi tyyppi() const { return tyyppi_;}

        void lisaaTeksti(const QString& teksti);

    private:
        QStringList tekstit_;
        double alku_;
        double loppu_;
        Tyyppi tyyppi_ = TUNTEMATON;
    };

public:
    TilioteOtsake();
    bool alkaakoOtsake(const PdfAnalyzerRow& row);
    void kasitteleRivi(const PdfAnalyzerRow& row);
    bool tarkastaRivi(const PdfAnalyzerRow& row);

    int sarakkeita() const;
    Sarake sarake(int indeksi) const;
    int rivia() const { return otsakeRivia_;}

    int indeksiSijainnilla(double sijainti);
    Tyyppi tyyppi(double paikka);

protected:
    QList<Sarake> sarakkeet_;
    int otsakeRivia_ = 0;
    int tarkastusRivia_ = 0;
};

}

#endif // TILIOTEOTSAKE_H
