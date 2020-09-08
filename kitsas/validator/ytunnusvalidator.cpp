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

#include "ytunnusvalidator.h"

YTunnusValidator::YTunnusValidator(bool alvtunnuksia, QObject *parent)
    : QValidator(parent),
      alvtunnuskelpaa(alvtunnuksia)
{

}

QValidator::State YTunnusValidator::validate(QString &input, int & /* pos */) const
{
    return kelpo(input, alvtunnuskelpaa);
}

QValidator::State YTunnusValidator::kelpo(const QString &input, bool alvkelpaa)
{
    QString str = input.simplified();

    if( alvkelpaa && str.length() && str.at(0).isLetter() )
    {
        if( str.startsWith("FI"))
        {
            str = str.mid(2);
            if( str.length() > 7)
                str.insert(7,'-');
        }
        else if( str.length() < 10)
            return Intermediate;

        else
            return Acceptable;
    }

    if( str.length() > 9 )
        return Invalid;

    for(int i=0; i < str.length(); i++)
    {
        if(( i != 7 && !str.at(i).isDigit() ) ||
            (i == 7 && str.at(7) != '-') )
            return Invalid;
    }

    if( str.length() < 9)
        return Intermediate;


    int summa = str.at(0).digitValue() * 7 +
                 str.at(1).digitValue() * 9 +
                 str.at(2).digitValue() * 10 +
                 str.at(3).digitValue() * 5 +
                 str.at(4).digitValue() * 8 +
                 str.at(5).digitValue() * 4 +
                 str.at(6).digitValue() * 2 ;

    int jaannos = summa % 11;
    int tarkaste = str.at(8).digitValue();

    if( jaannos == 1)
        return Invalid;

    if( jaannos == 0 && tarkaste == 0 )
        return Acceptable;

    if( 11 - jaannos == tarkaste)
        return Acceptable;

    return Invalid;
}

bool YTunnusValidator::kelpaako(const QString &input, bool alvtunnuksia)
{
    return kelpo(input, alvtunnuksia) == Acceptable;
}
