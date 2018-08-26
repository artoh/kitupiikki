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

#include <QSortFilterProxyModel>

#include "tilinavaus.h"
#include "tilinavausmodel.h"
#include "kirjaus/eurodelegaatti.h"

Tilinavaus::Tilinavaus(QWidget *parent) : MaaritysWidget(parent)
{
    ui = new Ui::Tilinavaus;
    ui->setupUi(this);

    model = new TilinavausModel();

    proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel(model);
    proxy->setFilterRole(TilinavausModel::KaytossaRooli);
    proxy->setFilterFixedString("1");

    ui->tiliView->setModel(proxy);

    ui->tiliView->setItemDelegateForColumn( TilinavausModel::SALDO, new EuroDelegaatti);

    ui->tiliView->resizeColumnsToContents();

    connect(model, SIGNAL(infoteksti(QString)), this, SLOT(naytaInfo(QString)));
    connect( ui->henkilostoSpin, SIGNAL(valueChanged(int)), this, SLOT(hlostoMuutos()));
    connect(ui->piiloCheck, SIGNAL(toggled(bool)), this, SLOT(naytaPiilotetut(bool)));
    connect(ui->tositeNappi, SIGNAL(clicked(bool)), this, SLOT(tosite()));

}

Tilinavaus::~Tilinavaus()
{
    delete ui;
}

void Tilinavaus::naytaInfo(const QString& info)
{
    ui->infoLabel->setText(info);
    emit tallennaKaytossa( onkoMuokattu());

    // Tositetta voi käyttää vain, jos ei tallentamatonta!
    ui->tositeNappi->setEnabled( !onkoMuokattu() );
}

void Tilinavaus::hlostoMuutos()
{
    // Tositetta voi käyttää vain, jos ei tallentamatonta!
    ui->tositeNappi->setEnabled( !onkoMuokattu() );
    emit tallennaKaytossa( onkoMuokattu());
}

void Tilinavaus::tosite()
{
    emit kp()->naytaTosite(0);
}

void Tilinavaus::naytaPiilotetut(bool naytetaanko)
{
    if( naytetaanko)
        proxy->setFilterFixedString("");
    else
        proxy->setFilterFixedString("1");
}

bool Tilinavaus::nollaa()
{
    model->lataa();
    ui->henkilostoSpin->setValue(kp()->tilikaudet()->tilikausiIndeksilla(0).json()->luku("Henkilosto"));
    ui->tositeNappi->setEnabled(true);
    emit tallennaKaytossa(onkoMuokattu());
    return true;
}

bool Tilinavaus::tallenna()
{
    // #40 Model tallennetaan vain, jos sitä on muokattu
    if( model->onkoMuokattu())
        model->tallenna();

    kp()->tilikaudet()->json(0)->set("Henkilosto", ui->henkilostoSpin->value());
    kp()->tilikaudet()->tallennaJSON();

    emit tallennaKaytossa(onkoMuokattu());

    // Tositetta voi käyttää vain, jos ei tallentamatonta!
    ui->tositeNappi->setEnabled( !onkoMuokattu() );

    return true;
}

bool Tilinavaus::onkoMuokattu()
{
    return model->onkoMuokattu() || kp()->tilikaudet()->tilikausiIndeksilla(0).json()->luku("Henkilosto") != ui->henkilostoSpin->value();
}
