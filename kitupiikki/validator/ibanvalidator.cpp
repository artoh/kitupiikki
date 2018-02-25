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
    QString str = input.simplified();
    str.remove(' ');

    for(int i=0; i < str.length(); i++)
    {
        QChar ch = str.at(i);
        if( i < 2 && !ch.isUpper())
            return Invalid;
        if( i > 1 && !ch.isDigit())
            return Invalid;
    }
    if( str.length() < 10)
        return Intermediate;

    if( str.startsWith("FI") && str.length() > 18)
        return Invalid;
    if( str.startsWith("FI") && str.length() < 18)
        return Intermediate;
    if( str.length() > 32)
        return Invalid;

    if( ibanModulo( str ) == 1)
        return Acceptable;
    else if( input.startsWith("FI"))
        return Invalid;         // Suomalainen IBAN täsmälleen 18 merkkiä
    else
        return Intermediate;

}

int IbanValidator::ibanModulo(const QString &iban)
{
    // Siirretään neljä ensimmäistä merkkiä loppuun
    QString siirto = iban.mid(4) + iban.left(4);

    // Muunnetaan kirjaimet numeropareiksi
    QString apu;
    for( QChar merkki : siirto)
    {
        if( merkki.isDigit() )
            apu.append(merkki);
        else if( merkki.isUpper())
            apu.append( QString("%1").arg( (int) merkki.toLatin1() - 55 , 2, 10, QChar('0')  ) );
        else
            return -1;
    }

    QString eka = apu.left(9);
    qlonglong luku = eka.toLongLong();
    int jaannos = luku % 97;

    if( apu.length() < 10 )
        return jaannos;

   apu.remove(0,9);

   while( apu.length() )
   {
       QString tama = QString("%1").arg( (int) jaannos , 2, 10, QChar('0')  );
       tama.append( apu.left(7));
       luku = tama.toLongLong();
       jaannos = luku % 97;

       if( apu.length() > 6)
           apu.remove(0,7);
       else
           apu.clear();
   }

    return jaannos;

}
