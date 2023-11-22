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
#include "eranvalintadialog.h"
#include "ui_eranvalintadialog.h"
#include "eraproxymodel.h"
#include <QPushButton>
#include "db/kirjanpito.h"
#include <QSettings>
#include "eranvalintamodel.h"

EranValintaDialog::EranValintaDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EranValintaDialog),
    proxy_(new EraProxyModel(this))
{        
    ui->setupUi(this);
    ui->alkuDate->setDate(kp()->tilikaudet()->kirjanpitoAlkaa());
    ui->loppuDate->setDate(kp()->tilikaudet()->kirjanpitoLoppuu());
    paivitaSuodatus();

    connect( ui->alkuDate, &KpDateEdit::dateChanged,
             this, &EranValintaDialog::paivitaSuodatus);
    connect( ui->loppuDate, &KpDateEdit::dateChanged,
             this, &EranValintaDialog::paivitaSuodatus);
    connect( ui->suodatusEdit, &QLineEdit::textChanged,
             this, &EranValintaDialog::paivitaSuodatus);

    paivitaOk();

    restoreGeometry( kp()->settings()->value("EranValintaDlg").toByteArray());
}

EranValintaDialog::~EranValintaDialog()
{
    kp()->settings()->setValue("EranValintaDlg", saveGeometry());
    delete ui;
}

QVariantMap EranValintaDialog::valittu() const
{
    return ui->view->currentIndex().data(EranValintaModel::MapRooli).toMap();
}

void EranValintaDialog::paivitaSuodatus()
{
    proxy_->suodata( ui->alkuDate->date(),
                     ui->loppuDate->date(),
                     ui->suodatusEdit->text());
}

void EranValintaDialog::paivitaOk()
{
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
                ui->view->selectionModel() &&
                                                            ui->view->selectionModel()->selectedRows().count());
}

void EranValintaDialog::asetaNykyinen(int eraId)
{
    nykyinen_ = eraId;
    paivitaNykyinen();
}

void EranValintaDialog::asetaModel(QAbstractItemModel *model)
{
    proxy_->setSourceModel(model);
    proxy_->setDynamicSortFilter(true);
    ui->view->setModel(proxy_);
    connect( proxy_, &QAbstractItemModel::modelReset,
             this, &EranValintaDialog::paivitaNykyinen);
    proxy_->sort(0);

    connect( ui->view->selectionModel(), &QItemSelectionModel::selectionChanged,
             this, &EranValintaDialog::paivitaOk);
    connect( proxy_, &QSortFilterProxyModel::modelReset,
             this, &EranValintaDialog::paivitaNykyinen);

    paivitaNykyinen();
    ui->view->setFocus();
}

void EranValintaDialog::paivitaNykyinen()
{
    for(int i=0; i < ui->view->model()->rowCount(); i++) {
        if( ui->view->model()->index(i,0).data(EranValintaModel::IdRooli).toInt() == nykyinen_) {
            ui->view->selectRow(i);
            break;
        }
    }
}
