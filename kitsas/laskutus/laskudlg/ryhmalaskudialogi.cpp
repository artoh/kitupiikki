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
#include "ryhmalaskudialogi.h"
#include "../ryhmalasku/ryhmalaskutab.h"

#include "ui_laskudialogi.h"

RyhmaLaskuDialogi::RyhmaLaskuDialogi(Tosite *tosite, QWidget *parent) :
    RivillinenLaskuDialogi(tosite, parent),
    ryhmalaskuTab_(new RyhmalaskuTab)
{
    setWindowTitle(tr("Ryhmälasku"));

    int toistoIndex = ui->tabWidget->indexOf( ui->tabWidget->findChild<QWidget*>("toisto") );
    ui->tabWidget->removeTab(toistoIndex);

    ui->tabWidget->addTab( ryhmalaskuTab_, QIcon(":/pic/asiakkaat.png"), tr("Laskutettavat"));

    ui->asiakasLabel->hide();
    ui->asiakas->hide();
    ui->osoiteLabel->hide();
    ui->osoiteEdit->hide();
    ui->emailLabel->hide();
    ui->email->hide();
    ui->asviiteLabel->hide();
    ui->asViiteEdit->hide();
    ui->kieliCombo->hide();
    ui->valmisNappi->hide();
    ui->laskutusCombo->hide();

    ui->luonnosNappi->hide();

}
