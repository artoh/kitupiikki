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


ViiteValidator::ViiteValidator()
{

}

QValidator::State ViiteValidator::validate(QString &input, int & /* pos */) const
{

    if( input == "R" || input == "RF")
        return Intermediate;

    if( input.startsWith("RF"))
    {
        for(int i=2; i < input.length(); i++)
            if( !input.at(i).isDigit())
                return Invalid;

        if( input.length() < 8)
            return Intermediate;

        QString apu = input.mid(4);
        apu.append("2715");     // RF
        apu.append(input.mid(2,2));

        qlonglong lasku = apu.toLongLong();
        if( lasku % 97 == 1)
            return Acceptable;
        if( input.length() >= 24 )
            return Invalid;
        return Intermediate;
    }



    // Perinteinen pankkiviite

    for( QChar ch : input)
        if( !ch.isDigit())
            return Invalid;

    if( input.length() > 3)
    {
        qlonglong lukuna = input.toLongLong();

        qlonglong laskettava = lukuna / 10;
        int tarkaste = lukuna % 10;
        if( LaskuModel::laskeViiteTarkiste(laskettava) == tarkaste )
            return Acceptable;
        else if( input.length() >= 20)
            return Invalid;
        else
            return Intermediate;
    }


}
