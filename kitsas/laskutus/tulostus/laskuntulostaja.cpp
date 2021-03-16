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
#include "laskuntulostaja.h"

#include <QPainter>
#include <QPagedPaintDevice>

#include <QPdfWriter>
#include <QBuffer>

#include <QApplication>

#include "db/kitsasinterface.h"
#include "laskuntietolaatikko.h"
#include "laskunosoitealue.h"
#include "laskunalaosa.h"


LaskunTulostaja::LaskunTulostaja(KitsasInterface *kitsas, QObject *parent)
    : QObject(parent), kitsas_(kitsas)
{

}

void LaskunTulostaja::tulosta(const Tosite &tosite, QPagedPaintDevice *printer, QPainter *painter)
{
    const Lasku& lasku = tosite.constLasku();

    LaskunOsoiteAlue osoiteosa( kitsas_ );
    LaskunTietoLaatikko laatikko( kitsas_ );
    LaskunAlaosa alaosa( kitsas_ );

    if( tosite.laskuNumero().isEmpty())
        tulostaLuonnos(painter, lasku.kieli().toLower());

    osoiteosa.lataa(tosite);
    laatikko.lataa(tosite);
    alaosa.lataa(lasku, osoiteosa.vastaanottaja());

    qreal alaosanKorkeus = alaosa.laske(painter);
    alaosa.piirra(painter, lasku);

    osoiteosa.laske(painter, printer);

    qreal sivunleveys = painter->window().width();
    qreal laatikkoleveys = osoiteosa.leveys() > sivunleveys
            ? sivunleveys - osoiteosa.leveys()
            : sivunleveys / 2;
    laatikko.laskeLaatikko(painter, laatikkoleveys);

    painter->save();
    osoiteosa.piirra(painter);
    painter->translate(sivunleveys - laatikkoleveys, 0);
    laatikko.piirra(painter);
    painter->restore();

    painter->translate(0, osoiteosa.korkeus() > laatikko.korkeus() ?
                          osoiteosa.korkeus() : laatikko.korkeus());

    painter->setFont(QFont("FreeSans", 10));
    painter->translate( 0, painter->fontMetrics().height() * 0.5 );

    if( !lasku.lisatiedot().isEmpty() ) {
        QRectF lisaRect = painter->boundingRect( QRectF(0,0,sivunleveys,sivunleveys),
                                                 Qt::TextWordWrap,
                                                 lasku.lisatiedot());
        painter->drawText( lisaRect, Qt::TextWordWrap, lasku.lisatiedot() );
        painter->translate(0, lisaRect.height());
        painter->translate( 0, painter->fontMetrics().height() * 0.5 );
    }

    // Sitten pitäisi alkaa tulostaa riveja niin paljon kun mahtuu


}

QByteArray LaskunTulostaja::pdf(const Tosite &tosite)
{
    QByteArray array;
    QBuffer buffer(&array);
    buffer.open( QIODevice::WriteOnly);

    QPdfWriter writer(&buffer);
    writer.setPdfVersion(QPagedPaintDevice::PdfVersion_A1b);
    writer.setPageSize( QPdfWriter::A4);
    writer.setPageMargins( QMarginsF(10,10,10,10), QPageLayout::Millimeter );
    QPainter painter(&writer);

    writer.setCreator(QString("%1 %2").arg( qApp->applicationName() ).arg( qApp->applicationVersion() ));
    writer.setTitle( tr("Lasku %1").arg( tosite.constLasku().numero() ) );
    tulosta(tosite , &writer, &painter);
    painter.end();

    buffer.close();

    return array;
}

void LaskunTulostaja::tulostaLuonnos(QPainter *painter, const QString& kieli)
{
    painter->save();
    painter->setPen( QPen( Qt::lightGray));
    painter->setFont( QFont("FreeSans",60,QFont::Black));
    painter->drawText(QRect( 0, 0, painter->window().width(), painter->window().height() ), Qt::AlignCenter,
                      kitsas_->kaanna("luonnos", kieli) );
    painter->restore();
}
