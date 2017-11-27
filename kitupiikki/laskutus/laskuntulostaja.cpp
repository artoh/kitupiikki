/*
   Copyright (C) 2017 Arto Hyvättinen

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
#include "db/kirjanpito.h"

#include <QDebug>
#include <cmath>

LaskunTulostaja::LaskunTulostaja(LaskuModel *model) : QObject(model), model_(model)
{

}

bool LaskunTulostaja::tulosta(QPrinter *printer)
{


    QPainter painter(printer);


    double mm = printer->width() * 1.00 / printer->widthMM();


    painter.setFont(QFont("Sans", 12));

    /*
    if( QFile::exists(kp()->hakemisto().absoluteFilePath("logo128.png")))
    {
        painter.drawPixmap( QRectF(0, 0, mm * 20, mm * 20), QPixmap( kp()->hakemisto().absoluteFilePath("logo128.png") ),
                             QRect(0,0,128,128));
    }

    painter.drawText( QRectF(mm * 32, 0, mm * 100, mm * 20), Qt::AlignLeft, kp()->asetukset()->asetus("Nimi"));
    painter.drawText( QRectF(mm * 190, 0, mm * 100, mm * 20), Qt::AlignCenter, tr("Otsikko"));

    painter.setPen(QPen(QBrush(Qt::black), mm * 0.8 ));
    painter.drawLine(QLineF(0,0,mm*203,mm*293));

    */
    tilisiirto(printer, &painter);

    painter.end();

    return true;
}

void LaskunTulostaja::tilisiirto(QPrinter *printer, QPainter *painter)
{
    painter->setFont(QFont("Sans", 7));
    double mm = printer->width() * 1.00 / printer->widthMM();

    painter->drawText( QRectF(0,0,mm*19,mm*16.9), Qt::AlignRight | Qt::AlignHCenter, tr("Saajan\n tilinumero\n Mottagarens\n kontonummer"));
    painter->drawText( QRectF(0, mm*18, mm*19, mm*14.8), Qt::AlignRight | Qt::AlignHCenter, tr("Saaja\n Mottagare"));
    painter->drawText( QRectF(0, mm*32.7, mm*19, mm*20), Qt::AlignRight | Qt::AlignTop, tr("Maksajan\n nimi ja\n osoite\n Betalarens\n namn och\n address"));
    painter->drawText( QRectF(0, mm*51.3, mm*19, mm*10), Qt::AlignRight | Qt::AlignBottom , tr("Allekirjoitus\n Underskrift"));
    painter->drawText( QRectF(0, mm*62.3, mm*19, mm*8.5), Qt::AlignRight | Qt::AlignHCenter, tr("Tililtä nro\n Från konto nr"));
    painter->drawText( QRectF(mm * 22, 0, mm*20, mm*10), Qt::AlignLeft, tr("IBAN"));

    painter->drawText( QRectF(mm*112.4, mm*53.8, mm*15, mm*8.5), Qt::AlignLeft | Qt::AlignTop, tr("Viitenumero\nRef.nr."));
    painter->drawText( QRectF(mm*112.4, mm*62.3, mm*15, mm*8.5), Qt::AlignLeft | Qt::AlignTop, tr("Eräpäivä\nFörfallodag"));
    painter->drawText( QRectF(mm*159, mm*62.3, mm*19, mm*8.5), Qt::AlignLeft, tr("Euro"));

    painter->setPen( QPen( QBrush(Qt::black), mm * 0.5));
    painter->drawLine(QLineF(mm*111.4,0,mm*111.4,mm*69.8));
    painter->drawLine(QLineF(0, mm*16.9, mm*111.4, mm*16.9));
    painter->drawLine(QLineF(0, mm*31.7, mm*111.4, mm*31.7));
    painter->drawLine(QLineF(mm*20, 0, mm*20, mm*31.7));
    painter->drawLine(QLineF(0, mm*61.3, mm*205, mm*61.3));
    painter->drawLine(QLineF(0, mm*69.8, mm*205, mm*69.8));
    painter->drawLine(QLineF(mm*111.4, mm*52.8, mm*205, mm*52.8));
    painter->drawLine(QLineF(mm*131.4, mm*52.8, mm*131.4, mm*69.8));
    painter->drawLine(QLineF(mm*158, mm*61.3, mm*158, mm*69.8));

    painter->setPen( QPen(QBrush(Qt::black), mm * 0.13));
    painter->drawLine( QLineF( mm*22, mm*57.1, mm*108, mm*57.1));

    painter->setFont(QFont("Sans", 10));
    painter->drawText( QRectF(mm*165, mm*62.3, mm*30, mm*8.5), Qt::AlignRight | Qt::AlignHCenter, QString("%L1").arg( std::round(model_->laskunSumma() / 100) ,0,'f',2) );
}

