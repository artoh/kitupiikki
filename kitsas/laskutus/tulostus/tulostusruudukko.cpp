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
#include "tulostusruudukko.h"

#include <QPainter>
#include <QPagedPaintDevice>

TulostusRuudukko::TulostusRuudukko()
{

}

void TulostusRuudukko::lisaaSarake(const QString &otsikko, Qt::AlignmentFlag tasaus, qreal vahimmaisleveys)
{
    sarakkeet_.append( Sarake(otsikko, tasaus, vahimmaisleveys));
}

void TulostusRuudukko::lisaaRivi(const QStringList &tekstit, bool lihava)
{
    rivit_.append( Rivi(tekstit, lihava));
}

void TulostusRuudukko::laske(QPainter *painter)
{
    painter->save();
    // Lasketaan ensin jokaiselle sarakkeeelle sarakkeen viemä tila
    // Aloitetaan otsikoista
    painter->setFont(QFont("FreeSans", pistekoko_ - 1, QFont::Normal));
    for(int c=0; c < sarakkeet_.count(); c++) {
        qreal leveys = painter->fontMetrics().horizontalAdvance( sarakkeet_.at(c).otsikko() );
        if( leveys > sarakkeet_[c].leveys()) {
            sarakkeet_[c].asetaLeveys(leveys);
        }
    }
    sarakevali_ = painter->fontMetrics().horizontalAdvance("MM");
    ivali_ = painter->fontMetrics().horizontalAdvance("i");

    for(const auto& rivi : rivit_) {
        for(int c=0; c < sarakkeet_.count(); c++) {
            painter->setFont(QFont("FreeSans", pistekoko_, rivi.lihava() ? QFont::Bold : QFont::Normal));
            qreal leveys = painter->fontMetrics().horizontalAdvance( rivi.teksti(c) );
            if( leveys > sarakkeet_[c].leveys()) {
                sarakkeet_[c].asetaLeveys(leveys);
            }
        }
    }

    qreal loppusarakeleveys = (sarakkeet_.count() - 1) * sarakevali_ +
        2 * ivali_;
    for(int c=1; c < sarakkeet_.count(); c++)
        loppusarakeleveys += sarakkeet_.at(c).leveys();

    if( vahimmaisLeveys_ > sarakkeet_.at(0).leveys() + loppusarakeleveys) {
        sarakkeet_[0].asetaLeveys( vahimmaisLeveys_ - loppusarakeleveys );
    } else if( enimmaisLeveys_ > 0 && enimmaisLeveys_ > sarakkeet_.at(0).leveys() + loppusarakeleveys ) {
        sarakkeet_[0].asetaLeveys( enimmaisLeveys_ - loppusarakeleveys );
    }
    qreal kokonaisleveys = loppusarakeleveys + sarakkeet_.at(0).leveys();

    painter->setFont(QFont("FreeSans", pistekoko_ - 1, QFont::Normal));
    qreal rivinkorkeus = painter->fontMetrics().height();
    QRectF mittaRect(0, 0, sarakkeet_.value(0).leveys(), painter->window().height());

    qreal kokonaiskorkeus = rivinkorkeus + ivali_;
    for(int r = 0; r < rivit_.count(); r++) {
        painter->setFont(QFont("FreeSans", pistekoko_, rivit_.at(r).lihava() ? QFont::Bold : QFont::Normal));
        QRectF rect = painter->boundingRect(mittaRect, rivit_.at(r).teksti(0));
        qreal korkeus = rect.height() > rivinkorkeus ? rect.height() : rivinkorkeus;
        rivit_[r].asetaKorkeus(korkeus);
        kokonaiskorkeus += korkeus + ivali_;
    }
    koko_ = QSizeF( kokonaisleveys, kokonaiskorkeus);

    painter->restore();
}

void TulostusRuudukko::piirra(QPainter *painter, QPagedPaintDevice *device, qreal alaMarginaali, SivunVaihtaja *vaihtaja)
{
    if( !koko_.isValid())
        laske(painter);

    qreal marginaali = alaMarginaali > 0 ? alaMarginaali : painter->window().height();
    piirraOtsikko(painter);
    for(const auto& rivi : rivit_) {
        if( painter->transform().dy() + rivi.korkeus() >= marginaali ) {
            // Pitää vaihtaa sivua!
            if( vaihtaja ) {
                marginaali = vaihtaja->vaihdaSivua(painter, device);
            } else {
                device->newPage();
                painter->resetTransform();
                marginaali = painter->window().height();
            }
            piirraOtsikko(painter);
        }
        piirraRivi(rivi, painter);
    }
}

void TulostusRuudukko::piirraOtsikko(QPainter *painter)
{
    painter->save();
    painter->setFont(QFont("FreeSans", pistekoko_ - 1, QFont::Normal));
    painter->setPen( Qt::NoPen );
    painter->setBrush( QBrush( QColor(230,230,230)) );
    painter->drawRect( QRect(0, 0, koko_.width(), painter->fontMetrics().height() + 2 * ivali_ ));
    painter->setBrush( Qt::NoBrush );
    painter->setPen( QPen(Qt::black) );

    qreal x = ivali_;
    for(const auto &sarake : sarakkeet_ ) {
        painter->drawText( QRectF(x, ivali_, sarake.leveys(), painter->fontMetrics().height()),
                           sarake.otsikko());
        x+= sarake.leveys() + sarakevali_;
    }
    painter->restore();
    painter->translate(0, painter->fontMetrics().height() + 2.5 * ivali_);
}

void TulostusRuudukko::piirraRivi(const Rivi &rivi, QPainter *painter)
{
    painter->save();
    painter->setFont(QFont("FreeSans", pistekoko_, rivi.lihava() ? QFont::Bold : QFont::Normal));
    qreal x = ivali_;
    for(int c = 0; c < sarakkeet_.count(); c++) {
        QRect alue(x, 0, sarakkeet_.at(c).leveys(), rivi.korkeus());
        painter->drawText(alue, sarakkeet_.at(c).tasaus(), rivi.teksti(c));
        x += sarakkeet_.at(c).leveys() + sarakevali_;
    }

    const double mm = painter->device()->width() * 1.00 / painter->device()->widthMM();
    painter->setPen( QPen(QBrush(QColor(230,230,230)), 0.15 * mm  ) );
    const qreal viivay = rivi.korkeus() + ivali_ ;
    painter->drawLine(0, viivay, koko_.width(), viivay);

    painter->restore();
    painter->translate(0, rivi.korkeus() + ivali_);
}

TulostusRuudukko::Sarake::Sarake()
{

}

TulostusRuudukko::Sarake::Sarake(const QString &otsikko, Qt::AlignmentFlag tasaus, qreal vahimmaisleveys) :
    otsikko_(otsikko), leveys_(vahimmaisleveys), tasaus_(tasaus)
{

}

TulostusRuudukko::Rivi::Rivi()
{

}

TulostusRuudukko::Rivi::Rivi(const QStringList &tekstit, bool lihava) :
    tekstit_(tekstit), lihava_(lihava)
{

}
