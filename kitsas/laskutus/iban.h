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
#ifndef IBAN_H
#define IBAN_H

#include <QString>

class Iban
{
public:
    Iban();
    Iban(const QString& tilinumero);

    QString bic() const;
    QString valeilla() const;
    QString valeitta() const;

    static QString lisaaValit(const QString& iban);

    /**
     * @brief Laskee IBAN-muotoisen numeron tarkasteen (mod 97)
     * @param iban
     * @return Tarkastenumero, tai -1 jos virheellinen
     */
    static int ibanModulo(const QString& iban);

private:
    QString tilinumero_;
};

#endif // IBAN_H
