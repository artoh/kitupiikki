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
#include <QScrollBar>

#include "tilinavaus.h"
#include "tilinavausmodel.h"
#include "kirjaus/eurodelegaatti.h"
#include "avauseradlg.h"

Tilinavaus::Tilinavaus(QWidget *parent) : MaaritysWidget(parent)
{
    ui = new Ui::Tilinavaus;
    ui->setupUi(this);

    model = new TilinavausModel();

    proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel(model);
    proxy->setFilterRole(TilinavausModel::KaytossaRooli);
    proxy->setFilterFixedString("1");


    suodatus = new QSortFilterProxyModel(this);
    suodatus->setSourceModel(proxy);

    ui->tiliView->setModel(suodatus);
    ui->siirryEdit->setValidator(new QIntValidator(this));

    ui->tiliView->setItemDelegateForColumn( TilinavausModel::SALDO, new EuroDelegaatti);

    ui->tiliView->horizontalHeader()->setSectionResizeMode(TilinavausModel::NIMI, QHeaderView::Stretch);

    connect( ui->henkilostoSpin, SIGNAL(valueChanged(int)), this, SLOT(hlostoMuutos()));

    connect( ui->kaikkiNappi, &QPushButton::clicked, this, &Tilinavaus::naytaPiilotetut);
    connect( ui->kirjauksetNappi, &QPushButton::clicked, this, &Tilinavaus::naytaVainKirjaukset);

    connect( ui->etsiEdit, &QLineEdit::textEdited, this, &Tilinavaus::suodata);
    connect( ui->tiliView, &QTableView::activated, this, &Tilinavaus::erittely);
    connect( ui->tiliView, &QTableView::clicked, this, &Tilinavaus::erittely);
    connect( ui->siirryEdit, &QLineEdit::textEdited, this, &Tilinavaus::siirry);

    connect( model, &TilinavausModel::tilasto, this, &Tilinavaus::info);
}

Tilinavaus::~Tilinavaus()
{
    delete ui;
}


void Tilinavaus::hlostoMuutos()
{
    // Tositetta voi käyttää vain, jos ei tallentamatonta!
    emit tallennaKaytossa( onkoMuokattu());
}


void Tilinavaus::naytaPiilotetut(bool naytetaanko)
{
    if( naytetaanko) {
        proxy->setFilterFixedString("0");
        ui->kirjauksetNappi->setChecked(false);
    } else
        proxy->setFilterFixedString("1");
}

void Tilinavaus::naytaVainKirjaukset(bool naytetaanko)
{
    if( naytetaanko ) {
        ui->kaikkiNappi->setChecked(false);
        proxy->setFilterFixedString("2");
    } else {
        proxy->setFilterFixedString("1");
    }
}

void Tilinavaus::suodata(const QString &suodatusteksti)
{
    if( suodatusteksti.toInt())
        suodatus->setFilterRole(TilinavausModel::NumeroRooli);
    else
        suodatus->setFilterRole(TilinavausModel::NimiRooli);
    suodatus->setFilterFixedString(suodatusteksti);

}

void Tilinavaus::erittely(const QModelIndex &index)
{
    if( index.data(TilinavausModel::ErittelyRooli).toInt() != TilinavausModel::EI_ERITTELYA) {
        int tili = index.data(TilinavausModel::NumeroRooli).toInt();
        AvausEraDlg dlg(tili,
                        index.data(TilinavausModel::ErittelyRooli).toInt() == TilinavausModel::KOHDENNUKSET,
                        model->erat(tili),
                        this
                        );
        if( dlg.exec() == QDialog::Accepted )
            model->asetaErat(tili, dlg.erat());
    }
}

void Tilinavaus::info(qlonglong vastaavaa, qlonglong vastattavaa, qlonglong tulos)
{
    emit tallennaKaytossa(onkoMuokattu());

    ui->infoLabel->setText(QString("Vastaavaa \t%L1 € \t\tTulos\t %L2 € \nVastattavaa \t%L3 €")
                           .arg( ( vastaavaa / 100.0 ), 10,'f',2)
                           .arg( ( tulos / 100.0 ), 10,'f',2)
                           .arg( ( vastattavaa / 100.0 ), 10,'f',2) );

    ui->poikkeusLabel->clear();
    if( vastaavaa != vastattavaa)
        ui->poikkeusLabel->setText(QString("%L1 €").arg( ( qAbs(vastaavaa-vastattavaa) / 100.0 ), 10,'f',2));
}

void Tilinavaus::siirry(const QString &minne)
{
    if( !minne.isEmpty()) {
        for(int i=0; i < ui->tiliView->model()->rowCount(); i++) {
            QModelIndex index = ui->tiliView->model()->index(i,0);

            if( QString::number(index.data(TiliModel::NroRooli).toInt()).startsWith(minne) ||
                index.data(TiliModel::NimiRooli).toString().startsWith(minne))
            {
                // Valitsee tämän rivin
                ui->tiliView->setCurrentIndex(index);
                // Ja scrollaa rivin näkyviin
                ui->tiliView->verticalScrollBar()->setSliderPosition(i);
                return;
            }
        }
    }
}

bool Tilinavaus::nollaa()
{
    model->lataa();
    ui->henkilostoSpin->setValue(kp()->tilikaudet()->tilikausiIndeksilla(0).luku("Henkilosto"));
    emit tallennaKaytossa(onkoMuokattu());
    return true;
}

bool Tilinavaus::tallenna()
{
    // #40 Model tallennetaan vain, jos sitä on muokattu
    if( model->onkoMuokattu())
        model->tallenna();

    kp()->tilikaudet()->viiteIndeksilla(0).set("Henkilosto", ui->henkilostoSpin->value());
    kp()->tilikaudet()->viiteIndeksilla(0).tallenna();

    emit tallennaKaytossa(onkoMuokattu());

    return true;
}

bool Tilinavaus::onkoMuokattu()
{
    return model->onkoMuokattu() ||
          ui->henkilostoSpin->value() != kp()->tilikaudet()->viiteIndeksilla(0).luku("Henkilosto");
}
