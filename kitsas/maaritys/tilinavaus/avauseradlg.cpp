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
#include "avauseradlg.h"
#include "ui_avauseradlg.h"

#include "avauseramodel.h"
#include "avauskohdennusmodel.h"
#include "avauskuukausimodel.h"

#include "kirjaus/eurodelegaatti.h"
#include "rekisteri/kumppanivalintadelegaatti.h"

#include "db/tili.h"
#include "db/kirjanpito.h"
#include "tools/vuosidelegaatti.h"


AvausEraDlg::AvausEraDlg(TilinavausModel::Erittely erittely, QList<AvausEra> erat, int tilinumero, QWidget *parent) :
    QDialog{parent},
    ui{new Ui::AvausEraDlg},
    erittelyTyyppi_{erittely},
    tilinumero_{tilinumero}
{
    ui->setupUi(this);

    Tili tili = kp()->tilit()->tiliNumerolla(tilinumero);
    ui->tiliLabel->setText( tili.nimiNumero() );

    if( erittely == TilinavausModel::TASEERAT ) {
        model_ = new AvausEraModel(this, tili.luku("tasaerapoisto"));
    } else if( erittely == TilinavausModel::KUUKAUDET) {
        TilinavausModel::Erittely erittely = TilinavausModel::EI_ERITTELYA;
        if( tili.eritellaankoTase() )
            erittely = TilinavausModel::TASEERAT;
        else if(tili.onko(TiliLaji::TULOS) || tili.luku("kohdennukset"))
            erittely = TilinavausModel::KOHDENNUKSET;
        model_ = new AvausKuukausiModel(this, erittely);
    } else {
        model_ = new AvausKohdennusModel(this);
    }

    model_->lataa(erat);
    ui->view->setModel(model_);

    if( erittely == TilinavausModel::TASEERAT ) {
        ui->view->setItemDelegateForColumn(AvausEraModel::KUMPPANI,
                                          new KumppaniValintaDelegaatti(this));
        ui->view->setItemDelegateForColumn(AvausEraModel::SALDO,
                                           new EuroDelegaatti(this));
        if( tili.onko(TiliLaji::TASAERAPOISTO)) {
            ui->view->setItemDelegateForColumn(AvausEraModel::POISTOAIKA,
                                               new VuosiDelegaatti(this));
        }
        ui->view->horizontalHeader()->setSectionResizeMode( AvausEraModel::NIMI, QHeaderView::Stretch );
        ui->view->horizontalHeader()->resizeSection(AvausEraModel::KUMPPANI, 300);

    } else if( erittely == TilinavausModel::KUUKAUDET) {
        ui->view->setItemDelegateForColumn(AvausKuukausiModel::SALDO, new EuroDelegaatti(this));
        ui->view->horizontalHeader()->setSectionResizeMode( AvausKuukausiModel::KUUKAUSI, QHeaderView::Stretch );
        connect( ui->view, &QTableView::clicked, this, &AvausEraDlg::erittely);
        connect( ui->view, &QTableView::activated, this, &AvausEraDlg::erittely);
    } else {
        ui->view->setItemDelegateForColumn(AvausKohdennusModel::SALDO, new EuroDelegaatti(this));
        ui->view->horizontalHeader()->setSectionResizeMode( AvausKohdennusModel::KOHDENNUS, QHeaderView::Stretch );
    }

    ui->kohdennusOhje->setVisible( erittely == TilinavausModel::KOHDENNUKSET );
    ui->eraLabel->setVisible( erittely == TilinavausModel::TASEERAT);


    connect( ui->buttonBox, &QDialogButtonBox::helpRequested, [] { kp()->ohje("asetukset/tilinavaus"); });
    connect( model_, &AvausEraKantaModel::dataChanged, this, &AvausEraDlg::paivitaSumma);
    connect( model_, &AvausEraKantaModel::dataChanged, this, &AvausEraDlg::lisaaTarvittaessa);

    paivitaSumma();
}

AvausEraDlg::~AvausEraDlg()
{
    delete ui;
}

QList<AvausEra> AvausEraDlg::erat() const
{
    return model_->erat();
}

void AvausEraDlg::paivitaSumma()
{
    ui->summaLabel->setText(tr("Yhteensä %1").arg( model_->summa().display() ) );
}

void AvausEraDlg::lisaaTarvittaessa()
{

    AvausEraModel* avaus = qobject_cast<AvausEraModel*>(model_);
    if( avaus && ui->view->currentIndex().row() == model_->rowCount() - 1 &&
           !model_->data(ui->view->currentIndex()).toString().isEmpty()  ) {
        QModelIndex current = ui->view->currentIndex();
        avaus->lisaaRivi();
        ui->view->setCurrentIndex(current.sibling(current.row(), current.column()+1));
    }
}

void AvausEraDlg::erittely(const QModelIndex& index)
{
    if( index.isValid() && index.column() == AvausKuukausiModel::ERITTELY )
    {
        AvausKuukausiModel* kuukausi = qobject_cast<AvausKuukausiModel*>(model_);
        if( !kuukausi) return;

        QList<AvausEra> erat =kuukausi->kuukaudenErat(index.row());
        AvausEraDlg dlg( kuukausi->erittely(), erat, tilinumero_, this);
        if( dlg.exec() == QDialog::Accepted) {
            kuukausi->asetaKuukaudenErat(index.row(), dlg.erat());
        }
    }
}
