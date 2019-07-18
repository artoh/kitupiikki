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
#include "asiakasdlg.h"
#include "ui_asiakasdlg.h"

#include "model/asiakas.h"
#include "postinumerot.h"

#include "validator/ytunnusvalidator.h"

AsiakasDlg::AsiakasDlg(QWidget *parent, Asiakas *asiakas) :
    QDialog(parent),
    ui(new Ui::AsiakasDlg),
    asiakas_(asiakas)
{
    ui->setupUi(this);

    ui->nimiEdit->setText( asiakas->nimi());
    ui->ytunnusEdit->setText( asiakas->ytunnus());
    ui->ytunnusEdit->setValidator(new YTunnusValidator(false));

    ui->lahiEdit->setText( asiakas->osoite());
    ui->postinumeroEdit->setText( asiakas->postinumero());
    ui->kaupunkiEdit->setText( asiakas->kaupunki());

    ui->verkkolaskuEdit->setText( asiakas->ovt());
    ui->valittajaEdit->setText( asiakas->operaattori());

    connect( ui->postinumeroEdit, &QLineEdit::textChanged, this, &AsiakasDlg::haeToimipaikka);
}

AsiakasDlg::~AsiakasDlg()
{
    delete ui;
}

void AsiakasDlg::accept()
{
    asiakas_->set("nimi", ui->nimiEdit->text());

    QDialog::accept();
}

void AsiakasDlg::haeToimipaikka()
{
    QString toimipaikka = Postinumerot::toimipaikka( ui->postinumeroEdit->text() );
    if( !toimipaikka.isEmpty())
        ui->kaupunkiEdit->setText(toimipaikka);
}
