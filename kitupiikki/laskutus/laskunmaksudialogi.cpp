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

#include "laskunmaksudialogi.h"
#include "ui_laskunmaksudialogi.h"
#include "db/jsonkentta.h"
#include "db/kirjanpito.h"

#include <QSqlQuery>

LaskunMaksuDialogi::LaskunMaksuDialogi(QWidget *parent, VientiModel *vientiModel) :
    QDialog(parent),
    viennit(vientiModel),
    ui(new Ui::LaskunMaksuDialogi)
{
    ui->setupUi(this);

    laskut = new LaskulistaModel(this);
    laskut->paivita( LaskulistaModel::AVOIMET);
    ui->laskutView->hideColumn(LaskulistaModel::TOSITE);

    proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel( laskut );
    proxy->setDynamicSortFilter(true);
    proxy->setSortRole(Qt::EditRole);

    ui->laskutView->setModel(proxy);
    ui->laskutView->setSelectionBehavior(QTableView::SelectRows);
    ui->laskutView->setSelectionMode(QTableView::SingleSelection);

    ui->laskutView->setModel(proxy);
    ui->laskutView->setSortingEnabled(true);
    ui->laskutView->horizontalHeader()->setStretchLastSection(true);

    ui->pvmEdit->setDate( kp()->paivamaara() );

    connect( ui->laskutView->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)), this, SLOT(valintaMuuttuu()));
    connect(ui->kirjaaNappi, SIGNAL(clicked(bool)), this, SLOT(kirjaa()));
}

LaskunMaksuDialogi::~LaskunMaksuDialogi()
{
    delete ui;
}

void LaskunMaksuDialogi::valintaMuuttuu()
{
    QModelIndex index = ui->laskutView->currentIndex();
    ui->euroSpin->setValue( index.data(LaskulistaModel::AvoinnaRooli).toDouble() / 100.0 );
}

void LaskunMaksuDialogi::kirjaa()
{
    QModelIndex index = ui->laskutView->currentIndex();
    JsonKentta json( index.data(LaskulistaModel::JsonRooli).toByteArray() );

    QString selite = QString("%1 [%2]").arg(index.data(LaskulistaModel::AsiakasRooli).toString())
            .arg(index.data(LaskulistaModel::IdRooli).toInt());

    if( json.luku("Kirjausperuste") == LaskuModel::MAKSUPERUSTE )
    {

    }
    else
    {
        // Tehdään saatavan vähennys
        VientiRivi saatavaRivi;
        saatavaRivi.tili = kp()->tilit()->tiliNumerolla( json.luku("Saatavatili") );
        saatavaRivi.kreditSnt = int( ui->euroSpin->value() * 100 );
        saatavaRivi.eraId = json.luku("TaseEra");
        saatavaRivi.pvm = ui->pvmEdit->date();
        saatavaRivi.selite = selite;

        // Sekä rahakirjaus
        VientiRivi rahaRivi;
        rahaRivi.pvm = saatavaRivi.pvm;
        rahaRivi.debetSnt = saatavaRivi.kreditSnt;
        rahaRivi.selite = selite;
        rahaRivi.tili = kp()->tilit()->tiliNumerolla( ui->tiliEdit->valittuTilinumero() );

        if( viennit )
        {
            viennit->lisaaVienti(saatavaRivi);
            viennit->lisaaVienti(rahaRivi);
        }

    }
    QSqlQuery kysely;
    kysely.exec(QString("UPDATE lasku SET avoinSnt = avoinSnt - %1 WHERE id=%2")
                .arg(int(ui->euroSpin->value() * 100)).arg( index.data(LaskulistaModel::IdRooli).toInt() ) );

    laskut->paivita(LaskulistaModel::AVOIMET);
}
