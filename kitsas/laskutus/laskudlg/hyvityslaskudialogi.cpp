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
#include "hyvityslaskudialogi.h"

#include "model/tosite.h"
#include "model/lasku.h"

#include "ui_laskudialogi.h"

#include "rivivientigeneroija.h"
#include "db/kirjanpito.h"


HyvitysLaskuDialogi::HyvitysLaskuDialogi(Tosite *tosite, QWidget *parent) :
    RivillinenLaskuDialogi(tosite, parent)
{
    setWindowTitle( tr("Hyvityslasku laskulle %1").arg(tosite->lasku().alkuperaisNumero()));

    ui->laskunPvmLabel->setText(tr("Hyvityslaskun pvm"));
    ui->maksuCombo->setVisible(false);
    ui->valvontaLabel->hide();
    ui->valvontaCombo->hide();
    ui->tarkeCombo->hide();
    ui->eraLabel->hide();
    ui->eraDate->hide();
    ui->maksuaikaLabel->hide();
    ui->maksuaikaSpin->hide();
    ui->viivkorkoLabel->hide();
    ui->viivkorkoSpin->hide();

    int toistoIndex = ui->tabWidget->indexOf( ui->tabWidget->findChild<QWidget*>("toisto") );
    ui->tabWidget->removeTab(toistoIndex);
}

void HyvitysLaskuDialogi::asetaEra(int eraId)
{
    eraId_ = eraId;
}

void HyvitysLaskuDialogi::valmisteleTallennus()
{
    RiviVientiGeneroija rivigeneroija(kp());
    rivigeneroija.generoiViennit(tosite_, eraId_);
}
