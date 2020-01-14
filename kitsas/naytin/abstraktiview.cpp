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
#include "abstraktiview.h"


Naytin::AbstraktiView::AbstraktiView() :
    QGraphicsView (new QGraphicsScene() )
{
    setDragMode(QGraphicsView::ScrollHandDrag);
    setBackgroundBrush(QBrush(Qt::darkGray));
}

Naytin::AbstraktiView::~AbstraktiView()
{

}

void Naytin::AbstraktiView::zoomOut()
{
    zoomaus_ *= 0.7;
    paivita();
}

void Naytin::AbstraktiView::zoomFit()
{
    zoomaus_ = 1.0;
    paivita();
}

void Naytin::AbstraktiView::resizeEvent(QResizeEvent * /* event */)
{
    paivita();
}

void Naytin::AbstraktiView::zoomIn()
{
    zoomaus_ *= 1.2;
    paivita();
}
