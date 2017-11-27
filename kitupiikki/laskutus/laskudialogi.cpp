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

#include "db/kirjanpito.h"

#include "laskudialogi.h"
#include "ui_laskudialogi.h"

#include "kirjaus/eurodelegaatti.h"
#include "kirjaus/kohdennusdelegaatti.h"
#include "kirjaus/tilidelegaatti.h"

#include "kirjaus/verodialogi.h"

LaskuDialogi::LaskuDialogi(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LaskuDialogi)
{
    ui->setupUi(this);

    ui->perusteCombo->addItem("Suoriteperusteinen", SUORITEPERUSTE);
    ui->perusteCombo->addItem("Laskutusperusteinen", LASKUTUSPERUSTE);
    ui->perusteCombo->addItem("Maksuperusteinen", MAKSUPERUSTE);
    ui->perusteCombo->addItem("Käteiskuitti", KATEISLASKU);

    ui->toimitusDate->setDate( kp()->paivamaara() );
    ui->eraDate->setDate( kp()->paivamaara().addDays(14));

    model = new LaskuModel(this);
    model->lisaaRivi();

    ui->rivitView->setModel(model);
    ui->rivitView->setItemDelegateForColumn(LaskuModel::AHINTA, new EuroDelegaatti());
    ui->rivitView->setItemDelegateForColumn(LaskuModel::TILI, new TiliDelegaatti());
    ui->rivitView->setItemDelegateForColumn(LaskuModel::KOHDENNUS, new KohdennusDelegaatti());
    ui->rivitView->setItemDelegateForColumn(LaskuModel::BRUTTOSUMMA, new EuroDelegaatti());

    ui->rivitView->setColumnHidden( LaskuModel::ALV, !kp()->asetukset()->onko("AlvVelvollinen") );
    ui->rivitView->setColumnHidden( LaskuModel::KOHDENNUS, kp()->kohdennukset()->rowCount(QModelIndex()) < 2);

    connect( ui->lisaaNappi, SIGNAL(clicked(bool)), model, SLOT(lisaaRivi()));
    connect( ui->rivitView, SIGNAL(activated(QModelIndex)), this, SLOT(viewAktivoitu(QModelIndex)));
    connect( model, SIGNAL(summaMuuttunut(int)), this, SLOT(paivitaSumma(int)));

    ui->rivitView->horizontalHeader()->setSectionResizeMode(LaskuModel::NIMIKE, QHeaderView::Stretch);
}

LaskuDialogi::~LaskuDialogi()
{
    delete ui;
}

void LaskuDialogi::viewAktivoitu(QModelIndex indeksi)
{
    if( indeksi.column() == LaskuModel::ALV)
    {
        VeroDialogiValinta uusivero = VeroDialogi::veroDlg( indeksi.data(LaskuModel::AlvKoodiRooli).toInt(), indeksi.data(LaskuModel::AlvProsenttiRooli).toInt(), true );
        model->setData(indeksi, uusivero.verokoodi, LaskuModel::AlvKoodiRooli);
        model->setData(indeksi, uusivero.veroprosentti, LaskuModel::AlvProsenttiRooli);
    }
}

void LaskuDialogi::paivitaSumma(int summa)
{
    ui->summaLabel->setText( QString("%L1 €").arg(summa / 100.0,0,'f',2) );
}
