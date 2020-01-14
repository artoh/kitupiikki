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
#include "maksutapamuokkaus.h"
#include "ui_maksutapamuokkaus.h"

#include "model/maksutapamodel.h"

#include "maksutapamuokkausdlg.h"

#include <QDebug>

MaksutapaMuokkaus::MaksutapaMuokkaus(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MaksutapaMuokkaus)
{
    ui->setupUi(this);

    connect( ui->lisaaNappi, &QPushButton::clicked, this, &MaksutapaMuokkaus::uusi);
    connect( ui->muokkaaNappi, &QPushButton::clicked, this, &MaksutapaMuokkaus::muokkaa);
    connect( ui->poistaNappi, &QPushButton::clicked, this, &MaksutapaMuokkaus::poista);
    connect( ui->view, &QTableView::clicked, this, &MaksutapaMuokkaus::paivitaNapit);
    paivitaNapit();
}

MaksutapaMuokkaus::~MaksutapaMuokkaus()
{
    delete ui;
}

void MaksutapaMuokkaus::setModel(MaksutapaModel *model)
{
    model_ = model;
    ui->view->setModel(model);
    ui->view->horizontalHeader()->setSectionResizeMode(MaksutapaModel::NIMI, QHeaderView::Stretch);
    ui->view->selectRow(0);
}

void MaksutapaMuokkaus::uusi()
{
    MaksutapaMuokkausDlg dlg(this);
    QVariantMap map = dlg.muokkaa();
    if( map.contains("TILI"))
        model_->lisaaRivi( ui->view->currentIndex().isValid() ?
                           ui->view->currentIndex().row() :
                           model_->rowCount()-1, map );
}

void MaksutapaMuokkaus::muokkaa()
{
    if( ui->view->currentIndex().isValid()) {
        MaksutapaMuokkausDlg dlg(this);
        QVariantMap muokattu = dlg.muokkaa( model_->rivi( ui->view->currentIndex().row()  ) );
        if( muokattu.contains("TILI"))
            model_->muutaRivi( ui->view->currentIndex().row(), muokattu);
    }
}

void MaksutapaMuokkaus::poista()
{
    if( ui->view->currentIndex().isValid())
        model_->poistaRivi(ui->view->currentIndex().row());
}

void MaksutapaMuokkaus::paivitaNapit()
{
    bool kaytossa = ui->view->currentIndex().isValid() && ui->view->currentIndex().row() < ui->view->model()->rowCount()-1;
    ui->muokkaaNappi->setEnabled(kaytossa);
    ui->poistaNappi->setEnabled(kaytossa);
}
