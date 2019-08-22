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
#include "uusialkusivu.h"

#include "ui_uusialoitus.h"
#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"

UusiAlkuSivu::UusiAlkuSivu() :
    ui( new Ui::UusiAloitus)
{
    ui->setupUi(this);

    setTitle( tr("Tervetuloa!"));

    // Voidaanko tallentaa pilveen
    ui->pilveenRadio->setEnabled( kp()->pilvi()->kayttajaPilvessa() &&
                                  kp()->pilvi()->omatPilvet() < kp()->pilvi()->pilviMax());

    if( !kp()->pilvi()->kayttajaPilvessa())
        ui->pilviInfo->setText( tr("Käyttääksesi pilveä luo ensin käyttäjätunnus "
                                   "tai kirjaudu sisään."));
    else if( kp()->pilvi()->omatPilvet() >= kp()->pilvi()->pilviMax() )
        ui->pilviInfo->setText( tr("Luodaksesi enemmän kirjanpitoja pilveen sinun "
                                   "on ensin päivitettävä tilauksesi laajempaan."));
    else if( !kp()->pilvi()->plan() )
        ui->pilviInfo->setText( tr("Voit kokeilla maksutta kirjanpitoa pilvessä 50 tositteen "
                                   "verran ennen kuin tilaat maksullisen pilvipalvelun."));

    if( kp()->pilvi()->plan())
        ui->tilausNappi->setText( tr("Päivitä tilauksesi"));

    if( ui->pilveenRadio->isEnabled())
        ui->pilveenRadio->setChecked(true);
    else
        ui->koneelleRadio->setChecked(true);

    ui->tilausNappi->setVisible( kp()->pilvi()->kayttajaPilvessa() );


    registerField("pilveen", ui->pilveenRadio);
}
