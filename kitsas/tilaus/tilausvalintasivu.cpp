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
#include "tilausvalintasivu.h"
#include "ui_tilausvalinta.h"

#include "planmodel.h"

#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"

#include <QHeaderView>

TilausValintaSivu::TilausValintaSivu(PlanModel *model) :
    ui( new Ui::TilausValinta)
{
    ui->setupUi(this);
    ui->planView->setModel(model);

    connect( ui->kuukausiRadio, &QRadioButton::toggled, model, &PlanModel::naytaKuukausittain );
    connect( ui->kuukausiRadio, &QRadioButton::toggled, this, &TilausValintaSivu::completeChanged);
    connect( ui->planView->selectionModel(), &QItemSelectionModel::currentRowChanged,
             this, &TilausValintaSivu::completeChanged);

    registerField("kuukausittain", ui->kuukausiRadio);

    setTitle("Valitse tilaus");
}

void TilausValintaSivu::alusta(int nykyinen, bool kuukausittain, double palautus)
{
    alkuperainenPlan_ = nykyinen;
    alunperinKuukaudet_ = kuukausittain;

    ui->kuukausiRadio->setChecked( kuukausittain );
    ui->vuosiRadio->setChecked( !kuukausittain );

    PlanModel *model = qobject_cast<PlanModel*>( ui->planView->model());

    ui->planView->selectRow( model->rowForPlan(nykyinen) );

    QString info;

    if( palautus > 1.00)
        info.append(tr("Vaihtaessasi isompaan tilaukseen hyvitetään ensimmäiseltä "
                       "laskulta %L1 € nykyisestä tilauksestasi.\n\n")
                    .arg(palautus,0,'f',2));

    if( kp()->pilvi()->omatPilvet() > 1)
        info.append( tr("Koska sinulla on jo pilveen tallennettuna %1 kirjanpitoa et voi vaihtaa "
                        "tätä pienempään tilaukseen ilman, että poistat ensin "
                        "osan kirjanpidoistasi")
                     .arg( kp()->pilvi()->omatPilvet()));

    ui->infoLabel->setText(info);

}

QVariant TilausValintaSivu::tilaus(int rooli) const
{
    return ui->planView->currentIndex().data(rooli);
}


bool TilausValintaSivu::isComplete() const
{
    return tilaus(PlanModel::PlanRooli).toInt() != alkuperainenPlan_ ||
            ui->kuukausiRadio->isChecked() != alunperinKuukaudet_;
}

void TilausValintaSivu::initializePage()
{
    ui->planView->horizontalHeader()->setSectionResizeMode( PlanModel::NIMI,
                                                            QHeaderView::Stretch);
}
