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

#include <QDate>
#include <QFont>
#include <QPen>

#include "raportti.h"
#include "db/kirjanpito.h"


Raportti::Raportti(QWidget *parent) : QWidget(parent)
{

}

QIcon Raportti::kuvake() const
{
    return QIcon();
}

void Raportti::alustaLomake()
{

}

void Raportti::tulosta(QPrinter */*printer*/)
{

}

void Raportti::tulostaYlatunniste(QPainter *painter, int sivu, const QString &otsikko, const QString &alaotsikko)
{

    painter->setFont(QFont("Sans",10));

    int sivunleveys = painter->window().width();
    int sivunkorkeus = painter->window().height();
    int rivinkorkeus = painter->fontMetrics().height();

    QString nimi = Kirjanpito::db()->asetus("nimi");
    QString paivays = QDate::currentDate().toString(Qt::SystemLocaleShortDate);


    painter->drawText( QRect(0,0,sivunleveys/4, rivinkorkeus ), Qt::AlignLeft, nimi );
    painter->drawText( QRect(sivunleveys/4,0,sivunleveys/2, rivinkorkeus  ), Qt::AlignHCenter, otsikko);
    painter->drawText( QRect(sivunleveys*3/4, 0, sivunleveys/4, rivinkorkeus), Qt::AlignRight, paivays);

    painter->translate(0, rivinkorkeus);

    QString ytunnus = Kirjanpito::db()->asetus("ytunnus") ;
    QString sivustr = tr("Sivu %1").arg(sivu);

    painter->drawText(QRect(0,0,sivunleveys/4, rivinkorkeus ), Qt::AlignLeft, ytunnus );
    painter->drawText(QRect(sivunleveys/4,0,sivunleveys/2, rivinkorkeus  ), Qt::AlignHCenter, alaotsikko);
    painter->drawText(QRect(sivunleveys*3/4, 0, sivunleveys/4, rivinkorkeus), Qt::AlignRight, sivustr);

    painter->translate(0, rivinkorkeus );

    painter->setPen(QPen(QBrush(Qt::black),1.00));

}
