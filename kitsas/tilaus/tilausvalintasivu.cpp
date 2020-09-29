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
    connect( ui->planView->selectionModel(), &QItemSelectionModel::currentRowChanged,
             this, &TilausValintaSivu::paivita);
    connect( ui->lisaSpin, SIGNAL(valueChanged(int)), this, SLOT(paivita()));
    connect( ui->kuukausiRadio, &QRadioButton::toggled, this, &TilausValintaSivu::paivita);

    registerField("puolivuosittain", ui->kuukausiRadio);
    registerField("lisapilvet", ui->lisaSpin);

    setTitle("Valitse tilaus");
    ui->planView->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

}

void TilausValintaSivu::alusta(int nykyinen, bool puolivuosittain, double palautus, double vakiokoko, int lisapilvet)
{
    alkuperainenPlan_ = nykyinen;
    alunperinPuolivuodet_ = puolivuosittain;
    alunperinLisapilvet_ = lisapilvet;

    ui->kuukausiRadio->setChecked( puolivuosittain );
    ui->vuosiRadio->setChecked( !puolivuosittain );
    ui->lisaSpin->setValue(lisapilvet);

    PlanModel *model = qobject_cast<PlanModel*>( ui->planView->model());

    ui->planView->selectRow( model->rowForPlan(nykyinen) );
    palautus_ = palautus;
    vakiokoko_ = vakiokoko;

    paivita();
}

void TilausValintaSivu::paivita()
{
    int pilvia = ui->planView->currentIndex().data(PlanModel::PilviaRooli).toInt();
    int pilviayht = pilvia + ui->lisaSpin->value();
    double pilvihinta = ui->planView->currentIndex().data(PlanModel::LisaPilviHinta).toDouble();

    QString info;
    int planId = ui->planView->currentIndex().data(PlanModel::PlanRooli).toInt();

    ui->lisahintaInfo->show();

    if( planId == 0) {
        ui->tilaInfo->setText(tr("Tilataksesi pilvitilaa useammalle kirjanpidolle valitse toinen paketti"));
        ui->lisaksiLabel->hide();
        ui->lisaSpin->hide();
        ui->kirjanpidolleLabel->hide();
        ui->lisahinta->hide();
        ui->lisahintaInfo->hide();
    } else if (planId == PlanModel::TILITOIMISTOPLAN) {
        double kkhinta = pilvihinta / 12;
        ui->tilaInfo->setText( tr("Pakettihintaan kuuluu %1 kirjanpidon tallentaminen pilveen.\n"
                                  "Lisäkirjanpidoista laskutetaan jälkikäteen %2 € / kuukausi")
                               .arg(pilvia).arg(kkhinta,0,'f',2));
        ui->lisaksiLabel->hide();
        ui->lisaSpin->hide();
        ui->kirjanpidolleLabel->hide();
        ui->lisahinta->hide();
    } else {

        ui->tilaInfo->setText( pilvia == 1 ? tr("Pakettihintaan kuuluu yhden kirjanpidon tallentaminen pilveen") :
                                             tr("Pakettihintaan kuuluu %1 kirjanpidon tallentaminen pilveen").arg(pilvia));
        ui->lisaSpin->setEnabled(true);
        ui->lisaksiLabel->show();
        ui->lisaSpin->show();
        ui->kirjanpidolleLabel->show();
        ui->lisahinta->show();


        ui->lisahinta->setText( tr("%L1 € / kpl").arg( (ui->kuukausiRadio->isChecked() ?
                                                      pilvihinta / 2 : pilvihinta),0,'f',2));

    }
    ui->lisahintaInfo->setText(tr("Lisähintaa voidaan periä, jos kirjanpitojen yhteiskoko ylittää %L1 Gt.").arg( vakiokoko_ * pilviayht,0,'f',1 ));


    if( palautus_ > 1.00)
        info.append(tr("Vaihtaessasi isompaan tilaukseen hyvitetään ensimmäiseltä "
                       "laskulta %L1 € nykyisestä tilauksestasi.")
                    .arg(palautus_,0,'f',2));

    if( kp()->pilvi()->omatPilvet() > pilviayht)
        info.append( tr("Koska sinulla on jo pilveen tallennettuna %1 kirjanpitoa et voi vaihtaa "
                        "tätä pienempään tilaukseen ilman, että poistat ensin "
                        "osan kirjanpidoistasi")
                     .arg( kp()->pilvi()->omatPilvet()));

    ui->infoLabel->setText(info);
    completeChanged();
}

QVariant TilausValintaSivu::tilaus(int rooli) const
{
    return ui->planView->currentIndex().data(rooli);
}


bool TilausValintaSivu::isComplete() const
{
    return (tilaus(PlanModel::PlanRooli).toInt() != alkuperainenPlan_ ||
            ui->kuukausiRadio->isChecked() != alunperinPuolivuodet_ ||
            ui->lisaSpin->value() != alunperinLisapilvet_ ) &&
            ui->planView->currentIndex().data(PlanModel::PilviaRooli).toInt() +
                ui->lisaSpin->value() >= kp()->pilvi()->omatPilvet();

}

void TilausValintaSivu::initializePage()
{
    ui->planView->horizontalHeader()->setSectionResizeMode( PlanModel::NIMI,
                                                            QHeaderView::Stretch);
}
