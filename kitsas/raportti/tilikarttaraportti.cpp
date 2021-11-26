/*
   Copyright (C) 2017,2018 Arto Hyvättinen

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

#include "tilikarttaraportti.h"
#include "db/asetusmodel.h"
#include <QJsonDocument>
#include "kieli/monikielinen.h"
#include "kieli/kielet.h"

TilikarttaRaportti::TilikarttaRaportti()
    : RaporttiWidget(nullptr)
{
    ui = new Ui::TilikarttaRaportti;
    ui->setupUi( raporttiWidget);

    connect( ui->tilikaudeltaCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(paivitaPaiva()));

    ui->tilikaudeltaCombo->setModel( kp()->tilikaudet() );
    ui->tilikaudeltaCombo->setModelColumn( TilikausiModel::KAUSI );
    ui->tilikaudeltaCombo->setCurrentIndex( ui->tilikaudeltaCombo->count() - 1);    

    ui->saldotDate->setDateRange( kp()->tilikaudet()->kirjanpitoAlkaa(),
                                  kp()->tilikaudet()->kirjanpitoLoppuu());

}

TilikarttaRaportti::~TilikarttaRaportti()
{
    delete ui;
}

void TilikarttaRaportti::tallenna()
{
    aseta(RaporttiValinnat::Tyyppi, "tililuettelo");
    aseta(RaporttiValinnat::LuetteloPvm, ui->tilikaudeltaCombo->currentData( TilikausiModel::PaattyyRooli ).toDate());
    aseta(RaporttiValinnat::SaldoPvm, ui->saldotCheck->isChecked() ? ui->saldotDate->date() : QDate());
    aseta(RaporttiValinnat::Kieli, ui->kieliCombo->currentData());

    aseta(RaporttiValinnat::NaytaTyypit, ui->tilityypitCheck->isChecked());
    aseta(RaporttiValinnat::NaytaKirjausohjeet, ui->kirjausohjeet->isChecked());
    aseta(RaporttiValinnat::NaytaOtsikot, ui->otsikotCheck->isChecked());

    if( ui->kaytossaRadio->isChecked())
        aseta(RaporttiValinnat::LuettelonTilit,"kaytossa");
    else if( ui->kirjauksiaRadio->isChecked())
        aseta(RaporttiValinnat::LuettelonTilit,"kirjatut");
    else if( ui->suosikkiRadio->isChecked())
        aseta(RaporttiValinnat::LuettelonTilit,"suosikki");
    else
        aseta(RaporttiValinnat::LuettelonTilit,"kaikki");

}

void TilikarttaRaportti::paivitaPaiva()
{
    Tilikausi kausi = kp()->tilikaudet()->tilikausiPaivalle( ui->tilikaudeltaCombo->currentData( TilikausiModel::PaattyyRooli ).toDate() );
    ui->saldotDate->setDate( kausi.paattyy());
}
