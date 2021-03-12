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
#include "laskuntietolaatikko.h"
#include "db/tositetyyppimodel.h"
#include "db/kitsasinterface.h"

#include <QDebug>
#include <QPainter>
#include <QPagedPaintDevice>

LaskunTietoLaatikko::LaskunTietoLaatikko(KitsasInterface *kitsas) :
    kitsas_(kitsas)
{

}

void LaskunTietoLaatikko::lataa(const Tosite &tosite)
{
    const Lasku& lasku = tosite.constLasku();
    const QVariantMap& kumppani = tosite.data(Tosite::KUMPPANI).toMap();

    if( tosite.tyyppi() == TositeTyyppi::MYYNTILASKU)
        otsikko_ = kitsas_->kaanna("laskuotsikko", kieli_);

    kieli_ = lasku.kieli().toLower();

    lisaa("lpvm", lasku.laskunpaiva());
    lisaa("lnro", lasku.numero());

    if( lasku.jaksopvm().isValid()) {
        lisaa("toimpvm", QString("%1 - %2").arg( lasku.toimituspvm().toString("dd.MM.yyyy"))
                                           .arg( lasku.jaksopvm().toString("dd.MM.yyyy")));
    } else {
        lisaa("toimpvm", lasku.toimituspvm());
    }

    lisaa("asalvtunnus", kumppani.value("alvtunnus").toString());
    lisaa("asviite", lasku.asiakasViite());
    lisaa("tilausnro", lasku.tilausNumero());
    lisaa("sopimusnro", lasku.sopimusnumero());
    lisaa("myyja", lasku.myyja());
    lisaa("tilaaja", lasku.tilaaja());
    lisaa("tilauspvm", lasku.tilausPvm());
    lisaa("tilausnro", lasku.tilausNumero());

    if( lasku.viivastyskorko() > 1e-3)
        lisaa("viivkorko", QString("%1 %").arg(lasku.viivastyskorko(),0,'f',1));

}

void LaskunTietoLaatikko::lisaa(const QString &avain, const QString &tieto)
{
    const QString& kaannos = kitsas_->kaanna(avain, kieli_);
    if( !tieto.isEmpty())
        rivit_.append( TietoRivi(kaannos, tieto) );
}

void LaskunTietoLaatikko::lisaa(const QString &avain, const QDate &pvm)
{
    if( pvm.isValid())
        lisaa( avain, pvm.toString("dd.MM.yyyy"));
}

qreal LaskunTietoLaatikko::laskeLaatikko(QPainter *painter, qreal leveys)
{
    painter->save();
    painter->setFont( QFont("FreeSans", fonttikoko_, QFont::Normal) );
    const QFontMetrics& metrics = painter->fontMetrics();
    const int rivilisa = metrics.xHeight() / 5;

    QRect otsikkoLaskuRect(0,0,leveys,painter->window().height());
    for(const auto& rivi: rivit_) {
        int leveys = painter->boundingRect( otsikkoLaskuRect, rivi.otsikko() ).width();
        if( leveys > otsikkoleveys_)
            otsikkoleveys_ = leveys;
    }
    otsikkoleveys_ += metrics.horizontalAdvance("iM");

    int tekstileveys = leveys - otsikkoleveys_ - metrics.horizontalAdvance("Mi");
    QRect tietoLaskuRect(0,0,tekstileveys, painter->window().height());

    qreal laatikonKorkeus = metrics.horizontalAdvance("ii");
    painter->setFont( QFont("FreeSans", fonttikoko_, QFont::Normal) );

    for(const auto& rivi: rivit_) {
        laatikonKorkeus += painter->boundingRect( tietoLaskuRect, Qt::TextWordWrap, rivi.tieto() ).height() + rivilisa;
    }
    laatikonKorkeus -= rivilisa;

    painter->restore();

    painter->setFont( QFont("FreeSans", fonttikoko_ + 6, QFont::Bold) );
    const QFontMetrics& otsikkoMetrics = painter->fontMetrics();

    laatikko_ = QRectF(0, otsikkoMetrics.height(), leveys, laatikonKorkeus);

    return korkeus();
}

void LaskunTietoLaatikko::piirra(QPainter *painter)
{
    if( kitsas_->onkoHarjoitus())
        piirraHarjoitus(painter);

    piirraLaatikko(painter);
    piirraTekstit(painter);
}

void LaskunTietoLaatikko::piirraLaatikko(QPainter *painter)
{
    painter->save();

    painter->setPen( Qt::NoPen );
    painter->setBrush( QBrush( QColor(230,230,230)) );   
    painter->drawRect( QRect(laatikko_.x(), laatikko_.y(),otsikkoleveys_, laatikko_.height()));

    painter->setBrush( Qt::NoBrush );

    const double mm = painter->device()->width() * 1.00 / painter->device()->widthMM();
    painter->setPen( QPen(QBrush(Qt::darkGray), 0.3 * mm  ) );
    painter->drawRect( laatikko_ );

    painter->restore();
}

void LaskunTietoLaatikko::piirraTekstit(QPainter *painter)
{
    painter->save();
    painter->setFont( QFont("FreeSans", fonttikoko_, QFont::Bold) );
    QRectF laatikonYlla(0,0,laatikko_.width(),laatikko_.height());
    painter->drawText(laatikonYlla, otsikko_);

    painter->setFont( QFont("FreeSans", fonttikoko_, QFont::Normal) );
    const QFontMetrics& metrics = painter->fontMetrics();
    const int tietoleveys = laatikko_.width() - otsikkoleveys_ - metrics.horizontalAdvance("Mi");
    const int tietoAlku = laatikko_.x() + otsikkoleveys_ + metrics.horizontalAdvance("M");
    const int marginaali = metrics.horizontalAdvance("i");
    const int rivilisa = metrics.xHeight() / 5;

    qreal y = laatikko_.y() + marginaali;

    for(const auto& rivi : rivit_) {
        QRectF otsikkoRect(laatikko_.x() + marginaali, y,
                           laatikko_.width(), laatikko_.height());
        painter->drawText( otsikkoRect, rivi.otsikko() );

        QRectF tietoLaskuRect(tietoAlku, y, tietoleveys, laatikko_.height());
        QRectF tietoRect = painter->boundingRect( tietoLaskuRect, Qt::TextWordWrap, rivi.tieto() );
        painter->drawText( tietoRect, rivi.tieto() );
        y += tietoRect.height() + rivilisa;
    }

    painter->restore();
}

void LaskunTietoLaatikko::piirraHarjoitus(QPainter *painter)
{
    painter->save();
    QRect harjoitusRect(0, 0, laatikko_.width(), laatikko_.y());
    painter->setPen( QPen(Qt::green ));
    painter->setFont(QFont("FreeSans", fonttikoko_ + 6, QFont::Black));
    painter->drawText( harjoitusRect, Qt::AlignRight | Qt::AlignTop,
                       kitsas_->kaanna("HARJOITUS", kieli_));
    painter->restore();
}

LaskunTietoLaatikko::TietoRivi::TietoRivi()
{

}

LaskunTietoLaatikko::TietoRivi::TietoRivi(const QString &otsikko, const QString &tieto) :
    otsikko_(otsikko), tieto_(tieto)
{
    qDebug() << otsikko << " : " << tieto;
}
