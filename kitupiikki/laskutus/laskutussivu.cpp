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
#include "db/kirjanpito.h"

LaskutusSivu::LaskutusSivu() :
    ui(new Ui::Laskutus)
{
    ui->setupUi(this);
    ui->suodatusTab->addTab("Kaikki");
    ui->suodatusTab->addTab("Avoimet");
    ui->suodatusTab->addTab("Erääntyneet");

    connect( ui->mistaDate, SIGNAL(dateChanged(QDate)), this, SLOT(paivita()));
    connect( ui->mihinDate, SIGNAL(dateChanged(QDate)), this, SLOT(paivita()));
    connect( ui->suodatusEdit, SIGNAL(textChanged(QString)), this, SLOT(suodata()));

    connect(ui->uusiNappi, SIGNAL(clicked(bool)), this, SLOT(uusiLasku()) );
    connect(ui->suodatusTab, SIGNAL(currentChanged(int)), this, SLOT(paivita()));
    connect(ui->naytaNappi, SIGNAL(clicked(bool)), this, SLOT(nayta()));
    connect(ui->hyvitysNappi, SIGNAL(clicked(bool)), this, SLOT(hyvitysLasku()));

    model = new LaskutModel(this);
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
    if( ui->mistaDate->date().daysTo( kp()->tilikaudet()->kirjanpitoAlkaa() ) > 0)
    {
        ui->mistaDate->setDateRange( kp()->tilikaudet()->kirjanpitoAlkaa(), kp()->tilikaudet()->kirjanpitoLoppuu());
        ui->mistaDate->setDate( kp()->tilikaudet()->kirjanpitoAlkaa());

        ui->mihinDate->setDateRange( kp()->tilikaudet()->kirjanpitoAlkaa(), kp()->tilikaudet()->kirjanpitoLoppuu());
        ui->mihinDate->setDate(kp()->tilikaudet()->kirjanpitoLoppuu());
    }

    paivita();
}

bool LaskutusSivu::poistuSivulta()
{
    return true;
}

void LaskutusSivu::uusiLasku()
{
    LaskuDialogi *dlg = new LaskuDialogi(this);
    dlg->exec();
    paivita();
    dlg->deleteLater();
}

void LaskutusSivu::hyvitysLasku()
{
    QModelIndex index = proxy->mapToSource( ui->laskutView->currentIndex() );
    if( index.isValid() )
    {
        LaskuDialogi *dlg = new LaskuDialogi(this, model->laskunTiedot( index.row() ) );
        dlg->exec();
        paivita();
        dlg->deleteLater();
    }

}

void LaskutusSivu::paivita()
{
    model->paivita( ui->suodatusTab->currentIndex(), ui->mistaDate->date(), ui->mihinDate->date() );
    valintaMuuttuu();
    return;
}

void LaskutusSivu::suodata()
{
    QString suodatin = ui->suodatusEdit->text();
    if( suodatin.toInt())
        proxy->setFilterKeyColumn(LaskutModel::NUMERO);
    else
        proxy->setFilterKeyColumn(LaskutModel::ASIAKAS);
    proxy->setFilterFixedString( suodatin );
}

void LaskutusSivu::nayta()
{
    QModelIndex index =  ui->laskutView->currentIndex();
    QString liite = index.data(LaskutModel::LiiteRooli).toString();
    if(!liite.isEmpty())
    {
        QDesktopServices::openUrl( QUrl( LiiteModel::liiteNimella(liite) ) );
    }
}

void LaskutusSivu::valintaMuuttuu()
{
    ui->naytaNappi->setEnabled( ui->laskutView->currentIndex().isValid());
    ui->hyvitysNappi->setEnabled( ui->laskutView->currentIndex().isValid() &&
                                  !ui->laskutView->currentIndex().data(LaskutModel::HyvitysLaskuModel).toInt());
}
