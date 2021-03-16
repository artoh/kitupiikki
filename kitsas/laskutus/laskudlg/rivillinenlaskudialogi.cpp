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
#include "rivillinenlaskudialogi.h"
#include "model/tositerivit.h"
#include "ui_laskudialogi.h"

#include "laskutusverodelegaatti.h"
#include "kirjaus/eurodelegaatti.h"
#include "kirjaus/kohdennusdelegaatti.h"
#include "kirjaus/tilidelegaatti.h"
#include "kappaledelegaatti.h"
#include "yksikkodelegaatti.h"
#include "../tuotedialogi.h"

#include "laskurividialogi.h"

#include "db/kirjanpito.h"

#include <QSortFilterProxyModel>
#include <QMenu>

RivillinenLaskuDialogi::RivillinenLaskuDialogi(Tosite *tosite, QWidget *parent)
    : KantaLaskuDialogi(tosite, parent)
{
    alustaRiviTab();

    connect( tosite->rivit(), &TositeRivit::dataChanged, this, &RivillinenLaskuDialogi::paivitaSumma);
    connect( tosite->rivit(), &TositeRivit::rowsInserted, this, &RivillinenLaskuDialogi::paivitaSumma);
    connect( tosite->rivit(), &TositeRivit::rowsRemoved, this, &RivillinenLaskuDialogi::paivitaSumma);
    connect( tosite->rivit(), &TositeRivit::modelReset, this, &RivillinenLaskuDialogi::paivitaSumma);

    connect( ui->rivitView->selectionModel(), &QItemSelectionModel::currentRowChanged , this, &RivillinenLaskuDialogi::paivitaRiviNapit);

    connect( ui->riviLisatiedotNappi, &QPushButton::clicked, this, &RivillinenLaskuDialogi::rivinLisaTiedot);

    ui->tabWidget->removeTab( ui->tabWidget->indexOf( ui->tabWidget->findChild<QWidget*>("maksumuistutus") ) );
    paivitaSumma();
}

LaskuAlvCombo::AsiakasVeroLaji RivillinenLaskuDialogi::asiakasverolaji() const
{
    const QString& alvtunnus = asiakkaanAlvTunnus();

    if( alvtunnus.isEmpty())
        return LaskuAlvCombo::YKSITYINEN ;
    else if( alvtunnus.startsWith("FI"))
        return LaskuAlvCombo::KOTIMAA;
    else
        return LaskuAlvCombo::EU;
}

void RivillinenLaskuDialogi::tuotteidenKonteksiValikko(QPoint pos)
{
    QModelIndex index = ui->tuoteView->indexAt(pos);
    int tuoteid = index.data(TuoteModel::IdRooli).toInt();
    QVariantMap tuoteMap = index.data(TuoteModel::MapRooli).toMap();

    QMenu *menu = new QMenu(this);
    menu->addAction(QIcon(":/pic/muokkaa.png"), tr("Muokkaa"), [this, tuoteMap] {
        TuoteDialogi* dlg = new TuoteDialogi(this);
        dlg->muokkaa( tuoteMap );
    });
    menu->addAction(QIcon(":/pic/refresh.png"), tr("Päivitä luettelo"), [] {
        kp()->tuotteet()->lataa();
    });
    if( tuoteid )
        menu->popup( ui->tuoteView->viewport()->mapToGlobal(pos));
}

void RivillinenLaskuDialogi::rivinLisaTiedot()
{
    LaskuRiviDialogi dlg(this);

    int riviIndeksi = ui->rivitView->currentIndex().row();

    const TositeRivi& rivi = tosite()->rivit()->rivi( riviIndeksi );
    dlg.lataa( rivi, ui->laskuPvm->date(), asiakasverolaji(),
               maksutapa() == Lasku::ENNAKKOLASKU, kp() );

    if( dlg.exec() == QDialog::Accepted) {
        tosite()->rivit()->asetaRivi(riviIndeksi, dlg.rivi());
    }
}

void RivillinenLaskuDialogi::paivitaRiviNapit()
{
    const QModelIndex& index = ui->rivitView->currentIndex();

    ui->lisaaRiviNappi->setEnabled( index.isValid() );
    ui->poistaRiviNappi->setEnabled( index.isValid() );
}

void RivillinenLaskuDialogi::tositteelle()
{
    KantaLaskuDialogi::tositteelle();
    tosite()->lasku().setSumma( tosite()->rivit()->yhteensa() );
}

void RivillinenLaskuDialogi::alustaRiviTab()
{
    QSortFilterProxyModel *proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel( kp()->tuotteet() );

    ui->tuoteView->setModel(proxy);
    proxy->setSortLocaleAware(true);
    proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    ui->tuoteView->sortByColumn(TuoteModel::NIMIKE, Qt::AscendingOrder);
    ui->tuoteView->horizontalHeader()->setSectionResizeMode(TuoteModel::NIMIKE, QHeaderView::Stretch);
    ui->tuoteView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect( ui->tuoteView, &QTableView::customContextMenuRequested,
             this, &RivillinenLaskuDialogi::tuotteidenKonteksiValikko);


    ui->rivitView->setModel(tosite()->rivit());

    ui->rivitView->horizontalHeader()->setSectionResizeMode(TositeRivit::NIMIKE, QHeaderView::Stretch);
    ui->rivitView->setItemDelegateForColumn(TositeRivit::AHINTA, new EuroDelegaatti());
    ui->rivitView->setItemDelegateForColumn(TositeRivit::TILI, new TiliDelegaatti());
    ui->rivitView->setItemDelegateForColumn(TositeRivit::MAARA, new KappaleDelegaatti);
    ui->rivitView->setItemDelegateForColumn(TositeRivit::YKSIKKO, new YksikkoDelegaatti);

    KohdennusDelegaatti *kohdennusDelegaatti = new KohdennusDelegaatti(this);
    kohdennusDelegaatti->asetaKohdennusPaiva(ui->toimitusDate->date());
    ui->rivitView->setItemDelegateForColumn(TositeRivit::KOHDENNUS, kohdennusDelegaatti );

    connect( ui->toimitusDate , SIGNAL(dateChanged(QDate)), kohdennusDelegaatti, SLOT(asetaKohdennusPaiva(QDate)));
    connect( ui->tuoteFiltterinEditori, &QLineEdit::textChanged, proxy, &QSortFilterProxyModel::setFilterFixedString);

    ui->rivitView->setItemDelegateForColumn(TositeRivit::BRUTTOSUMMA, new EuroDelegaatti());
    ui->rivitView->setItemDelegateForColumn(TositeRivit::ALV, new LaskutusVeroDelegaatti(this));

    ui->rivitView->setColumnHidden( TositeRivit::ALV, !kp()->asetukset()->onko("AlvVelvollinen") );
    ui->rivitView->setColumnHidden( TositeRivit::KOHDENNUS, !kp()->kohdennukset()->kohdennuksia());

    connect( ui->uusituoteNappi, &QPushButton::clicked, [this] { (new TuoteDialogi(this))->uusi(); } );
    connect( ui->lisaaRiviNappi, &QPushButton::clicked, [this] { this->tosite()->rivit()->lisaaRivi();} );
    connect( ui->poistaRiviNappi, &QPushButton::clicked, [this] {
        if( this->ui->rivitView->currentIndex().isValid())
                this->tosite()->rivit()->poistaRivi( ui->rivitView->currentIndex().row());
    });

    ui->splitter->setStretchFactor(0,1);
    ui->splitter->setStretchFactor(1,3);

    connect( ui->tuoteView, &QTableView::clicked, [this] (const QModelIndex& index)
        { this->tosite()->rivit()->lisaaRivi( index.data(TuoteModel::TuoteMapRooli).toMap() ); }  );
}

void RivillinenLaskuDialogi::paivitaSumma()
{
    ui->summaLabel->setText( QString("%L1 €").arg( tosite()->rivit()->yhteensa(),0,'f',2) );
}
