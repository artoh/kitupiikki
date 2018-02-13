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

#include "ibanvalidator.h"

#include <sstream>

IbanValidator::IbanValidator()
{

}

QValidator::State IbanValidator::validate(QString &input, int & /* pos */) const
{
    for(int i=0; i < input.length(); i++)
    {
        QChar ch = input.at(i);
        if( i < 2 && !ch.isUpper())
            return Invalid;
        if( i > 1 && !ch.isDigit())
            return Invalid;
    }
    if( input.length() < 10)
        return Intermediate;

    if( input.startsWith("FI") && input.length() > 18)
        return Invalid;
    if( input.startsWith("FI") && input.length() < 18)
        return Intermediate;

    QString apu = input.mid(4);

    apu.append( QString("%1").arg( ((int) input.at(0).toLatin1()) - 55 , 2, 10, QChar('0')  ) );
    apu.append( QString("%1").arg( ((int) input.at(1).toLatin1()) - 55 , 2, 10, QChar('0')  ) );
    apu.append( input.mid(2,2));

    // https://en.wikipedia.org/wiki/International_Bank_Account_Number

    unsigned __int128 lasku = 0;
    for(QChar merkki : apu)
    {
        lasku = lasku * 10 + (int) merkki.toLatin1() - 48;
    }


    // qulonglong lasku = apu.toULongLong();
    if( lasku % 97 == 1)
        return Acceptable;
    if( input.startsWith("FI"))     // Suomalainen iban aina 18 merkkiä
        return Invalid;

    return Intermediate;
}
