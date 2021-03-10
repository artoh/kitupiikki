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
#include "lasku.h"
#include "laskurivitmodel.h"

Lasku::Lasku(QObject *parent)
    : Tosite(parent)
{

}

void Lasku::lataaData(QVariant *variant)
{
    QVariantMap map = variant->toMap();

    data_ = map.take("lasku").toMap();
    rivit_->lataa( map.take("rivit").toList() );

    QVariant tositteelle( map );
    Tosite::lataaData( &tositteelle );
}

QDate Lasku::oikaiseErapaiva(QDate erapvm)
{
    while( erapvm.dayOfWeek() > 5 ||
           (erapvm.day()==1 && erapvm.month()==1) ||
           (erapvm.day()==6 && erapvm.month()==1) ||
           (erapvm.day()==1 && erapvm.month()==5) ||
           (erapvm.day()==6 && erapvm.month()==12) ||
           (erapvm.day()>= 24 && erapvm.day() <= 26 && erapvm.month()==12))
        erapvm = erapvm.addDays(1);
    return erapvm;
}
