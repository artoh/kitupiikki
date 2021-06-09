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
#include "laskuinfolaatikko.h"

#include <QPainter>
#include "db/kirjanpito.h"

LaskuInfoLaatikko::LaskuInfoLaatikko()
{

}

void LaskuInfoLaatikko::lisaa(const QString &otsikko, const QString &teksti)
{
    tekstit_.append(qMakePair(otsikko, teksti));
}

qreal LaskuInfoLaatikko::laskeKoko(QPainter *painter, qreal leveys)
{
    painter->save();
    qreal korkeus = 0;
    qreal tleveys = 0.0;
    for(const auto& teksti : qAsConst( tekstit_)) {
        const QString& otsikko = teksti.first.isEmpty() ?
                    QString() :
                    teksti.first + " ";
        const QString& leipa = teksti.second;

        painter->setFont(QFont("FreeSans", pistekoko_, QFont::Bold));
        QRectF oRect = painter->boundingRect( QRectF(0, 0, leveys, painter->window().height()),
                                              otsikko);

        painter->setFont(QFont("FreeSans", pistekoko_, QFont::Normal));
        QRectF lRect = painter->boundingRect( QRectF(0, 0, leveys - oRect.width(), painter->window().height()),
                                              Qt::TextWordWrap, leipa);
        korkeus += lRect.height() > oRect.height() ?
                    lRect.height() : oRect.height();
        qreal sleveys = lRect.width() + oRect.width();
        if( sleveys > tleveys) tleveys = sleveys;
    }
    koko_ = QSizeF( tleveys, korkeus);
    painter->restore();
    return korkeus;
}

void LaskuInfoLaatikko::piirra(QPainter *painter, qreal x, qreal y)
{
    painter->save();
    for(const auto& teksti : qAsConst( tekstit_ )) {
        const QString& otsikko = teksti.first.isEmpty() ?
                    QString() :
                    teksti.first + " ";
        const QString& leipa = teksti.second;

        painter->setFont(QFont("FreeSans", pistekoko_, QFont::Bold));
        painter->setPen(QPen( kp()->asetukset()->vari(AsetusModel::VariKehys), Qt::darkGray ));
        QRectF oRect = painter->boundingRect( QRectF(x, y, koko_.width(), koko_.height()),
                                              otsikko);
        painter->drawText(oRect, otsikko);

        painter->setFont(QFont("FreeSans", pistekoko_, QFont::Normal));
        painter->setPen(QPen(Qt::black));
        QRectF lRect = painter->boundingRect( QRectF(x + oRect.width(), y, koko_.width() - oRect.width(), koko_.height()),
                                              Qt::TextWordWrap, leipa);
        painter->drawText(lRect, Qt::TextWordWrap, leipa);
        y += lRect.height() > oRect.height() ?
             lRect.height() : oRect.height();
    }
    painter->restore();
}
