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
#include "abstraktinaytin.h"
#include "db/kirjanpito.h"


Naytin::AbstraktiNaytin::AbstraktiNaytin( QObject *parent)
    :  QObject (parent)
{

}

Naytin::AbstraktiNaytin::~AbstraktiNaytin()
{

}

void Naytin::AbstraktiNaytin::raidoita(bool raidat)
{
    raidat_ = raidat;
    paivita();
}

void Naytin::AbstraktiNaytin::asetaSuunta(QPageLayout::Orientation /*suunta*/)
{

}

QPrinter *Naytin::AbstraktiNaytin::printer()
{
    return kp()->printer();
}
