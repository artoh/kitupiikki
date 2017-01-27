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

#include "tilinavaus.h"
#include "tilinavausmodel.h"
#include "kirjaus/eurodelegaatti.h"

Tilinavaus::Tilinavaus(QWidget *parent) : QWidget(parent)
{
    ui = new Ui::Tilinavaus;
    ui->setupUi(this);

    model = new TilinavausModel();
    ui->tiliView->setModel(model);

    ui->tiliView->setItemDelegateForColumn( TilinavausModel::SALDO, new EuroDelegaatti);

    connect(model, SIGNAL(infoteksti(QString)), this, SLOT(naytaInfo(QString)));
    connect(Kirjanpito::db(), SIGNAL(tietokantaVaihtui()), ui->tiliView, SLOT(resizeColumnsToContents()));
    connect(Kirjanpito::db(), SIGNAL(tietokantaVaihtui()), this, SLOT(tsekkaaMuokkaus()));
    connect(ui->tallennaNappi, SIGNAL(clicked(bool)), model, SLOT(tallenna()));
}

Tilinavaus::~Tilinavaus()
{
    delete ui;
}

void Tilinavaus::naytaInfo(QString info)
{
    ui->infoLabel->setText(info);
}

void Tilinavaus::tsekkaaMuokkaus()
{
    ui->tallennaNappi->setEnabled( model->voikoMuokata());
    ui->tiliView->setEnabled(model->voikoMuokata());
}
