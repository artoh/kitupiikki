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
#ifndef LASKUNOSOITEALUE_H
#define LASKUNOSOITEALUE_H

#include "model/tosite.h"

#include <QRect>

class KitsasInterface;
class QPainter;
class QPagedPaintDevice;

class LaskunOsoiteAlue
{
public:
    LaskunOsoiteAlue( KitsasInterface* kitsas);
    void lataa(const Tosite& tosite);

    qreal laske(QPainter* painter, QPagedPaintDevice* device);
    void piirra(QPainter* painter);

    qreal korkeus() const { return korkeus_;}
    qreal leveys() const { return  leveys_;}

protected:
    QRect kuorenIkkuna(QPagedPaintDevice *device) const;

private:
    KitsasInterface *kitsas_;

    const int fonttikoko_ = 10;

    QRectF logoRect_;
    QRectF nimiRect_;
    QRectF lahettajanOsoiteRect_;
    QRectF vastaanottajaRect_;

    QString nimi_;
    QString lahettajaOsoite_;
    QString vastaanottaja_;
    bool tulostettava_ = false;

    qreal korkeus_;
    qreal leveys_;
};

#endif // LASKUNOSOITEALUE_H
