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
#include "kuvanaytin.h"
#include "kuvaview.h"

#include <QGraphicsScene>
#include <QGraphicsView>

#include <QPixmap>
#include <QBuffer>
#include <QPainter>
#include <QPrinter>

Naytin::KuvaNaytin::KuvaNaytin(const QImage &kuva, QObject *parent)
    : AbstraktiNaytin (parent),
      view_{ new KuvaView(kuva)}
{

}

QWidget *Naytin::KuvaNaytin::widget()
{
    return view_;
}

QByteArray Naytin::KuvaNaytin::data() const
{
    QByteArray ba;
    QBuffer buffer(&ba);

    buffer.open(QIODevice::WriteOnly);
    view_->kuva().save(&buffer,"JPG");
    buffer.close();

    return ba;
}

void Naytin::KuvaNaytin::paivita() const
{
    view_->paivita();
}

void Naytin::KuvaNaytin::tulosta(QPrinter *printer) const
{
    QPainter painter( printer );
    QRect rect = painter.viewport();
    QImage kuva = view_->kuva();
    QSize size = kuva.size();
    size.scale(rect.size(), Qt::KeepAspectRatio);
    painter.setViewport( rect.x(), rect.y(),
                         size.width(), size.height());
    painter.setWindow(kuva.rect());
    painter.drawImage(0, 0, kuva);
}

void Naytin::KuvaNaytin::zoomOut()
{
    view_->zoomOut();
}

void Naytin::KuvaNaytin::zoomFit()
{
    view_->zoomFit();
}

void Naytin::KuvaNaytin::zoomIn()
{
    view_->zoomIn();
    paivita();
}

