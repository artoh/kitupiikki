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
#include "tilaus/planmodel.h"

#include "tilaus/tilauswizard.h"

UusiAlkuSivu::UusiAlkuSivu() :
    ui( new Ui::UusiAloitus)
{
    ui->setupUi(this);

    setTitle( tr("Tervetuloa!"));

    registerField("pilveen", ui->pilveenRadio);

    connect( kp()->pilvi(), &PilviModel::kirjauduttu,
             this, &UusiAlkuSivu::paivitaKirjautuminen);

    connect( ui->tilausNappi, &QPushButton::clicked, this,
             []() { TilausWizard *tilaus = new TilausWizard();
                    tilaus->nayta();} );

}

void UusiAlkuSivu::paivitaKirjautuminen()
{
    if( wizard()->currentPage() == this && wizard()->isVisible())
        initializePage();
}

void UusiAlkuSivu::initializePage()
{
    // Voidaanko tallentaa pilveen
    ui->pilveenRadio->setEnabled( kp()->pilvi()->kayttajaPilvessa() &&
                                  (kp()->pilvi()->omatPilvet() < kp()->pilvi()->pilviMax() || kp()->pilvi()->kkLisaPilviHinta()  ) &&
                                  kp()->pilvi()->tilausvoimassa() );

    if( !kp()->pilvi()->kayttajaPilvessa())
        ui->pilviInfo->setText( tr("Käyttääksesi pilveä luo ensin käyttäjätunnus "
                                   "tai kirjaudu sisään."));
    else if( kp()->pilvi()->omatPilvet() >= kp()->pilvi()->pilviMax()) {
        if( kp()->pilvi()->kkLisaPilviHinta() ) {
            ui->pilviInfo->setText( tr("Kirjanpidon tallentamisesta pilveen veloitetaan %1/kk").arg(kp()->pilvi()->kkLisaPilviHinta().display()));
        } else {
            ui->pilviInfo->setText( tr("Luodaksesi enemmän kirjanpitoja pilveen sinun "
                                   "on ensin päivitettävä tilauksesi laajempaan."));
        }
    } else if( !kp()->pilvi()->plan() && kp()->pilvi()->tilausvoimassa())
        ui->pilviInfo->setText( tr("Maksuttoman kokeilujaksosi ajan sinulla voi olla yksi pilveen tallennettu kirjanpito. Ellet tee maksullista kirjanpitoa, poistetaan tämä kirjanpito kokeilujaksosi päätyttyä."));

    if( kp()->pilvi()->plan())
        ui->tilausNappi->setText( tr("Päivitä tilauksesi"));

    if( ui->pilveenRadio->isEnabled())
        ui->pilveenRadio->setChecked(true);
    else
        ui->koneelleRadio->setChecked(true);

    ui->tilausNappi->setVisible( kp()->pilvi()->kayttajaPilvessa() );
}
