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

#include "verodialogi.h"
#include "ui_verodialogi.h"
#include "db/verotyyppimodel.h"
#include "db/kirjanpito.h"

VeroDialogi::VeroDialogi(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::VeroDialogi)
{
    ui->setupUi(this);
    ui->verolajiCombo->setModel( kp()->alvTyypit());

    connect( ui->verolajiCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(lajimuuttuu()));

}

VeroDialogi::~VeroDialogi()
{
    delete ui;
}

void VeroDialogi::lajimuuttuu()
{
    // Nollaverolajeilla ei voi tehdä verokirjauksia
    ui->tyyppiGroup->setEnabled(  !ui->verolajiCombo->currentData( VerotyyppiModel::NollaLajiRooli).toBool());
    ui->prossaSpin->setEnabled(  !ui->verolajiCombo->currentData( VerotyyppiModel::NollaLajiRooli).toBool());

}

VeroDialogiValinta VeroDialogi::veroDlg(int koodi, int prosentti)
{
    VeroDialogi dlg;

    if( koodi > AlvKoodi::ALVVAHENNYS)
        dlg.ui->vahennysRadio->setChecked(true);
    else if( koodi > AlvKoodi::ALVKIRJAUS)
        dlg.ui->veroRadio->setChecked(true);

    dlg.ui->verolajiCombo->setCurrentIndex( dlg.ui->verolajiCombo->findData( koodi % 100));
    dlg.ui->prossaSpin->setValue(prosentti);
    dlg.lajimuuttuu();

    VeroDialogiValinta palautettava;

    if( dlg.exec())
    {
        palautettava.verokoodi = dlg.ui->verolajiCombo->currentData(VerotyyppiModel::KoodiRooli).toInt();
        if( dlg.ui->verolajiCombo->currentData( VerotyyppiModel::NollaLajiRooli).toBool() )
        {
            palautettava.veroprosentti = 0;
            return palautettava;
        }
        else
        {
            if( dlg.ui->veroRadio->isChecked())
                palautettava.verokoodi += AlvKoodi::ALVKIRJAUS;
            else if( dlg.ui->vahennysRadio->isChecked())
                palautettava.verokoodi += AlvKoodi::ALVVAHENNYS;
            palautettava.veroprosentti = dlg.ui->prossaSpin->value();
        }

    }
    else
    {
        palautettava.verokoodi = koodi;
        palautettava.veroprosentti = prosentti;
    }

    return palautettava;
}
