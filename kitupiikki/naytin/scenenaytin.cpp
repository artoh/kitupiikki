/*
 *
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
#include "scenenaytin.h"
#include "abstraktiview.h"

#include <QGraphicsScene>
#include <QGraphicsView>

#include <QPixmap>
#include <QBuffer>
#include <QPainter>
#include <QPrinter>


Naytin::SceneNaytin::SceneNaytin(AbstraktiView *view, QObject *parent)
    : AbstraktiNaytin (parent),
      view_(view)
{

}

QWidget *Naytin::SceneNaytin::widget()
{
    return view_;
}

QByteArray Naytin::SceneNaytin::data() const
{
    return view_->data();
}

QString Naytin::SceneNaytin::tiedostonMuoto() const
{
    return view_->tiedostonMuoto();
}

QString Naytin::SceneNaytin::tiedostonPaate() const
{
    return view_->tiedostonPaate();
}

QString Naytin::SceneNaytin::otsikko() const
{
    return view_->otsikko();
}

void Naytin::SceneNaytin::paivita() const
{
    view_->paivita();
}

void Naytin::SceneNaytin::tulosta(QPrinter *printer) const
{
    view_->tulosta(printer);
}

void Naytin::SceneNaytin::zoomOut()
{
    view_->zoomOut();
}

void Naytin::SceneNaytin::zoomFit()
{
    view_->zoomFit();
}

void Naytin::SceneNaytin::zoomIn()
{
    view_->zoomIn();
}

