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
#include "db/asetusmodel.h"
#include "laskutus/huoneisto/huoneistomodel.h"
#include "rekisteri/asiakastoimittajadlg.h"
#include "model/tositeviennit.h"

#include <QDebug>
#include <QPainter>
#include <QPagedPaintDevice>

LaskunTietoLaatikko::LaskunTietoLaatikko(KitsasInterface *kitsas) :
    kitsas_(kitsas)
{

}

void LaskunTietoLaatikko::lataa(Tosite &tosite)
{
    const Lasku& lasku = tosite.constLasku();
    const QVariantMap& kumppani = tosite.data(Tosite::KUMPPANI).toMap();

    kieli_ = lasku.kieli().toLower();

    if( tosite.tyyppi() == TositeTyyppi::HYVITYSLASKU) {
        otsikko_ = kitsas_->kaanna("hyvityslasku", kieli_);
        lisaa("hyvPvm", lasku.laskunpaiva());
        lisaa("hyvnro", lasku.numero());
        lisaa("hyvitettavaNumero", QString::number(lasku.alkuperaisNumero()));
        lisaa("hyvitettavaPvm", lasku.alkuperaisPvm());
    } else if( tosite.tyyppi() == TositeTyyppi::MAKSUMUISTUTUS) {
        otsikko_ = kitsas_->kaanna("maksumuistutus", kieli_);
        lisaa("muistutuspvm", lasku.laskunpaiva());
        lisaa("muistutusnro", lasku.numero());
    } else {
        if( lasku.maksutapa() == Lasku::KATEINEN)
            otsikko_ = kitsas_->kaanna("kateislasku", kieli_);
        else if( lasku.maksutapa() == Lasku::ENNAKKOLASKU)
            otsikko_ = kitsas_->kaanna("ennakkolasku", kieli_);
        else
            otsikko_ = kitsas_->kaanna("laskuotsikko", kieli_);

        lisaa("lpvm", lasku.laskunpaiva());
        lisaa("lnro", lasku.numero());

        if( lasku.jaksopvm().isValid()) {           
            lisaa( "laskutusjakso", QString("%1 - %2").arg( lasku.toimituspvm().toString("dd.MM.yyyy"))
                                               .arg( lasku.jaksopvm().toString("dd.MM.yyyy")));
        } else if( lasku.maksutapa() != Lasku::ENNAKKOLASKU) {
            lisaa("toimpvm", lasku.toimituspvm());
        }
    }

    const QString alvtunnus = kumppani.value("alvtunnus").toString();
    lisaa("asytunnus", AsiakasToimittajaDlg::alvToY(alvtunnus));

    if( (!alvtunnus.isEmpty() && !alvtunnus.startsWith("FI")) ||
         tosite.viennit()->onkoKaanteistaAlvia()) {
        lisaa("asalvtunnus", alvtunnus);
    }

    lisaa("asviite", lasku.asiakasViite());
    lisaa("sopimusnro", lasku.sopimusnumero());
    lisaa("myyja", lasku.myyja());
    lisaa("tilaaja", lasku.tilaaja());
    lisaa("tilauspvm", lasku.tilausPvm());
    lisaa("tilausnro", lasku.tilausNumero());
         

    if( lasku.viivastyskorko() > 1e-3)
        lisaa("viivkorko", QString("%L1 %").arg(lasku.viivastyskorko(),0,'f',1));   
    if( lasku.valvonta() == Lasku::HUONEISTO) {
        ViiteNumero viite( lasku.viite() );
        int huoneistoId = viite.numero();
        const QString& huoneistoTunnus = kitsas_->huoneistot()->tunnus(huoneistoId);
        lisaa("huoneisto", huoneistoTunnus);
    }

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

void LaskunTietoLaatikko::ylatunnisteNimialue(QPainter *painter)
{
    const QImage& logo = kitsas_->logo();
    const QString logonSijainti = kitsas_->asetukset()->asetus(AsetusModel::Logonsijainti);
    const qreal sivunleveys = painter->window().width();

    const QImage skaalattu = logo.scaled( logonSijainti == "VAINLOGO" ? sivunleveys / 2 : sivunleveys / 4 ,
                                          rivinKorkeus_ * 2,
                                          Qt::KeepAspectRatio );

    painter->drawImage(QRect(0,0,skaalattu.size().width(), skaalattu.size().height()), skaalattu);

    if( logo.isNull() && logonSijainti != "VAINLOGO" && logonSijainti != "YLLA"){
        const QString aputoiminimi = kitsas_->asetukset()->asetus(AsetusModel::Aputoiminimi);
        const QString& nimi = aputoiminimi.isEmpty() ?
                    kitsas_->asetukset()->asetus(AsetusModel::OrganisaatioNimi) :
                    aputoiminimi;
        painter->setFont( QFont("FreeSans", fonttikoko_, QFont::Normal) );
        const qreal x = logo.isNull() ? 0 : logo.size().width() + painter->fontMetrics().horizontalAdvance("M");
        const QRectF nimiRect(x, 0, sivunleveys / 2 - x, rivinKorkeus_ * 2);
        painter->drawText(nimiRect, Qt::AlignLeft | Qt::AlignVCenter, nimi );
    }
}

void LaskunTietoLaatikko::ylatunnisteOtsikko(QPainter *painter)
{
    painter->setFont( QFont("FreeSans", fonttikoko_, QFont::Bold) );
    const qreal sivunleveys = painter->window().width();
    QRectF oRect(sivunleveys / 2, 0, sivunleveys/4, rivinKorkeus_ * 2);
    painter->drawText(oRect, Qt::AlignLeft | Qt::AlignVCenter, otsikko_ );
}

void LaskunTietoLaatikko::ylatunnistePvmalue(QPainter *painter)
{
    const qreal sivunleveys = painter->window().width();
    QRectF pRect(0, 0, sivunleveys, rivinKorkeus_ * 2);
    if( kitsas_->onkoHarjoitus()) {
        painter->setPen( QPen(Qt::green ));
        painter->setFont(QFont("FreeSans", fonttikoko_ + 6, QFont::Black));
        painter->drawText( pRect, Qt::AlignHCenter | Qt::AlignVCenter,
                           kitsas_->kaanna("HARJOITUS", kieli_));
    }

    painter->setFont( QFont("FreeSans", fonttikoko_, QFont::Normal) );
    painter->drawText( pRect, Qt::AlignRight | Qt::AlignVCenter,
                       kitsas_->paivamaara().toString("dd.MM.yyyy"));

}

qreal LaskunTietoLaatikko::laskeLaatikko(QPainter *painter, qreal leveys)
{
    painter->save();
    painter->setFont( QFont("FreeSans", fonttikoko_, QFont::Normal) );
    const QFontMetrics& metrics = painter->fontMetrics();
    rivinKorkeus_ = metrics.height();
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

    piirraTulostusPaiva(painter);
    piirraLaatikko(painter);
    piirraTekstit(painter);
}

void LaskunTietoLaatikko::ylatunniste(QPainter *painter)
{
    painter->save();
    ylatunnisteNimialue(painter);
    ylatunnisteOtsikko(painter);
    ylatunnistePvmalue(painter);
    painter->restore();
    painter->translate(0, rivinKorkeus_ * 3.5);
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
    painter->drawText( harjoitusRect, Qt::AlignHCenter | Qt::AlignTop,
                       kitsas_->kaanna("HARJOITUS", kieli_));
    painter->restore();
}

void LaskunTietoLaatikko::piirraTulostusPaiva(QPainter *painter)
{
    painter->save();
    QRect harjoitusRect(0, 0, laatikko_.width(), laatikko_.y());
    painter->setFont(QFont("FreeSans", fonttikoko_));
    painter->drawText( harjoitusRect, Qt::AlignRight | Qt::AlignTop,
                       kitsas_->paivamaara().toString("dd.MM.yyyy"));
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
