/*
   Copyright (C) 2017 Arto Hyvättinen

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

#ifndef TILI_H
#define TILI_H

#include <QString>

class Tili
{
public:
    Tili();
    Tili(int tnumero, const QString tnimi, const QString& tohje, const QString ttyyppi, int ttila, const QString tjson);

    int numero() const { return numero_; }
    QString nimi() const { return nimi_; }
    QString tyyppi() const { return tyyppi_; }
    int tila() const { return tila_; }



protected:
    int numero_;
    QString nimi_;
    QString ohje_;
    QString tyyppi_;
    int tila_;
    QString json_;
};

#endif // TILI_H
