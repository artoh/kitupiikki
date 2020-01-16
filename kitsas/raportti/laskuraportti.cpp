/*
   Copyright (C) 2018 Arto Hyvättinen

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
#include <QDebug>

#include "laskuraportti.h"
#include "db/kirjanpito.h"
#include "laskuraportteri.h"

LaskuRaportti::LaskuRaportti()
{
    ui = new Ui::Laskuraportti;
    ui->setupUi( raporttiWidget );

    ui->alkaenPvm->setDate( kp()->tilikaudet()->kirjanpitoAlkaa() );
    ui->paattyenPvm->setDate( kp()->tilikaudet()->kirjanpitoLoppuu() );
    ui->saldoPvm->setDate( kp()->paivamaara() );

    connect( ui->myyntiRadio, SIGNAL(toggled(bool)), this, SLOT(tyyppivaihtuu()));

}

LaskuRaportti::~LaskuRaportti()
{
    delete ui;
}

void LaskuRaportti::esikatsele()
{
    LaskuRaportteri *laskut = new LaskuRaportteri(this);
    connect( laskut, &LaskuRaportteri::valmis, this, &RaporttiWidget::nayta);

    int optiot = 0;

    if( ui->myyntiRadio->isChecked())
        optiot += LaskuRaportteri::Myyntilaskut;
    else
        optiot += LaskuRaportteri::Ostolaskut;

    if( ui->rajaaPvm->isChecked())
        optiot += LaskuRaportteri::RajaaLaskupaivalla;
    else if( ui->rajaaEra->isChecked())
        optiot += LaskuRaportteri::RajaaErapaivalla;

    if( ui->avoimet->isChecked())
        optiot += LaskuRaportteri::VainAvoimet;

    if( ui->lajitteleNumero->isChecked())
        optiot += LaskuRaportteri::LajitteleNumero;
    else if( ui->lajitteleViite->isChecked())
        optiot += LaskuRaportteri::LajitteleViite;
    else if( ui->lajitteleErapvm->isChecked())
        optiot += LaskuRaportteri::LajitteleErapvm;
    else if( ui->lajitteleSumma->isChecked())
        optiot += LaskuRaportteri::LajitteleSumma;
    else if( ui->lajitteleAsiakas->isChecked())
        optiot += LaskuRaportteri::LajitteleAsiakas;

    if( ui->naytaViiteCheck->isChecked())
        optiot += LaskuRaportteri::NaytaViite;
    if( ui->vainKitsas->isChecked())
        optiot += LaskuRaportteri::VainKitsas;
    if( ui->summaBox->isChecked())
        optiot += LaskuRaportteri::TulostaSummat;

    laskut->kirjoita(optiot, ui->saldoPvm->date(),
                     ui->alkaenPvm->date(), ui->paattyenPvm->date());

}

void LaskuRaportti::tyyppivaihtuu()
{
    ui->lajitteleViite->setEnabled( ui->myyntiRadio->isChecked() );
    if( ui->myyntiRadio->isChecked())
        ui->lajitteleAsiakas->setText( tr("Asiakas"));
    else
        ui->lajitteleAsiakas->setText(tr("Toimittaja"));

    ui->vainKitsas->setVisible( ui->myyntiRadio->isChecked() );
}
