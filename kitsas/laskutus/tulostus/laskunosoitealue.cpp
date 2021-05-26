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
#include "laskunosoitealue.h"
#include "db/kitsasinterface.h"
#include "db/asetusmodel.h"
#include "rekisteri/maamodel.h"

#include <QVariantMap>
#include <QPainter>
#include <QPaintDevice>
#include <QPagedPaintDevice>

LaskunOsoiteAlue::LaskunOsoiteAlue(KitsasInterface *kitsas) :
    kitsas_(kitsas)
{
    const AsetusModel* asetukset = kitsas->asetukset();

    if( kitsas->logo().size().isEmpty() ||
        asetukset->asetus(AsetusModel::Logonsijainti) != "VAINLOGO" ) {
        const QString& aputoiminimi = asetukset->asetus(AsetusModel::Aputoiminimi);
        nimi_ = aputoiminimi.isEmpty() ?
                kitsas->asetukset()->asetus(AsetusModel::OrganisaatioNimi) :
                aputoiminimi;
    }
    lahettajaOsoite_ = isoIkkuna() ?
            asetukset->asetus(AsetusModel::Katuosoite) + "\n" +
            asetukset->asetus(AsetusModel::Postinumero) + " " + asetukset->asetus(AsetusModel::Kaupunki)
              : QString();

}

void LaskunOsoiteAlue::lataa(const Tosite &tosite)
{
    const QVariantMap& kumppani = tosite.data(Tosite::KUMPPANI).toMap();
    if( kumppani.value("osoite").toString().isEmpty()) {
        const Lasku& lasku = tosite.constLasku();
        vastaanottaja_ = lasku.osoite();
    } else {

        vastaanottaja_ = MaaModel::instanssi()->muotoiltuOsoite(kumppani);
    }

    tulostettava_ = tosite.constLasku().lahetystapa() == Lasku::TULOSTETTAVA ||
                    tosite.constLasku().lahetystapa() == Lasku::POSTITUS ||
                    tosite.constLasku().lahetystapa() == Lasku::PDF;
}

qreal LaskunOsoiteAlue::laske(QPainter *painter, QPagedPaintDevice *device)
{
    const double mm = device->width() * 1.00 / device->widthMM();
    painter->setFont( QFont("FreeSans",fonttikoko_) );

    int sivunLeveys = painter->window().width();

    const QRectF ikkuna = kuorenIkkuna(device);

    const QImage& logo = kitsas_->logo();
    const QString logonSijainti = logo.height() ? kitsas_->asetukset()->asetus(AsetusModel::Logonsijainti) : QString();


    QRectF lahettajaAlue =
            ikkuna.isNull() ? QRectF(0, 0 , sivunLeveys / 2 - 10 * mm, 35 * mm ) :
            ( isoIkkuna() ? QRect( ikkuna.x(), ikkuna.y(), ikkuna.width(), 25 * mm )
                        : QRect( 0, 0, sivunLeveys / 2, ikkuna.y() - 10 * mm  ));

    nimiFonttiKoko_ = isoIkkuna() ? fonttikoko_ + 1 :
                                    ( logo.isNull() ? fonttikoko_ + 4 : fonttikoko_ + 2);

    painter->setFont( QFont("FreeSans", nimiFonttiKoko_));

    nimiRect_ = painter->boundingRect( lahettajaAlue, Qt::TextWordWrap, nimi_ );

    painter->setFont( QFont("FreeSans", fonttikoko_ - 1) );
    lahettajanOsoiteRect_ = painter->boundingRect(lahettajaAlue, lahettajaOsoite_ );
    qreal logoMaxKorkeus = mm * 20;

    if( logonSijainti.isEmpty()) {
        // Ei logoa
        nimiRect_.moveTo( lahettajaAlue.x(), lahettajaAlue.y() );
        lahettajanOsoiteRect_.moveTo( lahettajaAlue.x(), lahettajaAlue.y() + nimiRect_.height() );
    } else if( logonSijainti == "VAINLOGO") {        
        qreal lkorkeus = lahettajaAlue.height() - lahettajanOsoiteRect_.height();
        QSize size = logo.scaled(lahettajaAlue.width(),
                                 qMin(logoMaxKorkeus, lkorkeus),
                                 Qt::KeepAspectRatio ).size();
        logoRect_ = QRectF( lahettajaAlue.x(), lahettajaAlue.y(),
                            size.width(), size.height());
        lahettajanOsoiteRect_.moveTo(lahettajaAlue.x(), lahettajaAlue.y() + logoRect_.height());
    } else if( logonSijainti == "YLLA") {
        qreal lkorkeus = lahettajaAlue.height() - nimiRect_.height() - lahettajanOsoiteRect_.height();
        QSize size = logo.scaled(lahettajaAlue.width(),
                                 qMin(logoMaxKorkeus, lkorkeus),
                                 Qt::KeepAspectRatio).size();
        logoRect_ = QRectF( lahettajaAlue.x(), lahettajaAlue.y(),
                            size.width(), size.height());
        nimiRect_.moveTo(lahettajaAlue.x(), lahettajaAlue.y() +  logoRect_.height());
        lahettajanOsoiteRect_.moveTo(lahettajaAlue.x(), lahettajaAlue.y() +  logoRect_.height() + nimiRect_.height());
    } else  {
        qreal leveampi = nimiRect_.width() > lahettajanOsoiteRect_.width()
                        ? nimiRect_.width()
                        : lahettajanOsoiteRect_.width();
        qreal jaljella = lahettajaAlue.width() - leveampi;
        qreal tekstienKorkeus = nimiRect_.height() + lahettajanOsoiteRect_.height();

        QSize size = logo.scaled(jaljella,
                                 qMax(tekstienKorkeus, logoMaxKorkeus),
                                 Qt::KeepAspectRatio).size();
        logoRect_ = QRectF( lahettajaAlue.x(), lahettajaAlue.y(),
                            size.width(), size.height());

        qreal korkeuttaYli = size.height() - nimiRect_.height() - lahettajanOsoiteRect_.height();
        nimiRect_.moveTo( lahettajaAlue.x() + logoRect_.width(),
                          lahettajaAlue.y() + korkeuttaYli / 2);
        lahettajanOsoiteRect_.moveTo( lahettajaAlue.x() + logoRect_.width(),
                                      lahettajaAlue.y() + korkeuttaYli / 2 + nimiRect_.height());

    }

    painter->setFont( QFont("FreeSans", fonttikoko_ ) );
    if( ikkuna.isNull()) {
        qreal alemmalla = logoRect_.bottom() > lahettajanOsoiteRect_.bottom() ?
                    logoRect_.bottom() : lahettajanOsoiteRect_.bottom();
        QRectF alue = QRectF( lahettajaAlue.x(), alemmalla + 7 * mm,
                             lahettajaAlue.width(), lahettajaAlue.height());

        vastaanottajaRect_ = painter->boundingRect( alue,  vastaanottaja_ );
        korkeus_ = vastaanottajaRect_.bottom() + 5 * mm;
    } else {
        vastaanottajaRect_ = isoIkkuna() ?
                    QRect( ikkuna.x(), ikkuna.y() + 28 * mm,
                           ikkuna.width(), ikkuna.height() - 28 * mm)
                  : ikkuna;

        korkeus_ = ikkuna.bottom() + 15 * mm;
    }
    leveys_ = ikkuna.isNull() ? lahettajaAlue.right() : ikkuna.right() + 10 * mm;
    return korkeus();
}

void LaskunOsoiteAlue::piirra(QPainter *painter)
{
    painter->save();

    const QImage& logo = kitsas_->logo();

    painter->setFont( QFont("FreeSans", nimiFonttiKoko_ ) );

    painter->drawImage( logoRect_, kitsas_->logo() );
    if(!nimi_.isEmpty())
        painter->drawText( nimiRect_, Qt::TextWordWrap, nimi_);

    painter->setFont( QFont("FreeSans", fonttikoko_ - 1) );
    painter->drawText( lahettajanOsoiteRect_, lahettajaOsoite_ );

    painter->setFont( QFont("FreeSans", fonttikoko_ ) );
    painter->drawText( vastaanottajaRect_, vastaanottaja_ );

    painter->restore();
}


QRect LaskunOsoiteAlue::kuorenIkkuna(QPagedPaintDevice *device) const
{
    const AsetusModel *asetukset = kitsas_->asetukset();
    if( !tulostettava_ || !asetukset->onko("LaskuIkkuna"))
        return QRect();

    const double mm = device->width() * 1.00 / device->widthMM();
    return QRect( (asetukset->luku("LaskuIkkunaX", 0) - device->pageLayout().margins(QPageLayout::Millimeter).left() ) * mm,
                   (asetukset->luku("LaskuIkkunaY",0) - device->pageLayout().margins(QPageLayout::Millimeter).top()) * mm,
                   asetukset->luku("LaskuIkkunaLeveys",90) * mm,
                  asetukset->luku("LaskuIkkunaKorkeus",30) * mm);
}

bool LaskunOsoiteAlue::isoIkkuna() const
{
    return kitsas_->asetukset()->luku(AsetusModel::LaskuIkkunaKorkeus) > 55;
}
