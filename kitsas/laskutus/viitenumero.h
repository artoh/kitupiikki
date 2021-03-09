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
#ifndef VIITENUMERO_H
#define VIITENUMERO_H

#include <QString>

class ViiteNumero
{
public:

    enum ViiteNumeroTyyppi {
        VIRHEELLINEN = 0,
        LASKU = 1,
        VAKIOVIITE = 2,
        ASIAKAS = 3,
        KOHDE = 4
    };

    ViiteNumero();
    ViiteNumero(const QString& viite);
    ViiteNumero(ViiteNumeroTyyppi tyyppi, qlonglong kanta);
    ViiteNumero(ViiteNumeroTyyppi tyyppi, const QString& kanta);

    ViiteNumeroTyyppi tyyppi() const { return tyyppi_;};
    QString viite() const { return viitenumero_;}

    QString valeilla() const;
    QString rfviite() const;

    QString kanta() const;
    qlonglong numero() const;
    int eraId() const;

    static QString laskeTarkaste(const QString& pohja);

protected:
    static ViiteNumeroTyyppi tyyppiMerkista(const QString& merkki);

private:

    QString viitenumero_;
    ViiteNumeroTyyppi tyyppi_ = VIRHEELLINEN;
    QString kanta_;

};
#endif // VIITENUMERO_H
