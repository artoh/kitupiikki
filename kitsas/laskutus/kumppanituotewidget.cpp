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
#include "rekisteri/ryhmatmodel.h"
#include "tuotedialogi.h"

#include "db/kirjanpito.h"

#include <QDebug>
#include <QInputDialog>
#include <QMessageBox>

KumppaniTuoteWidget::KumppaniTuoteWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::KumppaniTuoteWidget),
    proxy_(new QSortFilterProxyModel(this)),
    asiakkaat_( new AsiakkaatModel(this))
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
    connect( ui->view, &QTableView::doubleClicked, this, &KumppaniTuoteWidget::muokkaa);

    ui->muokkaaNappi->setEnabled(false);
    ui->poistaNappi->setEnabled(false);

    // Tuonti ja vienti odottaa aikaa tulevaa ...
    ui->tuoNappi->setVisible(false);
    ui->VieNappi->setVisible(false);

}

KumppaniTuoteWidget::~KumppaniTuoteWidget()
{
    delete ui;
}

void KumppaniTuoteWidget::nayta(int valilehti)
{
    valilehti_ = valilehti;

    ui->tuoNappi->setVisible( valilehti != RYHMAT);
    ui->VieNappi->setVisible( valilehti != RYHMAT);

    paivita();
}

void KumppaniTuoteWidget::suodata(const QString &suodatus)
{
    proxy_->setFilterFixedString( suodatus );
}

void KumppaniTuoteWidget::suodataRyhma(int ryhma)
{
    ryhma_ = ryhma;
    paivita();
}

void KumppaniTuoteWidget::ilmoitaValinta()
{    
    bool napitkaytossa = true;
    if( ui->view->selectionModel()->selectedRows(0).count() ) {        
        if( valilehti_ == RYHMAT) {
            emit ryhmaValittu( ui->view->selectionModel()->selectedRows(0).value(0).data(RyhmatModel::IdRooli).toInt());
            napitkaytossa = ui->view->selectionModel()->selectedRows(0).value(0).row() > 0;
        } else {
            emit kumppaniValittu(  ui->view->selectionModel()->selectedRows(0).value(0).data().toString() );
        }
    } else {
        if( valilehti_ == RYHMAT)
            emit ryhmaValittu(0);
        else
            emit kumppaniValittu("");
        napitkaytossa = false;
    }
    ui->muokkaaNappi->setEnabled( napitkaytossa );
    ui->poistaNappi->setEnabled( napitkaytossa );
}

void KumppaniTuoteWidget::uusi()
{
    if( valilehti_ == TUOTTEET ) {
        TuoteDialogi *dlg = new TuoteDialogi(this);
        connect( dlg, &TuoteDialogi::tuoteTallennettu, kp()->tuotteet(), &TuoteModel::lataa);
        dlg->uusi();
    } else if( valilehti_ == RYHMAT) {
        QString nimi = QInputDialog::getText(this, tr("Uusi ryhmä"), tr("Ryhmän nimi"));
        if( !nimi.isEmpty()) {
            QVariantMap uusiryhma;
            uusiryhma.insert("nimi", nimi);
            KpKysely* kysely = kpk("/ryhmat", KpKysely::POST);
            connect(kysely, &KpKysely::vastaus, kp()->ryhmat(), &RyhmatModel::paivita);
            kysely->kysy(uusiryhma);
        }
    } else {
        AsiakasToimittajaDlg *dlg = new AsiakasToimittajaDlg(this);
        connect( dlg, &AsiakasToimittajaDlg::tallennettu, this, &KumppaniTuoteWidget::paivita);
        dlg->uusi();
        if( ryhma_)
            dlg->lisaaRyhmaan(ryhma_);
    }
}

void KumppaniTuoteWidget::muokkaa()
{
    if( valilehti_ == TUOTTEET) {
        TuoteDialogi *dlg = new TuoteDialogi(this);
        dlg->muokkaa( ui->view->currentIndex().data(TuoteModel::MapRooli).toMap()  );
        connect( dlg, &TuoteDialogi::tuoteTallennettu, this, &KumppaniTuoteWidget::paivita);
    } else if (valilehti_ == RYHMAT) {
        QString nimi = QInputDialog::getText(this, tr("Muokkaa ryhmää"), tr("Ryhmän nimi"),QLineEdit::Normal,
                                             ui->view->currentIndex().data(Qt::DisplayRole).toString());
        if( !nimi.isEmpty()) {
            int ryhmaid = ui->view->currentIndex().data(RyhmatModel::IdRooli).toInt();
            QVariantMap muokattu;
            muokattu.insert("nimi", nimi);
            KpKysely* kysely = kpk(QString("/ryhmat/%1").arg(ryhmaid), KpKysely::PUT);
            connect(kysely, &KpKysely::vastaus, kp()->ryhmat(), &RyhmatModel::paivita);
            kysely->kysy(muokattu);
        }
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
            kp()->tuotteet()->poista(tuoteid);
        }
    } else if( valilehti_ == RYHMAT) {
        if( QMessageBox::question(this, tr("Ryhmän poistaminen"),tr("Haluatko varmasti poistaa ryhmän?")) == QMessageBox::Yes) {
            int ryhmaid = ui->view->currentIndex().data(RyhmatModel::IdRooli).toInt();
            KpKysely *kysely = kpk(QString("/ryhmat/%1").arg(ryhmaid), KpKysely::DELETE );
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
        proxy_->setSourceModel( kp()->tuotteet() );
    else if (valilehti_ == RYHMAT) {
        proxy_->setSourceModel( kp()->ryhmat() );
        ui->view->selectRow(0);
    } else
        proxy_->setSourceModel( asiakkaat_);

    if( valilehti_ == REKISTERI) {
        if( ryhma_)
            asiakkaat_->suodataRyhma(ryhma_);
        else
            asiakkaat_->paivita(AsiakkaatModel::REKISTERI);
    } else if( valilehti_ == ASIAKKAAT )
        asiakkaat_->paivita(AsiakkaatModel::ASIAKKAAT);
    else if( valilehti_ == TOIMITTAJAT)
        asiakkaat_->paivita(AsiakkaatModel::TOIMITTAJAT);


    ui->view->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ilmoitaValinta();
}




