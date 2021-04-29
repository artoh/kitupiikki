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

void TulostusRuudukko::lisaaSummaRivi(const QString &otsikko, const QString &summa)
{
    summarivit_.append(qMakePair(otsikko, summa));
}

void TulostusRuudukko::laske(QPainter *painter)
{
    if( rivit_.isEmpty())
        return;

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

    qreal summaleveys = 0.0;
    int viimeSarakeIndeksi = sarakkeet_.count() - 1;

    for(const auto& summarivi : summarivit_) {
        painter->setFont(QFont("FreeSans", pistekoko_, QFont::Bold));
        qreal soleveys = painter->fontMetrics().horizontalAdvance( summarivi.first);
        if( soleveys > summaleveys )
            summaleveys = soleveys;
        qreal ssleveys = painter->fontMetrics().horizontalAdvance( summarivi.second );
        if( ssleveys > sarakkeet_.value( viimeSarakeIndeksi ).leveys()) {
            sarakkeet_[ viimeSarakeIndeksi ].asetaLeveys(ssleveys);
        }
    }

    qreal loppusarakeleveys = (sarakkeet_.count() - 1) * sarakevali_ +
        2 * ivali_;
    for(int c=1; c < sarakkeet_.count(); c++)
        loppusarakeleveys += sarakkeet_.at(c).leveys();

    if( vahimmaisLeveys_ > sarakkeet_.at(0).leveys() + loppusarakeleveys) {
        sarakkeet_[0].asetaLeveys( vahimmaisLeveys_ - loppusarakeleveys );
    }
    if( enimmaisLeveys_ > 0 &&
        enimmaisLeveys_ < sarakkeet_.at(0).leveys() + loppusarakeleveys ) {
        sarakkeet_[0].asetaLeveys( enimmaisLeveys_ - loppusarakeleveys );
    }
    qreal kokonaisleveys = loppusarakeleveys + sarakkeet_.at(0).leveys();

    painter->setFont(QFont("FreeSans", pistekoko_, QFont::Normal));
    qreal rivinkorkeus = painter->fontMetrics().height();
    QRectF mittaRect(0, 0, sarakkeet_.value(0).leveys(), painter->window().height());

    qreal kokonaiskorkeus = rivinkorkeus + 2 * ivali_;

    for(int r = 0; r < rivit_.count(); r++) {
        painter->setFont(QFont("FreeSans", pistekoko_, rivit_.at(r).lihava() ? QFont::Bold : QFont::Normal));
        QRectF rect = painter->boundingRect(mittaRect, rivit_.at(r).teksti(0));
        qreal korkeus = rect.height() > rivinkorkeus ? rect.height() : rivinkorkeus;
        korkeus = korkeus + 2 * ivali_;
        rivit_[r].asetaKorkeus(korkeus );
        kokonaiskorkeus += korkeus;
    }
    painter->setFont(QFont("FreeSans", pistekoko_, QFont::Normal));

    qreal summarivienleveys = summaleveys + sarakkeet_.value( viimeSarakeIndeksi ).leveys() + sarakevali_ + 2 * ivali_;
    qreal summarivienkorkeus =  ( painter->fontMetrics().height() + ivali_ * 2 ) * summarivit_.count();

    kokonaiskorkeus += summarivienkorkeus;

    koko_ = QSizeF( kokonaisleveys, kokonaiskorkeus);
    summakoko_ = QSizeF( summarivienleveys, summarivienkorkeus );

    painter->restore();
}

qreal TulostusRuudukko::piirra(QPainter *painter, QPagedPaintDevice *device, qreal alaMarginaali, SivunVaihtaja *vaihtaja)
{
    qreal marginaali = alaMarginaali > 0 ? alaMarginaali : painter->window().height();
    if( rivit_.isEmpty())
        return marginaali;

    if( !koko_.isValid())
        laske(painter);

    piirraOtsikko(painter);
    for(int r=0; r < rivit_.count(); r++ ) {
        const Rivi& rivi = rivit_.at(r);
        if( (painter->transform().dy() + rivi.korkeus() >= marginaali) ||
             (r == rivit_.count() - 1
                && painter->transform().dy() + rivi.korkeus() + summakoko_.height() >= marginaali) ) {
            // Pitää vaihtaa sivua!
            // Varmistetaan, että summarivien ylle tulee ainakin yksi laskurivi, jotta
            // lasku näyttää selkeämmältä
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
    piiraSummaRivit(painter);
    return marginaali;
}

void TulostusRuudukko::piirraOtsikko(QPainter *painter)
{
    painter->save();
    painter->setFont(QFont("FreeSans", pistekoko_ - 1, QFont::Normal));
    const double rivinkorkeus = painter->fontMetrics().height();
    const double mm = painter->device()->width() * 1.00 / painter->device()->widthMM();
    painter->setPen( QPen(QBrush(QColor(230,230,230)), 0.15 * mm  ) );
    painter->setBrush( QBrush( QColor(230,230,230)) );
    painter->drawRect( QRect(0, 0, koko_.width(), rivinkorkeus + 2 * ivali_ ));
    painter->setBrush( Qt::NoBrush );
    painter->setPen( QPen(Qt::black) );

    qreal x = ivali_;
    for(const auto &sarake : sarakkeet_ ) {
        painter->drawText( QRectF(x, ivali_, sarake.leveys(), rivinkorkeus),
                           sarake.otsikko());
        x+= sarake.leveys() + sarakevali_;
    }
    painter->restore();
    painter->translate(0, rivinkorkeus + ivali_ * 2);
}

void TulostusRuudukko::piirraRivi(const Rivi &rivi, QPainter *painter)
{
    painter->save();
    painter->setFont(QFont("FreeSans", pistekoko_, rivi.lihava() ? QFont::Bold : QFont::Normal));
    qreal x = ivali_;
    for(int c = 0; c < sarakkeet_.count(); c++) {
        QRect alue(x, ivali_, sarakkeet_.at(c).leveys(), rivi.korkeus());
        painter->drawText(alue, sarakkeet_.at(c).tasaus(), rivi.teksti(c));
        x += sarakkeet_.at(c).leveys() + sarakevali_;
    }

    const double mm = painter->device()->width() * 1.00 / painter->device()->widthMM();
    painter->setPen( QPen(QBrush(QColor(230,230,230)), 0.15 * mm  ) );
    const qreal viivay = rivi.korkeus();
    painter->drawLine(0, viivay, koko_.width(), viivay);
    painter->drawLine(0, 0 - ivali_ * 2, 0, viivay);
    painter->drawLine(koko_.width(), 0 - ivali_ * 2, koko_.width(), viivay);

    painter->restore();
    painter->translate(0, rivi.korkeus());
}

void TulostusRuudukko::piiraSummaRivit(QPainter *painter)
{
    painter->setFont(QFont("FreeSans", pistekoko_, QFont::Normal));
    qreal viimeSarakeLeveys = sarakkeet_.value( sarakkeet_.count() - 1 ).leveys();

    QRectF otsikkoRect( koko_.width() - summaKoko().width() + ivali_,
                        ivali_,
                        summakoko_.width() - viimeSarakeLeveys - 2 * ivali_ - sarakevali_,
                        painter->fontMetrics().height());

    QRectF summaRect( koko_.width() - ivali_ - viimeSarakeLeveys,
                      ivali_,
                      viimeSarakeLeveys,
                      painter->fontMetrics().height() );

    for(int r=0; r < summarivit_.count(); r++) {
        const auto& summarivi = summarivit_.at(r);
        // Alin summarivi eli kokonaissumma on lihavoitu
        if( r == summarivit_.count() - 1)
            painter->setFont(QFont("FreeSans", pistekoko_, QFont::Bold));

        painter->drawText( otsikkoRect, summarivi.first );
        painter->drawText( summaRect, Qt::AlignRight, summarivi.second);
        painter->translate(0, otsikkoRect.height() + ivali_ * 2);
    }

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
