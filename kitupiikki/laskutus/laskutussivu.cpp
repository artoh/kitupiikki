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

#include <QSqlQueryModel>
#include <QDesktopServices>
#include <QUrl>

#include "laskutussivu.h"
#include "laskudialogi.h"
#include "db/liitemodel.h"

#include "kirjaus/eurodelegaatti.h"

LaskutusSivu::LaskutusSivu() :
    ui(new Ui::Laskutus)
{
    ui->setupUi(this);
    ui->suodatusTab->addTab("Kaikki");
    ui->suodatusTab->addTab("Avoimet");
    ui->suodatusTab->addTab("Erääntyneet");

    connect(ui->uusiNappi, SIGNAL(clicked(bool)), this, SLOT(uusiLasku()) );
    connect(ui->suodatusTab, SIGNAL(currentChanged(int)), this, SLOT(paivita()));
    connect(ui->naytaNappi, SIGNAL(clicked(bool)), this, SLOT(nayta()));

    model = new LaskulistaModel(this);
    proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel(model);
    proxy->setDynamicSortFilter(true);
    proxy->setSortRole(Qt::EditRole);

    ui->laskutView->setSelectionBehavior(QTableView::SelectRows);
    ui->laskutView->setSelectionMode(QTableView::SingleSelection);

    ui->laskutView->setModel(proxy);
    ui->laskutView->setSortingEnabled(true);
    ui->laskutView->horizontalHeader()->setStretchLastSection(true);

    connect(ui->laskutView->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)), this, SLOT(valintaMuuttuu()));


}

void LaskutusSivu::siirrySivulle()
{
    paivita();
}

bool LaskutusSivu::poistuSivulta()
{
    return true;
}

void LaskutusSivu::uusiLasku()
{
    LaskuDialogi dlg;
    dlg.exec();
    paivita();
}

void LaskutusSivu::paivita()
{
    model->paivita( ui->suodatusTab->currentIndex() );
    valintaMuuttuu();
    return;
}

void LaskutusSivu::nayta()
{
    QModelIndex index = ui->laskutView->currentIndex();
    int tositeId = index.data(LaskulistaModel::TositeRooli).toInt();
    if(tositeId)
    {
        QDesktopServices::openUrl( QUrl( LiiteModel::liitePolulla(tositeId, 1) ) );
    }
}

void LaskutusSivu::valintaMuuttuu()
{
    ui->naytaNappi->setEnabled( ui->laskutView->currentIndex().isValid());
}
