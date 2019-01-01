/*
   Copyright (C) 2018 Arto Hyvättinen

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
#include "vanhakuvanaytin.h"

#include <QGraphicsPixmapItem>
#include <QPixmap>
#include <QByteArray>
#include <QBuffer>
#include <QPainter>
#include <QPrinter>

VanhaKuvaNaytin::VanhaKuvaNaytin(QObject *parent) :
    NaytinScene (parent)
{

}

VanhaKuvaNaytin::VanhaKuvaNaytin(const QByteArray &kuvadata, QObject *parent)
    : VanhaKuvaNaytin( parent )
{
    naytaKuva( kuvadata );
}

bool VanhaKuvaNaytin::naytaKuva(const QByteArray &kuvadata)
{
    kuva_.loadFromData(kuvadata);
    return !kuva_.isNull();
}

QString VanhaKuvaNaytin::tyyppi() const
{
    if( kuva_.isNull())
        return  QString();
    return "img";
}

void VanhaKuvaNaytin::piirraLeveyteen(double leveyteen)
{
    clear();
    double korkeus = leveyteen / kuva_.width() * kuva_.height();
    QPixmap pixmap = QPixmap::fromImage( kuva_.scaled( qRound( leveyteen ),  qRound(korkeus), Qt::KeepAspectRatio) );
    addPixmap( pixmap );
}

QByteArray VanhaKuvaNaytin::data()
{
    QByteArray ba;
    QBuffer buffer(&ba);

    buffer.open(QIODevice::WriteOnly);
    kuva_.save(&buffer,"JPG");
    buffer.close();

    return ba;
}

void VanhaKuvaNaytin::tulosta(QPrinter *printer)
{
    QPainter painter( printer );
    QRect rect = painter.viewport();
    QSize size = kuva_.size();
    size.scale(rect.size(), Qt::KeepAspectRatio);
    painter.setViewport( rect.x(), rect.y(),
                         size.width(), size.height());
    painter.setWindow(kuva_.rect());
    painter.drawImage(0, 0, kuva_);
}


