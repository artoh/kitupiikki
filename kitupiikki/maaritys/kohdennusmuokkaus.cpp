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

#include "kohdennusmuokkaus.h"
#include "db/kirjanpito.h"
#include "kohdennusdialog.h"

#include <QDebug>

KohdennusMuokkaus::KohdennusMuokkaus(QWidget *parent) :
    MaaritysWidget(parent)
{
    ui = new Ui::Kohdennukset;
    ui->setupUi(this);

    model = new KohdennusModel( kp()->tietokanta(), this);
    proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel(model);
    proxy->setSortRole(KohdennusModel::NimiRooli);
    proxy->sort(0);

    proxy->setFilterRole(KohdennusModel::IdRooli);
    proxy->setFilterRegExp("^[^0].*");
    // Ei näytetä tässä luettelossa Yleistä ei id 0

    ui->view->setModel(proxy);

    connect( ui->view->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
             this, SLOT(riviValittu(QModelIndex)));
    connect( ui->lisaaNappi, SIGNAL(clicked(bool)), this, SLOT(uusi()));
    connect( ui->muokkaaNappi, SIGNAL(clicked(bool)), this, SLOT(muokkaa()));
    connect(ui->poistaNappi, SIGNAL(clicked(bool)), this, SLOT(poista()));
    connect( ui->view, SIGNAL(clicked(QModelIndex)), this, SLOT(muokkaa()));

}

KohdennusMuokkaus::~KohdennusMuokkaus()
{
    delete ui;
}

bool KohdennusMuokkaus::nollaa()
{
    model->lataa();
    proxy->sort(0);

    // Sarakkeiden leveydet
    ui->view->setColumnWidth(0, (ui->view->width()-10)/2 );
    ui->view->setColumnWidth(1, (ui->view->width()-10)/4 );
    ui->view->setColumnWidth(2, (ui->view->width()-10)/4 );
    ui->view->horizontalHeader()->setStretchLastSection(true);

    return true;
}

bool KohdennusMuokkaus::tallenna()
{
    model->tallenna();
    kp()->kohdennukset()->lataa();

    return true;
}

bool KohdennusMuokkaus::onkoMuokattu()
{
    return model->onkoMuokattu();
}

void KohdennusMuokkaus::uusi()
{
    KohdennusDialog dlg(model);
    dlg.exec();
    proxy->sort(0);
    emit tallennaKaytossa( onkoMuokattu() );
}

void KohdennusMuokkaus::muokkaa()
{
    KohdennusDialog dlg( model, proxy->mapToSource( ui->view->currentIndex()));
    dlg.exec();
    proxy->sort(0);
    emit tallennaKaytossa( onkoMuokattu() );
}

void KohdennusMuokkaus::poista()
{
    model->poistaRivi( proxy->mapToSource( ui->view->currentIndex()).row());
    emit tallennaKaytossa( onkoMuokattu() );
}



void KohdennusMuokkaus::riviValittu(const QModelIndex &index)
{
    ui->muokkaaNappi->setEnabled( index.isValid());
    ui->poistaNappi->setEnabled( index.isValid() &&
                                 index.data(KohdennusModel::VientejaRooli).toInt() == 0);
    // Saa poistaa, jos ei ole vientejä tai id=-1 eli ei vielä tallennettu
    // Kohdentamattomiin käytetään id:tä 0
}
