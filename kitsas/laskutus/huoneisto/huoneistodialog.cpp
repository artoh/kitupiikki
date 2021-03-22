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
#include "huoneistodialog.h"
#include "ui_huoneistodialog.h"

#include "../laskudlg/kappaledelegaatti.h"
#include "huoneisto.h"

#include "db/kirjanpito.h"
#include <QSortFilterProxyModel>

HuoneistoDialog::HuoneistoDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HuoneistoDialog),
    proxy_(new QSortFilterProxyModel(this))
{
    ui->setupUi(this);

    ui->laskuView->setModel(huoneisto_.laskutus());
    ui->laskuView->setItemDelegateForColumn( HuoneistoLaskutusModel::MAARA, new KappaleDelegaatti );

    proxy_->setSourceModel(kp()->tuotteet());
    proxy_->setSortRole(Qt::DisplayRole);
    proxy_->sort(TuoteModel::NIMIKE);
    proxy_->setFilterRole(Qt::DisplayRole);
    proxy_->setFilterKeyColumn(TuoteModel::NIMIKE);
    ui->tuoteView->setModel(proxy_);

    connect( &huoneisto_, &Huoneisto::ladattu, this, &HuoneistoDialog::naytolle);
    connect(&huoneisto_, &Huoneisto::tallennettu, this, &HuoneistoDialog::tallennettu);

    connect( ui->tuoteSuodatus, &QLineEdit::textChanged, proxy_, &QSortFilterProxyModel::setFilterFixedString);
    connect( ui->tuoteView, &QTableView::clicked, this, &HuoneistoDialog::lisaaTuote);
    connect( ui->laskuView->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &HuoneistoDialog::valintaMuuttui);
    connect( ui->poistaNappi, &QPushButton::clicked, this, &HuoneistoDialog::poistaRivi);
}

HuoneistoDialog::~HuoneistoDialog()
{
    delete ui;
}

void HuoneistoDialog::lataa(int id)
{
    huoneisto_.lataa(id);
}

void HuoneistoDialog::accept()
{
    huoneisto_.setNimi( ui->tunnusEdit->text() );
    huoneisto_.setAsiakas( ui->asiakas->id() );
    huoneisto_.setMuistiinpanot( ui->muistiinpanoEdit->toPlainText());

    huoneisto_.tallenna();
}

void HuoneistoDialog::naytolle()
{
    ui->tunnusEdit->setText( huoneisto_.nimi() );
    ui->asiakas->set( huoneisto_.asiakas() );
    ui->muistiinpanoEdit->setPlainText( huoneisto_.muistiinpanot());
}

void HuoneistoDialog::tallennettu()
{
    QDialog::accept();
}

void HuoneistoDialog::lisaaTuote()
{
    int tuote = ui->tuoteView->currentIndex().data(TuoteModel::IdRooli).toInt();
    huoneisto_.laskutus()->lisaaTuote(tuote);
}

void HuoneistoDialog::valintaMuuttui()
{
    ui->poistaNappi->setEnabled( ui->laskuView->currentIndex().isValid() );
}

void HuoneistoDialog::poistaRivi()
{
    int rivi = proxy_->mapToSource( ui->laskuView->currentIndex() ).row();
    huoneisto_.laskutus()->poistaRivi(rivi);
}
