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
#include "uusitilikausidlg.h"
#include "ui_lisaatilikausidlg.h"

#include "db/kirjanpito.h"
#include "db/tilikausi.h"

UusiTilikausiDlg::UusiTilikausiDlg(QWidget *parent) : QDialog(parent), ui(new Ui::UusiTilikausiDlg)
{
    ui->setupUi(this);

    loppuun();
    ui->alkuRadio->setEnabled( !kp()->asetukset()->luku("Tilinavaus") );

    connect( ui->alkuRadio, &QRadioButton::clicked, this, &UusiTilikausiDlg::alkuun );
    connect( ui->loppuRadio, &QRadioButton::clicked, this, &UusiTilikausiDlg::loppuun);
}

void UusiTilikausiDlg::accept()
{
    if( ui->avausCheck->isChecked() && ui->alkuRadio->isChecked()) {
        QDate avauspvm = ui->paattyyEdit->date();

        kp()->asetukset()->aseta("Tilinavaus", 2);
        kp()->asetukset()->aseta("TilinavausPvm", avauspvm);
        if( !kp()->tilitpaatetty().isValid() || kp()->tilitpaatetty() < avauspvm)
            kp()->asetukset()->aseta("TilitPaatetty", avauspvm);
    }
    Tilikausi uusi(ui->alkaaEdit->date(), ui->paattyyEdit->date());
    uusi.tallenna();

    QDialog::accept();
}

void UusiTilikausiDlg::alkuun()
{
    ui->alkaaEdit->setEnabled(true);
    ui->paattyyEdit->setEnabled(false);
    ui->avausCheck->setEnabled(true);

    QDate loppu = kp()->tilikaudet()->kirjanpitoAlkaa().addDays(-1);
    ui->paattyyEdit->setDateRange(QDate(2000,1,1),QDate(2200,1,1));
    ui->paattyyEdit->setDate(loppu);

    ui->alkaaEdit->setMinimumDate(loppu.addMonths(-18));
    ui->alkaaEdit->setMaximumDate(loppu.addDays(-1));
    ui->alkaaEdit->setDate(loppu.addYears(-1).addDays(1));
}

void UusiTilikausiDlg::loppuun()
{
    ui->alkaaEdit->setEnabled(false);
    ui->loppuRadio->setEnabled(true);
    ui->avausCheck->setEnabled(false);
    ui->alkaaEdit->setDateRange(QDate(2000,1,1),QDate(2200,1,1));

    QDate edellinen = kp()->tilikaudet()->kirjanpitoLoppuu();
    ui->alkaaEdit->setDate( edellinen.addDays(1) );
    ui->paattyyEdit->setMinimumDate( edellinen.addDays(2) );
    ui->paattyyEdit->setMaximumDate( edellinen.addMonths(18));
    ui->paattyyEdit->setDate( edellinen.addYears(1));
}
