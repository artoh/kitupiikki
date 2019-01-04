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
#include "kuvaview.h"

#include <QPainter>
#include <QPrinter>
#include <QBuffer>

Naytin::KuvaView::KuvaView(const QImage &kuva) :
    kuva_(kuva)
{

}

QByteArray Naytin::KuvaView::data() const
{
    QByteArray ba;
    QBuffer buffer(&ba);

    buffer.open(QIODevice::WriteOnly);
    kuva_.save(&buffer,"JPG");
    buffer.close();

    return ba;
}

void Naytin::KuvaView::paivita() const
{
    scene()->clear();
    int leveys = kuva_.width() > width() ?  width() - 20 : kuva_.width();
    int korkeus = qRound( 1.00 * leveys / kuva_.width() * kuva_.height());
    QPixmap pixmap = QPixmap::fromImage( kuva_.scaled( qRound(leveys * zoomaus()), qRound(korkeus * zoomaus()), Qt::KeepAspectRatio ) );
    scene()->addPixmap(pixmap);
}

void Naytin::KuvaView::tulosta(QPrinter *printer) const
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
