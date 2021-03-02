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

#include "taseerittely.h"
#include "taseerittelija.h"
#include <QSqlQuery>

#include <QDebug>

TaseErittely::TaseErittely() :
    RaporttiWidget(nullptr)
{
    ui = new Ui::PvmVali;
    ui->setupUi( raporttiWidget);

    int tilikausia = kp()->tilikaudet()->rowCount(QModelIndex()) ;
    Tilikausi kausi = tilikausia > 2 ? kp()->tilikaudet()->tilikausiIndeksilla(tilikausia - 2) : kp()->tilikaudet()->tilikausiIndeksilla(tilikausia - 1);

    ui->alkaa->setDate( kausi.alkaa());
    ui->paattyy->setDate( kausi.paattyy());

}

TaseErittely::~TaseErittely()
{
    delete ui;
}

void TaseErittely::esikatsele()
{
    TaseErittelija* erittelija = new TaseErittelija(this, ui->kieliCombo->currentData().toString());
    connect( erittelija, &TaseErittelija::valmis, this, &TaseErittely::nayta);
    erittelija->kirjoita( ui->alkaa->date(), ui->paattyy->date() );
}

