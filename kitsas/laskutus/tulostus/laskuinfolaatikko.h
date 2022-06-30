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
#ifndef LASKUINFOLAATIKKO_H
#define LASKUINFOLAATIKKO_H

#include <QList>
#include <QRectF>
#include <QColor>

class QPainter;

class LaskuInfoLaatikko
{
public:
    LaskuInfoLaatikko();
    void lisaa(const QString& otsikko, const QString& teksti);
    qreal laskeKoko(QPainter* painter, qreal leveys);
    void piirra(QPainter* painter, qreal x, qreal y, const QColor &vari);

    QSizeF koko() const { return koko_;}
private:
    QList<QPair<QString,QString>> tekstit_;
    QSizeF koko_;

    const qreal pistekoko_ = 8;
};

#endif // LASKUINFOLAATIKKO_H
