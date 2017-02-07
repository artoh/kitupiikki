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

#include "kohdennus.h"


Kohdennus::Kohdennus(const QString &nimi) :
    id_(0), nimi_(nimi), muokattu_(false)
{

}

Kohdennus::Kohdennus(int id, QString nimi, QDate alkaa, QDate paattyy)
    : id_(id), nimi_(nimi), alkaa_(alkaa), paattyy_(paattyy)
{

}

void Kohdennus::asetaId(int id)
{
    id_ = id;
    muokattu_ = true;
}

void Kohdennus::asetaNimi(const QString &nimi)
{
    nimi_ = nimi;
    muokattu_ = true;
}

void Kohdennus::asetaAlkaa(const QDate &alkaa)
{
    alkaa_ = alkaa;
    muokattu_ = true;
}

void Kohdennus::asetaPaattyy(const QDate &paattyy)
{
    paattyy_ = paattyy;
    muokattu_ = true;
}

void Kohdennus::nollaaMuokattu()
{
    muokattu_ = false;
}
