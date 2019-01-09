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

#ifndef TILINPAATOSTULOSTAJA_H
#define TILINPAATOSTULOSTAJA_H

#include <QTextDocument>
#include <QPagedPaintDevice>
#include "db/tilikausi.h"

/**
 * @brief Tilinpäätöksen tulostus
*/
class TilinpaatosTulostaja
{
public:

    static void tulostaTilinpaatos(QPagedPaintDevice* writer,Tilikausi tilikausi, const QString &teksti);
private:
    static void tulostaKansilehti(Tilikausi tilikausi, QPainter *painter);

};

#endif // TILINPAATOSTULOSTAJA_H
