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

#include <QSqlQuery>
#include "db/kirjanpito.h"

#include "tositeluetteloraportti.h"
#include "tositeluettelo.h"

#include "tools/tulkki.h"

TositeluetteloRaportti::TositeluetteloRaportti()
    : RaporttiWidget(nullptr)
{
    ui = new Ui::Paivakirja;
    ui->setupUi(raporttiWidget);

    Tilikausi nykykausi = Kirjanpito::db()->tilikausiPaivalle( Kirjanpito::db()->paivamaara() );
    if( !nykykausi.alkaa().isValid())
        nykykausi = kp()->tilikaudet()->tilikausiIndeksilla( kp()->tilikaudet()->rowCount(QModelIndex()) - 1 );

    ui->alkupvm->setDate(nykykausi.alkaa());
    ui->loppupvm->setDate(nykykausi.paattyy());

    ui->kohdennusCheck->hide();
    ui->kohdennusCombo->hide();
    ui->tulostakohdennuksetCheck->hide();

    ui->tiliBox->hide();
    ui->tiliCombo->hide();

    ui->ryhmittelelajeittainCheck->setChecked(true);
    ui->tositejarjestysRadio->setChecked(true);
    ui->tulostakohdennuksetCheck->setEnabled(false);

    ui->kumppaniCheck->setChecked(true);

    Tulkki::alustaKieliCombo(ui->kieliCombo);
}


void TositeluetteloRaportti::esikatsele()
{
    int optiot = 0;
    if( ui->tositejarjestysRadio->isChecked())
        optiot |= TositeLuettelo::TositeJarjestyksessa;
    if( ui->ryhmittelelajeittainCheck->isChecked())
      optiot |= TositeLuettelo::RyhmitteleLajeittain;
    if( ui->tulostasummat->isChecked())
        optiot |= TositeLuettelo::TulostaSummat;
    if( ui->kumppaniCheck->isChecked())
        optiot |= TositeLuettelo::AsiakasToimittaja;

    TositeLuettelo *luettelo = new TositeLuettelo(this);
    connect( luettelo, &TositeLuettelo::valmis, this, &RaporttiWidget::nayta);
    luettelo->kirjoita(ui->alkupvm->date(),
                       ui->loppupvm->date(),
                       optiot);
}


