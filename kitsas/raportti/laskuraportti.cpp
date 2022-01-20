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

#include "raporttivalinnat.h"

LaskuRaportti::LaskuRaportti()
{
    ui = new Ui::Laskuraportti;
    ui->setupUi( raporttiWidget );

    ui->alkaenPvm->setDate( arvo(RaporttiValinnat::AlkuPvm).toDate() );
    ui->paattyenPvm->setDate( arvo(RaporttiValinnat::LoppuPvm).toDate());

    ui->myyntiRadio->setChecked( arvo(RaporttiValinnat::LaskuTyyppi).toString() != "osto" );

    const QString rajaus = arvo(RaporttiValinnat::LaskuRajausTyyppi).toString();
    if( rajaus == "erapvm")
        ui->rajaaEra->setChecked(true);
    else
        ui->rajaaPvm->setChecked(true);

    ui->avoimet->setChecked( onko(RaporttiValinnat::VainAvoimet) );

    QDate saldopvm = arvo(RaporttiValinnat::SaldoPvm).toDate();
    ui->saldoPvm->setDate( saldopvm.isValid() ? saldopvm : kp()->paivamaara() );

    const QString lajittelu = arvo(RaporttiValinnat::LaskunLajittelu).toString();
    if( lajittelu == "numero")
        ui->lajitteleNumero->setChecked(true);
    else if( lajittelu == "viite")
        ui->lajitteleViite->setChecked(true);
    else if( lajittelu == "erapvm")
        ui->lajitteleErapvm->setChecked(true);
    else if( lajittelu == "asiakas")
        ui->lajitteleAsiakas->setChecked(true);
    else if( lajittelu == "summa")
        ui->lajitteleNumero->setChecked(true);
    else
        ui->lajitteleLaskupvm->setChecked(true);

    ui->naytaViiteCheck->setChecked( onko(RaporttiValinnat::NaytaViitteet) );
    ui->vainKitsas->setChecked( onko(RaporttiValinnat::VainKitsaalla));
    ui->summaBox->setChecked( onko(RaporttiValinnat::TulostaSummarivit));

    ui->kieliCombo->valitse(arvo(RaporttiValinnat::Kieli).toString() );

    connect( ui->myyntiRadio, SIGNAL(toggled(bool)), this, SLOT(tyyppivaihtuu()));

}

LaskuRaportti::~LaskuRaportti()
{
    delete ui;
}

void LaskuRaportti::tallenna()
{
    aseta(RaporttiValinnat::Tyyppi, "laskut");

    aseta(RaporttiValinnat::AlkuPvm, ui->alkaenPvm->date());
    aseta(RaporttiValinnat::LoppuPvm, ui->paattyenPvm->date());

    aseta(RaporttiValinnat::LaskuTyyppi,
        ui->myyntiRadio->isChecked() ? "myynti" : "osto");

    if( ui->rajaaPvm->isChecked())
        aseta(RaporttiValinnat::LaskuRajausTyyppi, "laskupvm");
    else
        aseta(RaporttiValinnat::LaskuRajausTyyppi, "erapvm");

    aseta(RaporttiValinnat::VainAvoimet, ui->avoimet->isChecked());

    if( ui->lajitteleNumero->isChecked())
        aseta(RaporttiValinnat::LaskunLajittelu, "numero");
    else if( ui->lajitteleViite->isChecked())
        aseta(RaporttiValinnat::LaskunLajittelu, "viite");
    else if( ui->lajitteleErapvm->isChecked())
        aseta(RaporttiValinnat::LaskunLajittelu, "erapvm");
    else if( ui->lajitteleSumma->isChecked())
        aseta(RaporttiValinnat::LaskunLajittelu, "summa");
    else if( ui->lajitteleAsiakas->isChecked())
        aseta(RaporttiValinnat::LaskunLajittelu, "asiakas");

    aseta(RaporttiValinnat::NaytaViitteet, ui->naytaViiteCheck->isChecked());
    aseta(RaporttiValinnat::VainKitsaalla, ui->vainKitsas->isChecked());
    aseta(RaporttiValinnat::TulostaSummarivit, ui->summaBox->isChecked());
    aseta(RaporttiValinnat::SaldoPvm, ui->saldoPvm->date());



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
