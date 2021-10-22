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

#include "raporttivalinnat.h"

PaivakirjaRaportti::PaivakirjaRaportti()
    : RaporttiWidget(nullptr)
{
    ui = new Ui::Paivakirja;
    ui->setupUi( raporttiWidget );

    ui->alkupvm->setDate( kp()->raporttiValinnat()->arvo(RaporttiValinnat::AlkuPvm).toDate() );
    ui->loppupvm->setDate( kp()->raporttiValinnat()->arvo(RaporttiValinnat::LoppuPvm).toDate() );

    ui->ryhmittelelajeittainCheck->setChecked( kp()->raporttiValinnat()->onko(RaporttiValinnat::RyhmitteleTositelajit) );
    ui->tulostakohdennuksetCheck->setChecked( kp()->raporttiValinnat()->onko(RaporttiValinnat::TulostaKohdennus) );
    ui->tulostasummat->setChecked( kp()->raporttiValinnat()->onko(RaporttiValinnat::TulostaSummarivit));
    ui->kumppaniCheck->setChecked( kp()->raporttiValinnat()->onko(RaporttiValinnat::TulostaKumppani));
    ui->eriPaivatCheck->setChecked( kp()->raporttiValinnat()->onko(RaporttiValinnat::ErittelePaivat));

    ui->kohdennusCombo->valitseNaytettavat(KohdennusProxyModel::KAIKKI);

    if( !kp()->kohdennukset()->kohdennuksia()) {
        ui->kohdennusCheck->setVisible(false);
        ui->kohdennusCombo->setVisible(false);
    } else {
        int kohdennus = kp()->raporttiValinnat()->arvo(RaporttiValinnat::Kohdennuksella).toInt();
        ui->kohdennusCheck->setChecked(kohdennus > -1);
        ui->kohdennusCombo->setCurrentIndex( ui->kohdennusCombo->findData(kohdennus, KohdennusModel::IdRooli) );
    }

    ui->tiliBox->hide();
    ui->tiliCombo->hide();
    ui->laatuLabel->hide();
    ui->laatuSlider->hide();

    connect( ui->alkupvm, &QDateEdit::dateChanged, this, &PaivakirjaRaportti::paivitaKohdennukset);
    connect( ui->loppupvm, &QDateEdit::dateChanged, this, &PaivakirjaRaportti::paivitaKohdennukset);

    paivitaKohdennukset();


}

PaivakirjaRaportti::~PaivakirjaRaportti()
{
    delete ui;
}

void PaivakirjaRaportti::paivitaKohdennukset()
{
    ui->kohdennusCombo->suodataValilla(ui->alkupvm->date(), ui->loppupvm->date());
}

void PaivakirjaRaportti::esikatsele()
{    
    kp()->raporttiValinnat()->aseta(RaporttiValinnat::Tyyppi, "paivakirja");
    kp()->raporttiValinnat()->aseta(RaporttiValinnat::AlkuPvm, ui->alkupvm->date());
    kp()->raporttiValinnat()->aseta(RaporttiValinnat::LoppuPvm, ui->loppupvm->date());

    if( ui->kohdennusCheck->isVisible() && ui->kohdennusCheck->isChecked()) {
        kp()->raporttiValinnat()->aseta(RaporttiValinnat::Kohdennuksella, ui->kohdennusCombo->kohdennus());
    } else {
        kp()->raporttiValinnat()->aseta(RaporttiValinnat::Kohdennuksella, -1);
    }

    if( ui->tositejarjestysRadio->isChecked() )
        kp()->raporttiValinnat()->aseta(RaporttiValinnat::VientiJarjestys, "tosite");
    else
        kp()->raporttiValinnat()->aseta(RaporttiValinnat::VientiJarjestys, "pvm");

    kp()->raporttiValinnat()->aseta(RaporttiValinnat::RyhmitteleTositelajit, ui->ryhmittelelajeittainCheck->isChecked());
    kp()->raporttiValinnat()->aseta(RaporttiValinnat::TulostaKohdennus, ui->tulostakohdennuksetCheck->isChecked());
    kp()->raporttiValinnat()->aseta(RaporttiValinnat::TulostaSummarivit, ui->tulostasummat->isChecked());
    kp()->raporttiValinnat()->aseta(RaporttiValinnat::TulostaKumppani, ui->kumppaniCheck->isChecked());
    kp()->raporttiValinnat()->aseta(RaporttiValinnat::ErittelePaivat, ui->eriPaivatCheck->isChecked());
    kp()->raporttiValinnat()->aseta(RaporttiValinnat::Kieli, ui->kieliCombo->currentData().toString());

    NaytinIkkuna::naytaRaportti( *kp()->raporttiValinnat() );

    /*

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
    if( ui->kumppaniCheck->isChecked())
        optiot |= Paivakirja::AsiakasToimittaja;
    if( ui->eriPaivatCheck->isChecked() )
        optiot |= Paivakirja::ErittelePaivat;

    Paivakirja *kirja = new Paivakirja(this, ui->kieliCombo->currentData().toString());
    connect( kirja, &Paivakirja::valmis, this, &RaporttiWidget::nayta );
    kirja->kirjoita( ui->alkupvm->date(), ui->loppupvm->date(),
                     optiot, kohdennuksella);

    */
}


