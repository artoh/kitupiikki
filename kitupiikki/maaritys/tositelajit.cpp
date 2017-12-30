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

#include "tositelajit.h"
#include "db/kirjanpito.h"
#include "tositelajidialogi.h"

Tositelajit::Tositelajit(QWidget *parent) : MaaritysWidget(parent)
{
    ui = new Ui::Tositelajit;
    ui->setupUi(this);

    model = new TositelajiModel( kp()->tietokanta(), this);

    proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel( model );
    proxy->setSortRole( TositelajiModel::TunnusRooli);

    ui->view->setModel(proxy);

    connect( ui->view->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
             this, SLOT(riviValittu(QModelIndex)));
    connect( ui->uusiNappi, SIGNAL(clicked(bool)), this, SLOT(uusi()));
    connect( ui->muokkaaNappi, SIGNAL(clicked(bool)), this, SLOT(muokkaa()));
    connect( ui->poistaNappi, SIGNAL(clicked(bool)), this, SLOT(poista()));

    connect( ui->view, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(muokkaa()));

}

Tositelajit::~Tositelajit()
{
    delete ui;
}

void Tositelajit::uusi()
{
    TositelajiDialogi dlg( model );
    dlg.exec();
    proxy->sort(0);
    emit tallennaKaytossa( onkoMuokattu() );
}

void Tositelajit::muokkaa()
{
    TositelajiDialogi dlg( model, proxy->mapToSource( ui->view->currentIndex()));
    dlg.exec();
    proxy->sort(0);
    emit tallennaKaytossa( onkoMuokattu());
}

void Tositelajit::poista()
{
    model->poistaRivi( proxy->mapToSource(ui->view->currentIndex()).row() );
    emit tallennaKaytossa( onkoMuokattu());
}

void Tositelajit::riviValittu(const QModelIndex &index)
{
    ui->muokkaaNappi->setEnabled( index.isValid());
    ui->poistaNappi->setEnabled( index.isValid() &&
                                 index.data(TositelajiModel::IdRooli).toInt() > 1 &&
                                 index.data(TositelajiModel::TositeMaaraRooli).toInt() == 0);

}

bool Tositelajit::tallenna()
{
    // Tallentaa nämä tietokantaan
    model->tallenna();
    // Lataa kirjanpidon modelin
    kp()->tositelajit()->lataa();
    return true;
}

bool Tositelajit::nollaa()
{
    model->lataa();

    ui->view->setColumnWidth(TositelajiModel::TUNNUS, ui->view->width() / 4 - 2 );
    ui->view->setColumnWidth(TositelajiModel::NIMI, ui->view->width() / 2);

    proxy->sort(0);

    return true;
}

bool Tositelajit::onkoMuokattu()
{
    return model->onkoMuokattu();
}
