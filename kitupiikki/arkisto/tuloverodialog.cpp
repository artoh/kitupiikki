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
#include "tuloverodialog.h"
#include "ui_tuloverodialog.h"

TuloveroDialog::TuloveroDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TuloveroDialog)
{
    ui->setupUi(this);

    ui->veroEdit->setReadOnly(true);
    ui->yleveroEdit->setReadOnly(true);

    connect( ui->tuloEdit, &KpEuroEdit::textEdited, this, &TuloveroDialog::paivitaYlevero);
    connect( ui->tuloEdit, &KpEuroEdit::textEdited, this, &TuloveroDialog::paivitaVahennys);
    connect( ui->vahennysEdit, &KpEuroEdit::textEdited, this, &TuloveroDialog::paivitaVahennys);
    connect( ui->yleveroEdit, &KpEuroEdit::textEdited, this, &TuloveroDialog::paivitaVahennys);
    connect( ui->tulosEdit, &KpEuroEdit::textEdited, this, &TuloveroDialog::paivitaVero);
    connect( ui->veroEdit, &KpEuroEdit::textEdited, this, &TuloveroDialog::paivitaVero);
    connect( ui->maksetutEdit, &KpEuroEdit::textEdited, this, &TuloveroDialog::paivitaJaannos);
}

TuloveroDialog::~TuloveroDialog()
{
    delete ui;
}

void TuloveroDialog::paivitaYlevero()
{
    double tulo = ui->tuloEdit->value();
    if( tulo < 50000)
        ui->yleveroEdit->setValue(0);
    else if( tulo >= 867143)
        ui->yleveroEdit->setValue(3000);
    else
        ui->yleveroEdit->setValue(140+(0.0035*tulo));
    paivitaVahennys();
}

void TuloveroDialog::paivitaVahennys()
{
    ui->vahennysYhteensa->setValue( ui->vahennysEdit->value() + ui->yleveroEdit->value());
    paivitaTulos();
}

void TuloveroDialog::paivitaTulos()
{
    ui->tulosEdit->setValue( ui->tuloEdit->value() - ui->vahennysYhteensa->value());
    paivitaVero();
}

void TuloveroDialog::paivitaVero()
{
    if( ui->tulosEdit->value() > 1e-5)
        ui->veroEdit->setValue(0.2 * ui->tulosEdit->value());
    else
        ui->veroEdit->setValue(0);
    paivitaJaannos();
}

void TuloveroDialog::paivitaJaannos()
{
    ui->jaaveroaEdit->setValue( ui->veroEdit->value() - ui->maksetutEdit->value());
}
