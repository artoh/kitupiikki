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
#include "../laskutus/iban.h"

#include <sstream>

IbanValidator::IbanValidator(QObject *parent) :
    QValidator (parent)
{

}

QValidator::State IbanValidator::validate(QString &input, int & /* pos */) const
{
    return kelpo(input);
}

QValidator::State IbanValidator::kelpo(const QString &input)
{
    QString str = input.simplified();
    str.remove(' ');

    for(int i=0; i < str.length(); i++)
    {
        QChar ch = str.at(i);
        if( i < 2 && !ch.isUpper())
            return Invalid;
        if( i > 1 && !ch.isDigit() && str.startsWith("FI"))
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

    if( Iban::ibanModulo( str ) == 1)
        return Acceptable;
    else if( input.startsWith("FI"))
        return Invalid;         // Suomalainen IBAN täsmälleen 18 merkkiä
    else
        return Intermediate;
}

bool IbanValidator::kelpaako(const QString &input)
{
    return kelpo(input) == Acceptable;
}


