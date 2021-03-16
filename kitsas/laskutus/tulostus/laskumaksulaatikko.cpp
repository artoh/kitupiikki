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
#include "laskumaksulaatikko.h"

#include <QPainter>

LaskuMaksuLaatikko::LaskuMaksuLaatikko()
{

}

void LaskuMaksuLaatikko::lisaa(const QString &otsikko, const QString &teksti, Qt::AlignmentFlag tasaus, bool lihava)
{
    LaatikkoSarake sarake(otsikko, teksti, tasaus, lihava);
    sarakkeet_.append( sarake );
}

qreal LaskuMaksuLaatikko::laske(QPainter *painter, qreal leveys)
{
    painter->setFont(QFont("FreeSans", LaskuMaksuLaatikko::TEKSTI_KOKO));
    int ileveys = painter->fontMetrics().horizontalAdvance("i");

    qreal korkein = 0.0;
    qreal valjyys = leveys - 2 * ileveys;

    for(int i=0; i < sarakkeet_.count(); i++) {
        const QSizeF skoko = sarakkeet_[i].laske(painter);
        if( skoko.height() > korkein)
            korkein = skoko.height();
        valjyys -= skoko.width();
    }
    vali_ = sarakkeet_.count() > 1 ? valjyys / ( sarakkeet_.count() - 1 ) : 0;
    koko_ = QSizeF( leveys, korkein + 2 * ileveys);

    return koko_.height();
}

void LaskuMaksuLaatikko::piirra(QPainter *painter, qreal x, qreal y)
{
    painter->save();

    const double mm = painter->device()->width() * 1.00 / painter->device()->widthMM();
    painter->setPen( QPen(QBrush(Qt::darkGray), 0.3 * mm  ) );
    painter->drawRect( QRectF(x, y, koko_.width(), koko_.height()));

    painter->setPen(QPen(Qt::black));

    painter->setFont(QFont("FreeSans", LaskuMaksuLaatikko::TEKSTI_KOKO));
    int ileveys = painter->fontMetrics().horizontalAdvance("i");

    x += ileveys;
    y += ileveys;

    for( const auto& sarake : sarakkeet_) {
        sarake.piirra(painter, x, y);
        x += sarake.koko().width() + vali_;
    }

    painter->restore();
}

LaskuMaksuLaatikko::LaatikkoSarake::LaatikkoSarake()
{

}

LaskuMaksuLaatikko::LaatikkoSarake::LaatikkoSarake(QString otsikko,  QString teksti, Qt::AlignmentFlag tasaus, bool lihava) :
    otsikko_(otsikko), teksti_(teksti), tasaus_(tasaus), lihava_(lihava)
{

}
void LaskuMaksuLaatikko::LaatikkoSarake::piirra(QPainter *painter, qreal x, qreal y) const
{
    painter->save();

    painter->setFont(QFont("FreeSans", LaskuMaksuLaatikko::OTSIKKO_KOKO));
    QRectF oRect = painter->boundingRect(QRect(x,y,koko_.width(), koko_.height()),
                                         otsikko_);
    painter->drawText(oRect, otsikko_);

    painter->setFont(QFont("FreeSans", LaskuMaksuLaatikko::TEKSTI_KOKO,
                           lihava_ ? QFont::Bold : QFont::Normal));
    painter->drawText( QRectF(x, y + oRect.height(), koko_.width(), koko_.height()),
                       tasaus_,
                       teksti_);
    painter->restore();
}

QSizeF LaskuMaksuLaatikko::LaatikkoSarake::laske(QPainter *painter)
{
    painter->save();

    painter->setFont(QFont("FreeSans", LaskuMaksuLaatikko::OTSIKKO_KOKO));
    QRectF oRect = painter->boundingRect(QRect(0,0,painter->window().width(), painter->window().height()),
                                         otsikko_);
    painter->setFont(QFont("FreeSans", LaskuMaksuLaatikko::TEKSTI_KOKO,
                           lihava_ ? QFont::Bold : QFont::Normal));
    QRectF tRect = painter->boundingRect(QRect(0, 0, painter->window().width(), painter->window().height()),
                                         teksti_);

    koko_ = QSizeF( oRect.width() > tRect.width() ? oRect.width() : tRect.width(),
                    oRect.height() + tRect.height());

    painter->restore();
    return koko_;
}
