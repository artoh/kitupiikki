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
#include "maksutapasivu.h"

#include "ui_maksutapasivu.h"
#include "maksutapamuokkaus.h"
#include "model/maksutapamodel.h"
#include "db/kirjanpito.h"

MaksutapaSivu::MaksutapaSivu() :
    ui(new Ui::MaksutapaSivu)
{
    ui->setupUi(this);

    menoModel_ = new MaksutapaModel(this);
    tuloModel_ = new MaksutapaModel(this);

    ui->menoView->setModel(menoModel_);
    ui->tuloView->setModel(tuloModel_);

    QString lisatiedot = kp()->asetukset()->asetus(AsetusModel::LaskuLisatiedot);
    if( lisatiedot == "KAIKKI")
        ui->kaikilleRadio->setChecked(true);
    else if(lisatiedot == "EI")
        ui->eiRadio->setChecked(true);
    else
        ui->laskuilleRadio->setChecked(true);


    connect( ui->kaikilleRadio, &QRadioButton::clicked, this, &MaksutapaSivu::ltmuuttuu);
    connect( ui->laskuilleRadio, &QRadioButton::clicked, this, &MaksutapaSivu::ltmuuttuu);
    connect( ui->eiRadio, &QRadioButton::clicked, this, &MaksutapaSivu::ltmuuttuu);
}

bool MaksutapaSivu::nollaa()
{
    menoModel_->lataa(MaksutapaModel::MENO);
    tuloModel_->lataa(MaksutapaModel::TULO);
    return true;
}

void MaksutapaSivu::ltmuuttuu()
{
    if( ui->eiRadio->isChecked())
        kp()->asetukset()->aseta(AsetusModel::LaskuLisatiedot, "EI");
    else if( ui->kaikilleRadio->isChecked())
        kp()->asetukset()->aseta(AsetusModel::LaskuLisatiedot, "KAIKKI");
    else
        kp()->asetukset()->poista(AsetusModel::LaskuLisatiedot);
}
