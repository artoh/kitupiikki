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

#include "kohdennusdialog.h"
#include "ui_kohdennusdialog.h"

#include "db/kirjanpito.h"

KohdennusDialog::KohdennusDialog(KohdennusModel *model, QModelIndex index, QWidget *parent)
    : QDialog(parent),
     ui(new Ui::KohdennusDialog),
     model_(model),
     index_(index)

{
    ui->setupUi(this);

    // Oletuskestot
    Tilikausi nykyinen = kp()->tilikausiPaivalle( kp()->paivamaara() );
    ui->alkaaDate->setDate( nykyinen.alkaa());
    ui->paattyyDate->setDate( nykyinen.paattyy());

    ui->alkaaDate->setMinimumDate( kp()->tilikaudet()->kirjanpitoAlkaa() );
    ui->paattyyDate->setMinimumDate( kp()->tilikaudet()->kirjanpitoAlkaa() );

    connect( ui->alkaaDate, SIGNAL(dateChanged(QDate)), this, SLOT(tarkennaLoppuMinimi()));

    if( index.isValid())
        lataa();
}

KohdennusDialog::~KohdennusDialog()
{
    delete ui;
}

void KohdennusDialog::tarkennaLoppuMinimi()
{
    if( ui->alkaaDate->date().daysTo( ui->paattyyDate->minimumDate() ) < 0)
    {
        // Päättyy ennen kuin alkaakaan, sehän ei passaa!
        ui->paattyyDate->setMinimumDate( ui->alkaaDate->date());
    }
}

void KohdennusDialog::accept()
{
    if( !ui->nimiEdit->text().isEmpty())
    {
        tallenna();
        QDialog::accept();
    }
}

void KohdennusDialog::lataa()
{
    ui->kustannuspaikkaRadio->setChecked( index_.data(KohdennusModel::TyyppiRooli).toInt() == Kohdennus::KUSTANNUSPAIKKA);
    ui->projektiRadio->setChecked(index_.data(KohdennusModel::TyyppiRooli).toInt() == Kohdennus::PROJEKTI );
    ui->nimiEdit->setText( index_.data(KohdennusModel::NimiRooli).toString());

    ui->maaraaikainenCheck->setChecked( index_.data(KohdennusModel::AlkaaRooli ).toDate().isValid() );
    ui->alkaaDate->setDate( index_.data(KohdennusModel::AlkaaRooli).toDate());
    ui->paattyyDate->setDate( index_.data(KohdennusModel::PaattyyRooli).toDate());
}

void KohdennusDialog::tallenna()
{
    Kohdennus::KohdennusTyyppi tyyppi = Kohdennus::EIKOHDENNETA;
    if( ui->kustannuspaikkaRadio->isChecked())
        tyyppi = Kohdennus::KUSTANNUSPAIKKA;
    else if( ui->projektiRadio->isChecked())
        tyyppi = Kohdennus::PROJEKTI;


    if( index_.isValid())
    {
        // Päivitetään
        model_->setData(index_, tyyppi, KohdennusModel::TyyppiRooli);
        model_->setData(index_, ui->nimiEdit->text(), KohdennusModel::NimiRooli);

        if( ui->maaraaikainenCheck->isChecked())
        {
            model_->setData(index_, ui->alkaaDate->date(), KohdennusModel::AlkaaRooli);
            model_->setData(index_, ui->paattyyDate->date(), KohdennusModel::PaattyyRooli);
        }
        else
        {
            model_->setData(index_, QDate(), KohdennusModel::AlkaaRooli);
            model_->setData(index_, QDate(), KohdennusModel::PaattyyRooli);
        }
    }
    else
    {
        // Tallennetaan uusi
        Kohdennus uusi(tyyppi, ui->nimiEdit->text());
        if( ui->maaraaikainenCheck->isChecked())
        {
            uusi.asetaAlkaa( ui->alkaaDate->date());
            uusi.asetaPaattyy( ui->paattyyDate->date());
        }
        model_->lisaaUusi(uusi);

    }
}


