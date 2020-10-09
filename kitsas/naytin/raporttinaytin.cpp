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
#include "raporttinaytin.h"

#include <QPainter>
#include <QPrinter>
#include <QDebug>

Naytin::RaporttiNaytin::RaporttiNaytin(RaportinKirjoittaja raportti, QWidget *parent)
    : PrintPreviewNaytin (parent),
      raportti_(raportti)
{    
    qDebug() << "RaporttiNaytin";
}

Naytin::RaporttiNaytin::~RaporttiNaytin()
{
    qDebug() << "~RaporttiNaytin";
}

QString Naytin::RaporttiNaytin::otsikko() const
{
    return raportti_.otsikko();
}

bool Naytin::RaporttiNaytin::csvMuoto() const
{
    return raportti_.csvKaytossa();
}

QByteArray Naytin::RaporttiNaytin::csv() const
{
    return raportti_.csv();
}

QByteArray Naytin::RaporttiNaytin::data() const
{
    QPageLayout leiska( printer_->pageLayout() );
    leiska.setOrientation(suunta_);
    return raportti_.pdf( onkoRaidat(), false, &leiska, suunta_ );
}

QString Naytin::RaporttiNaytin::html() const
{
    return raportti_.html();
}


void Naytin::RaporttiNaytin::tulosta(QPrinter *printer) const
{
    qDebug() << "Tulosta " << this << " Raportti " << raportti_.otsikko();
    QPainter painter(printer);
    raportti_.tulosta(printer, &painter, onkoRaidat());
}
