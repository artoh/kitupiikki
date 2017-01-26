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

#include <QDateEdit>

#include "paivakirjaraportti.h"

#include "db/kirjanpito.h"
#include "db/tilikausi.h"

PaivakirjaRaportti::PaivakirjaRaportti()
{

}

void PaivakirjaRaportti::alustaLomake(QWidget *kehys)
{
    QDateEdit *alkaaEdit = kehys->findChild<QDateEdit*>("alkupvm");
    QDateEdit *loppuuEdit = kehys->findChild<QDateEdit*>("loppupvm");

    Tilikausi nykykausi = Kirjanpito::db()->tilikausiPaivalle( Kirjanpito::db()->paivamaara() );
    alkaaEdit->setDate(nykykausi.alkaa());
    loppuuEdit->setDate(nykykausi.paattyy());
}

void PaivakirjaRaportti::tulosta(QPrinter *printer, QWidget *kehys)
{
    QPainter painter(printer);
    painter.drawText(100,100,"Kokeilua ja testiä");
}
