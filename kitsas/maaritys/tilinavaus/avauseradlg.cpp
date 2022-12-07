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


AvausEraDlg::AvausEraDlg(TilinavausModel *avaus, int tilinumero, QWidget *parent) :
    QDialog{parent},
    avaus_{avaus}
{
    ui->setupUi(this);

    tili_ = kp()->tilit()->tiliNumerolla(tilinumero);
    ui->tiliLabel->setText( tili_.nimiNumero() );



    if( tili_.eritellaankoTase()) {
        model_ = new AvausEraModel(this);
    } else if( avaus->kuukausittain()) {
        model_ = new AvausKuukausiModel(this);
    } else {
        model_ = new AvausKohdennusModel(this);
    }

    const QList<AvausEra> erat = avaus_->erat(tilinumero);
    model_->lataa(erat);

    ui->view->setModel(model_);

}
/*
AvausEraDlg::AvausEraDlg(int tili, bool kohdennukset, QList<AvausEra> erat, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AvausEraDlg)
{
    ui->setupUi(this);

    ui->tiliLabel->setText( kp()->tilit()->tiliNumerolla(tili).nimiNumero() );

    ui->eraLabel->setVisible(!kohdennukset);
    ui->kohdennusOhje->setVisible(kohdennukset);    

    if( kohdennukset ) {
        model_ = new AvausKohdennusModel(erat, this);
        ui->view->setModel(model_);
        ui->view->hideColumn(AvausKohdennusModel::KUMPPANI);        
    } else {        
        model_ = new AvausEraModel(erat, this);
        ui->view->setModel(model_);
        ui->view->setItemDelegateForColumn(AvausEraModel::KUMPPANI,
                                          new KumppaniValintaDelegaatti(this));
        ui->view->setItemDelegateForColumn(AvausEraModel::POISTOAIKA,
                                           new VuosiDelegaatti(this));
        ui->view->horizontalHeader()->resizeSection(AvausEraModel::KUMPPANI, 300);       
    }    

    Tili* tiliObj = kp()->tilit()->tili(tili);
    if( !tiliObj || !tiliObj->onko(TiliLaji::TASAERAPOISTO))
        ui->view->hideColumn(AvausEraKantaModel::POISTOAIKA);


    ui->view->horizontalHeader()->setSectionResizeMode( AvausEraKantaModel::NIMI, QHeaderView::Stretch);
    ui->view->setItemDelegateForColumn( TilinavausModel::SALDO, new EuroDelegaatti);

    connect( model_, &AvausEraKantaModel::dataChanged, this, &AvausEraDlg::paivitaSumma);
    connect( model_, &AvausEraKantaModel::dataChanged, this, &AvausEraDlg::lisaaTarvittaessa);

    connect( ui->buttonBox, &QDialogButtonBox::helpRequested, [] { kp()->ohje("asetukset/tilinavaus"); });

    paivitaSumma();
}
*/
AvausEraDlg::~AvausEraDlg()
{
    delete ui;
}

QList<AvausEra> AvausEraDlg::erat() const
{
    return model_->erat();
}

void AvausEraDlg::accept()
{
    avaus_->asetaErat(tili_.numero(), model_->erat());

    QDialog::accept();
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
