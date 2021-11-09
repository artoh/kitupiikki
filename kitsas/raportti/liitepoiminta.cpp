/*
   Copyright (C) 2019 Arto Hyvättinen

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
#include "liitepoiminta.h"
#include "liitepoimija.h"

#include "db/kirjanpito.h"
#include "ui_paivakirja.h"

LiitePoiminta::LiitePoiminta() : PvmRaporttiWidget("liitepoiminta")
{
    ui->kumppaniCheck->hide();
    ui->tulostakohdennuksetCheck->hide();
    ui->tulostasummat->hide();

    ui->laatuLabel->show();
    ui->laatuSlider->show();

}

void LiitePoiminta::esikatsele()
{

    int kohdennuksella = -1;
    if( ui->kohdennusCheck->isChecked())
        kohdennuksella = ui->kohdennusCombo->kohdennus();
    int tililta = -1;
    if( ui->tiliBox->isChecked())
        tililta = ui->tiliCombo->currentData().toInt();

    LiitePoimija *poimija = new LiitePoimija(ui->kieliCombo->currentData().toString(), ui->laatuSlider->value());
    connect( poimija, &LiitePoimija::valmis, this, [this]  {this->odotaLabel->hide();});
    connect( poimija, &LiitePoimija::tyhja, this, [this] {this->nayta(RaportinKirjoittaja());} );
    poimija->poimi(ui->alkupvm->date(), ui->loppupvm->date(), tililta, kohdennuksella);

}
