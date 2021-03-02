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
#include "abstraktikielet.h"

Kieli::Kieli()
{

}

Kieli::Kieli(const QString &lyhenne, const QString &nimi) :
    lyhenne_(lyhenne), nimi_(nimi)
{

}



const QString Kieli::lyhenne() const
{
    return lyhenne_;
}



const QString Kieli::nimi() const
{
    return nimi_;
}



const QString Kieli::lippu() const
{
    return ":/liput/" + lyhenne() + ".png";
}

