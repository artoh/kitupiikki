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
#ifndef ESIKATSELTAVA_H
#define ESIKATSELTAVA_H

#include <QString>

class QPagedPaintDevice;
class NaytinIkkuna;


/**
 * @brief Esikatseluikkunassa esitettävää ainesta
 */
class Esikatseltava
{
public:
    Esikatseltava();
    virtual ~Esikatseltava();

    void esikatsele();
    QByteArray pdf() const;

    virtual void tulosta(QPagedPaintDevice * printer) const = 0 ;
    virtual QString otsikko() const { return QString(); }


};

#endif // ESIKATSELTAVA_H
