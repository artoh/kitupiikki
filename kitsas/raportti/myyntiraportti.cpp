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

#include "myyntiraportti.h"
#include "myyntiraportteri.h"

#include "ui_pvmvali.h"
#include "db/kirjanpito.h"


MyyntiRaportti::MyyntiRaportti()
{
    ui = new Ui::PvmVali;
    ui->setupUi( raporttiWidget );

    Tilikausi kausi = kp()->tilikausiPaivalle( kp()->paivamaara() );
    if( !kausi.alkaa().isValid())
        kausi = kp()->tilikaudet()->tilikausiIndeksilla(kp()->tilikaudet()->rowCount()-1);

    ui->alkaa->setDate(kausi.alkaa());
    ui->paattyy->setDate(kausi.paattyy());

}

MyyntiRaportti::~MyyntiRaportti()
{
    delete ui;
}

void MyyntiRaportti::esikatsele()
{
    MyyntiRaportteri *myynti = new MyyntiRaportteri(this, ui->kieliCombo->currentData().toString());
    connect( myynti, &MyyntiRaportteri::valmis, this, &RaporttiWidget::nayta);
    myynti->kirjoita(ui->alkaa->date(), ui->paattyy->date());
}

