/*
   Copyright (C) 2018 Arto Hyvättinen

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

#include "viitevalidator.h"
#include "laskutus/laskumodel.h"

#include "ibanvalidator.h"

ViiteValidator::ViiteValidator()
{

}

QValidator::State ViiteValidator::validate(QString &input, int & /* pos */) const
{
    QString str = input.simplified();
    str.remove(' ');


    if( str == "R" || str == "RF")
        return Intermediate;

    if( str.startsWith("RF"))
    {
        if( IbanValidator::ibanModulo(str) == 1)
            return Acceptable;
        else
            return Intermediate;
    }

    // Perinteinen pankkiviite

    for( QChar ch : str)
        if( !ch.isDigit())
            return Invalid;

    if( str.length() < 4 )
        return Intermediate;


    int tarkaste = str.right(1).toInt();
    str.chop(1);

    int indeksi = 0;
    int summa = 0;

    for( QChar ch : str)
    {
        int numero = ch.digitValue();

        if( indeksi % 3 == 0)
            summa += 7 * numero;
        else if( indeksi % 3 == 1)
            summa += 3 * numero;
        else
            summa += numero;

        indeksi++;
    }

    if (( 10 - summa % 10) % 10 == tarkaste )
        return Acceptable;
    else if( str.length() < 19)
        return Intermediate;
    else
        return Invalid; // Viitenumeron enimmäispituus 20 numeroa


}
