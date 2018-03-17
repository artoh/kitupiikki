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

int VeroDialogi::alvProsentti() const
{
    if( ui->verolajiCombo->currentData( VerotyyppiModel::NollaLajiRooli).toBool() )
    {
        return 0;
    }
    else
    {
        return ui->prossaSpin->value();
    }
}

int VeroDialogi::alvKoodi() const
{
    int koodi = ui->verolajiCombo->currentData(VerotyyppiModel::KoodiRooli).toInt();

    if( !ui->verolajiCombo->currentData( VerotyyppiModel::NollaLajiRooli).toBool() )
    {
        if( ui->veroRadio->isChecked())
            return koodi + AlvKoodi::ALVKIRJAUS;
        else if( ui->vahennysRadio->isChecked())
            return koodi + AlvKoodi::ALVVAHENNYS;
        else if( koodi == AlvKoodi::MAKSUPERUSTEINEN_MYYNTI || koodi == AlvKoodi::MAKSUPERUSTEINEN_OSTO )
        {
            if( ui->kohdentamaton->isChecked())
                return koodi + AlvKoodi::MAKSUPERUSTEINEN_KOHDENTAMATON;
        }
    }
    return koodi;
}

int VeroDialogi::exec(int koodi, int prosentti, bool tyyppilukko)
{

    if( koodi > AlvKoodi::MAKSUPERUSTEINEN_KOHDENTAMATON )
        ui->kohdentamaton->setChecked(true);
    else if( koodi > AlvKoodi::ALVVAHENNYS)
        ui->vahennysRadio->setChecked(true);
    else if( koodi > AlvKoodi::ALVKIRJAUS)
        ui->veroRadio->setChecked(true);


    ui->verolajiCombo->setCurrentIndex( ui->verolajiCombo->findData( koodi % 100));
    ui->prossaSpin->setValue(prosentti);

    // Jos tyyppi lukittu, ei voi muuttaa kirjausta veroksi jne...
    ui->vahennysRadio->setEnabled(!tyyppilukko);
    ui->veroRadio->setEnabled(!tyyppilukko);
    ui->veronalainenRadio->setEnabled(!tyyppilukko);

    lajimuuttuu();

    return QDialog::exec();

}

void VeroDialogi::lajimuuttuu()
{
    // Nollaverolajeilla ei voi tehdä verokirjauksia
    ui->tyyppiGroup->setEnabled(  !ui->verolajiCombo->currentData( VerotyyppiModel::NollaLajiRooli).toBool());
    ui->prossaSpin->setEnabled(  !ui->verolajiCombo->currentData( VerotyyppiModel::NollaLajiRooli).toBool() );

    if( !ui->verolajiCombo->currentData( VerotyyppiModel::NollaLajiRooli).toBool() && !ui->prossaSpin->value())
        ui->prossaSpin->setValue( VerotyyppiModel::oletusAlvProsentti() );

    int alvkoodi = ui->verolajiCombo->currentData( VerotyyppiModel::KoodiRooli).toInt();
    ui->kohdentamaton->setEnabled( alvkoodi == AlvKoodi::MAKSUPERUSTEINEN_MYYNTI || alvkoodi == AlvKoodi::MAKSUPERUSTEINEN_OSTO);
}

