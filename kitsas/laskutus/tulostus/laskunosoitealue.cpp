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
#include "model/toiminimimodel.h"

#include <QVariantMap>
#include <QPainter>
#include <QPaintDevice>
#include <QPagedPaintDevice>

LaskunOsoiteAlue::LaskunOsoiteAlue(KitsasInterface *kitsas) :
    kitsas_(kitsas)
{


}

void LaskunOsoiteAlue::lataa(const Tosite &tosite)
{
    const Lasku lasku = tosite.constLasku();
    const int toiminimiIndeksi = lasku.toiminimi();
    const ToiminimiModel* toiminimet = kitsas_->toiminimet();
    const QString sijaintiStr = toiminimet->tieto(ToiminimiModel::LogonSijainti, toiminimiIndeksi);
    if( toiminimet->logo(toiminimiIndeksi).isNull()) {
        logoSijainti_ = EILOGOA;
    } else if( sijaintiStr == "VAINLOGO") {
        logoSijainti_ = VAINLOGO;
    } else if( sijaintiStr == "YLLA") {
        logoSijainti_ = YLLA;
    } else {
        logoSijainti_ = VIERESSA;
    }

    if( logoSijainti_ != VAINLOGO) {
        nimi_ = toiminimet->tieto(ToiminimiModel::Nimi, toiminimiIndeksi);
    }

    lahettajaOsoite_ = isoIkkuna() ?
        toiminimet->tieto(ToiminimiModel::Katuosoite, toiminimiIndeksi) + "\n" +
        toiminimet->tieto(ToiminimiModel::Postinumero) + " " + toiminimet->tieto(ToiminimiModel::Kaupunki)
        :   QString();


    const QVariantMap& kumppani = tosite.data(Tosite::KUMPPANI).toMap();
    if( kumppani.value("osoite").toString().isEmpty()) {
        const Lasku& lasku = tosite.constLasku();
        vastaanottaja_ =  lasku.osoite().isEmpty() ? kumppani.value("nimi").toString() : lasku.osoite();
    } else {
        vastaanottaja_ = MaaModel::instanssi()->muotoiltuOsoite(kumppani);
    }

    tulostettava_ = lasku.lahetystapa() == Lasku::TULOSTETTAVA ||
                    lasku.lahetystapa() == Lasku::POSTITUS ||
                    lasku.lahetystapa() == Lasku::PDF;

    logo_ = toiminimet->logo(toiminimiIndeksi);
    logonKorkeus_ = toiminimet->tieto(ToiminimiModel::LogonKorkeus, toiminimiIndeksi, "20").toInt();
}

qreal LaskunOsoiteAlue::laske(QPainter *painter, QPagedPaintDevice *device)
{
    const double mm = device->width() * 1.00 / device->widthMM();
    painter->setFont( QFont("FreeSans",fonttikoko_) );

    int sivunLeveys = painter->window().width();

    const QRectF ikkuna = kuorenIkkuna(device);    

    QRectF lahettajaAlue =
            ikkuna.isNull() ? QRectF(0, 0 , sivunLeveys / 2 - 10 * mm, 35 * mm ) :
            ( isoIkkuna() ? QRect( ikkuna.x(), ikkuna.y(), ikkuna.width(), 25 * mm )
                        : QRect( 0, 0, sivunLeveys / 2, ikkuna.y() - 10 * mm  ));

    nimiFonttiKoko_ = isoIkkuna() ? fonttikoko_ + 1 :
                                    ( logo_.isNull() ? fonttikoko_ + 4 : fonttikoko_ + 2);

    painter->setFont( QFont("FreeSans", nimiFonttiKoko_));

    nimiRect_ = painter->boundingRect( lahettajaAlue, Qt::TextWordWrap, nimi_ );

    painter->setFont( QFont("FreeSans", fonttikoko_ - 1) );
    lahettajanOsoiteRect_ = painter->boundingRect(lahettajaAlue, lahettajaOsoite_ );
    qreal logoMaxKorkeus = mm *  kitsas_->asetukset()->luku("LaskuLogoKorkeus",20) ;

    if( logoSijainti_ == EILOGOA) {
        // Ei logoa
        nimiRect_.moveTo( lahettajaAlue.x(), lahettajaAlue.y() );
        lahettajanOsoiteRect_.moveTo( lahettajaAlue.x(), lahettajaAlue.y() + nimiRect_.height() );
    } else if( logoSijainti_ == VAINLOGO) {
        qreal lkorkeus = lahettajaAlue.height() - lahettajanOsoiteRect_.height();
        QSize size = logo_.scaled(lahettajaAlue.width(),
                                 qMin(logoMaxKorkeus, lkorkeus),
                                 Qt::KeepAspectRatio ).size();
        logoRect_ = QRectF( lahettajaAlue.x(), lahettajaAlue.y(),
                            size.width(), size.height());
        lahettajanOsoiteRect_.moveTo(lahettajaAlue.x(), lahettajaAlue.y() + logoRect_.height());
    } else if( logoSijainti_ == YLLA) {
        qreal lkorkeus = lahettajaAlue.height() - nimiRect_.height() - lahettajanOsoiteRect_.height();
        QSize size = logo_.scaled(lahettajaAlue.width(),
                                 qMin(logoMaxKorkeus, lkorkeus),
                                 Qt::KeepAspectRatio).size();
        logoRect_ = QRectF( lahettajaAlue.x(), lahettajaAlue.y(),
                            size.width(), size.height());
        nimiRect_.moveTo(lahettajaAlue.x(), lahettajaAlue.y() +  logoRect_.height());
        lahettajanOsoiteRect_.moveTo(lahettajaAlue.x(), lahettajaAlue.y() +  logoRect_.height() + nimiRect_.height());
    } else  {   // Logo on vieressä
        qreal leveampi = nimiRect_.width() > lahettajanOsoiteRect_.width()
                        ? nimiRect_.width()
                        : lahettajanOsoiteRect_.width();
        qreal jaljella = lahettajaAlue.width() - leveampi;
        qreal tekstienKorkeus = nimiRect_.height() + lahettajanOsoiteRect_.height();

        QSize size = logo_.scaled(jaljella,
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

    painter->setFont( QFont("FreeSans", nimiFonttiKoko_ ) );

    painter->drawImage( logoRect_, logo_ );
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
