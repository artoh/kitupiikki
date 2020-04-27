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

#include <QDateEdit>


#include "paivakirjaraportti.h"
#include "db/kirjanpito.h"
#include "db/tilikausi.h"
#include "raportinkirjoittaja.h"
#include "paivakirja.h"
#include "naytin/naytinikkuna.h"
#include <QDebug>
#include "tools/tulkki.h"

PaivakirjaRaportti::PaivakirjaRaportti()
    : RaporttiWidget(nullptr)
{
    ui = new Ui::Paivakirja;
    ui->setupUi( raporttiWidget );

    Tilikausi nykykausi = Kirjanpito::db()->tilikausiPaivalle( Kirjanpito::db()->paivamaara() );
    if( !nykykausi.alkaa().isValid())
        nykykausi = kp()->tilikaudet()->tilikausiIndeksilla( kp()->tilikaudet()->rowCount(QModelIndex()) - 1 );


    ui->alkupvm->setDate(nykykausi.alkaa());
    ui->loppupvm->setDate(nykykausi.paattyy());
    ui->kohdennusCombo->valitseNaytettavat(KohdennusProxyModel::KAIKKI);

    if( !kp()->kohdennukset()->kohdennuksia()) {
        ui->kohdennusCheck->setVisible(false);
        ui->kohdennusCombo->setVisible(false);
    }

    ui->tiliBox->hide();
    ui->tiliCombo->hide();
    ui->kumppaniCheck->hide();

    Tulkki::alustaKieliCombo(ui->kieliCombo);
}

PaivakirjaRaportti::~PaivakirjaRaportti()
{
    delete ui;
}

void PaivakirjaRaportti::esikatsele()
{
    int optiot = 0;
    int kohdennuksella = -1;
    if( ui->kohdennusCheck->isChecked()) {
        kohdennuksella = ui->kohdennusCombo->kohdennus();
        optiot |= Paivakirja::Kohdennuksella;
    }


    if( ui->tositejarjestysRadio->isChecked() )
        optiot |= Paivakirja::TositeJarjestyksessa;
    if( ui->ryhmittelelajeittainCheck->isChecked() )
        optiot |= Paivakirja::RyhmitteleLajeittain;
    if( ui->tulostakohdennuksetCheck->isChecked() )
        optiot |= Paivakirja::TulostaKohdennukset;
    if( ui->tulostasummat->isChecked() )
        optiot |= Paivakirja::TulostaSummat;

    Paivakirja *kirja = new Paivakirja(this, ui->kieliCombo->currentData().toString());
    connect( kirja, &Paivakirja::valmis, this, &RaporttiWidget::nayta );
    kirja->kirjoita( ui->alkupvm->date(), ui->loppupvm->date(),
                     optiot, kohdennuksella);
}


