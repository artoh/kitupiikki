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
#include "tosite.h"

Tosite::Tosite(QObject *parent) : QObject(parent)
{

}

QVariant Tosite::data(int kentta) const
{
    return data_.value( avaimet__.at(kentta) );
}

void Tosite::setData(int kentta, QVariant arvo)
{
    if( arvo.isNull())
        data_.remove( avaimet__.at(kentta) );
    else
        data_.insert( avaimet__.at(kentta), arvo );
}


std::map<int,QString> Tosite::avaimet__ = {
    { ID, "id" },
    { PVM, "pvm "},
    { TYYPPI, "tyyppi"},
    { TILA, "tila"},
    { TUNNISTE, "tunniste"},
    { OTSIKKO, "otsikko"},
    { VIITE, "viite"},
    { ERAPVM, "erapvm"},
    { TOIMITTAJA, "toimittaja" },
    { ASIAKAS, "asiakas" },
    { INFO, "info"}
};
