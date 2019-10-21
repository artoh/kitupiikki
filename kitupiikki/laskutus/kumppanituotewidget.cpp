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
#include "kumppanituotewidget.h"
#include "ui_kumppanituotewidget.h"

#include "asiakkaatmodel.h"
#include "tuotemodel.h"
#include <QSortFilterProxyModel>

#include "rekisteri/asiakastoimittajadlg.h"
#include "tuotedialogi.h"

#include "db/kirjanpito.h"

#include <QDebug>

KumppaniTuoteWidget::KumppaniTuoteWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::KumppaniTuoteWidget),
    proxy_(new QSortFilterProxyModel),
    asiakkaat_( new AsiakkaatModel(this)),
    tuotteet_( new TuoteModel(this))
{
    ui->setupUi(this);

    proxy_->setDynamicSortFilter(true);
    proxy_->setFilterCaseSensitivity(Qt::CaseInsensitive);
    ui->view->setSortingEnabled(true);
    ui->view->setModel(proxy_);

    connect( ui->view->selectionModel(), &QItemSelectionModel::selectionChanged,
             this, &KumppaniTuoteWidget::ilmoitaValinta);

    connect( ui->uusiNappi, &QPushButton::clicked, this, &KumppaniTuoteWidget::uusi);
    connect( ui->muokkaaNappi, &QPushButton::clicked, this, &KumppaniTuoteWidget::muokkaa);
    connect( ui->poistaNappi, &QPushButton::clicked, this, &KumppaniTuoteWidget::poista);
}

KumppaniTuoteWidget::~KumppaniTuoteWidget()
{
    delete ui;
}

void KumppaniTuoteWidget::nayta(int valilehti)
{
    valilehti_ = valilehti;
    paivita();
}

void KumppaniTuoteWidget::suodata(const QString &suodatus)
{
    proxy_->setFilterFixedString( suodatus );
}

void KumppaniTuoteWidget::ilmoitaValinta()
{
    if( ui->view->selectionModel()->selectedRows(0).count() )
        emit kumppaniValittu(  ui->view->selectionModel()->selectedRows(0).value(0).data().toString() );
}

void KumppaniTuoteWidget::uusi()
{
    if( valilehti_ == TUOTTEET ) {
        TuoteDialogi *dlg = new TuoteDialogi(this);
        connect( dlg, &TuoteDialogi::tuoteTallennettu, this, &KumppaniTuoteWidget::paivita);
        dlg->uusi();
    } else {
        AsiakasToimittajaDlg *dlg = new AsiakasToimittajaDlg(this);
        connect( dlg, &AsiakasToimittajaDlg::tallennettu, this, &KumppaniTuoteWidget::paivita);
        dlg->uusi();
    }
}

void KumppaniTuoteWidget::muokkaa()
{
    if( valilehti_ == TUOTTEET) {
        TuoteDialogi *dlg = new TuoteDialogi(this);
        dlg->muokkaa( ui->view->currentIndex().data(TuoteModel::MapRooli).toMap()  );
        connect( dlg, &TuoteDialogi::tuoteTallennettu, this, &KumppaniTuoteWidget::paivita);
    } else {
        AsiakasToimittajaDlg *dlg = new AsiakasToimittajaDlg(this);
        dlg->muokkaa( ui->view->currentIndex().data(AsiakkaatModel::IdRooli).toInt() );
        connect( dlg, &AsiakasToimittajaDlg::tallennettu, this, &KumppaniTuoteWidget::paivita);
    }
}

void KumppaniTuoteWidget::poista()
{
    if( valilehti_ == TUOTTEET) {
        int tuoteid = ui->view->currentIndex().data(TuoteModel::IdRooli).toInt();
        if( tuoteid ) {
            KpKysely *kysely = kpk(QString("/tuotteet/%1").arg(tuoteid), KpKysely::DELETE );
            connect( kysely, &KpKysely::vastaus, this, &KumppaniTuoteWidget::paivita);
            kysely->kysy();
        }
    } else {
        int kid = ui->view->currentIndex().data(AsiakkaatModel::IdRooli).toInt();
        if( kid ) {
            KpKysely *kysely = kpk(QString("/kumppanit/%1").arg(kid), KpKysely::DELETE );
            connect( kysely, &KpKysely::vastaus, this, &KumppaniTuoteWidget::paivita);
            kysely->kysy();
        }
    }
}

void KumppaniTuoteWidget::paivita()
{
    if( valilehti_ == TUOTTEET)
        proxy_->setSourceModel( tuotteet_ );
    else
        proxy_->setSourceModel( asiakkaat_);

    if( valilehti_ == REKISTERI)
        asiakkaat_->paivita(AsiakkaatModel::REKISTERI);
    else if( valilehti_ == ASIAKKAAT )
        asiakkaat_->paivita(AsiakkaatModel::ASIAKKAAT);
    else if( valilehti_ == TOIMITTAJAT)
        asiakkaat_->paivita(AsiakkaatModel::TOIMITTAJAT);
    else if( valilehti_ == TUOTTEET)
        tuotteet_->lataa();

    ui->view->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
 }

