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
#include "tuontiapu.h"

#include <QString>

TuontiApu::TuontiApu()
{

}

qlonglong TuontiApu::sentteina(QString merkkijono)
{
    // Sallittuja ilmaisutapoja
    // 1 234,56
    // 1.123,56
    // 1,452.52
    // -12,34
    // -1,5
    // 1,5-
    // 10

    bool miinus = merkkijono.contains('-');
    qlonglong sentit = 0;
    merkkijono.remove('-').remove('+').remove(' ');

    int pilkkuindeksi = merkkijono.lastIndexOf(',');
    int pisteindeksi = merkkijono.lastIndexOf('.');

    int desindeksi = pilkkuindeksi > pisteindeksi ? pilkkuindeksi : pisteindeksi;

    if( desindeksi > -1 && ( desindeksi == merkkijono.length()-3 || desindeksi == merkkijono.length()-2) )
    {
        if( desindeksi == merkkijono.length()-3)
            sentit = merkkijono.mid(desindeksi+1).toLongLong();
        else
            sentit = merkkijono.mid(desindeksi+1).toLongLong() * 10L;

        merkkijono.truncate(desindeksi);
    }
    merkkijono.remove(',').remove('.');

    sentit += merkkijono.toLongLong() * 100L;

    return miinus ? -sentit : sentit ;
}
