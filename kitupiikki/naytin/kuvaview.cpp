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


Naytin::KuvaView::KuvaView(const QImage &kuva) :
    QGraphicsView (new QGraphicsScene()),
    kuva_( kuva )
{
    setDragMode(QGraphicsView::ScrollHandDrag);
    setBackgroundBrush(QBrush(Qt::darkGray));
}

QImage Naytin::KuvaView::kuva() const
{
    return kuva_;
}

void Naytin::KuvaView::paivita()
{
    scene()->clear();
    int leveys = kuva_.width() > width() ?  width() - 20 : kuva_.width();
    int korkeus = qRound( 1.00 * leveys / kuva_.width() * kuva_.height());
    QPixmap pixmap = QPixmap::fromImage( kuva_.scaled( qRound(leveys * zoomaus_), qRound(korkeus * zoomaus_), Qt::KeepAspectRatio ) );
    scene()->addPixmap(pixmap);
}

void Naytin::KuvaView::zoomOut()
{
    zoomaus_ *= 0.7;
    paivita();
}

void Naytin::KuvaView::zoomFit()
{
    zoomaus_ = 1.0;
    paivita();
}

void Naytin::KuvaView::resizeEvent(QResizeEvent * /* event */)
{
    paivita();
}

void Naytin::KuvaView::zoomIn()
{
    zoomaus_ *= 1.2;
    paivita();
}
