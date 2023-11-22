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
#include "eraeranvalintadialog.h"
#include "eranvalintamodel.h"

#include "ui_eranvalintadialog.h"
#include "db/kirjanpito.h"
#include "db/tilikausimodel.h"

EraEranValintaDialog::EraEranValintaDialog(int tili, int asiakas, int nykyinen, QWidget *parent)
    : EranValintaDialog(parent),      
      model_(new EranValintaModel(this))
{
    model_->lataa(tili, asiakas);

    asetaModel(model_);
    ui->view->horizontalHeader()->setSectionResizeMode(EranValintaModel::SELITE, QHeaderView::Stretch);

    connect( ui->avoimetCheck, &QCheckBox::toggled,
             this, &EraEranValintaDialog::paivita);


    asetaNykyinen(nykyinen);
}


void EraEranValintaDialog::paivita()
{
    model_->paivita( ui->avoimetCheck->isChecked() );
}


