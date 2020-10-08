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

#include "kirjaus/eurodelegaatti.h"
#include "rekisteri/asiakastoimittajataydentaja.h"
#include "rekisteri/kumppanivalintadelegaatti.h"

AvausEraDlg::AvausEraDlg(int tili, bool kohdennukset, QList<AvausEra> erat, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AvausEraDlg)
{
    ui->setupUi(this);

    ui->tiliLabel->setText( kp()->tilit()->tiliNumerolla(tili).nimiNumero() );

    ui->eraLabel->setVisible(!kohdennukset);
    ui->kohdennusOhje->setVisible(kohdennukset);

    if( kohdennukset ) {
        model = new AvausKohdennusModel(erat, this);
        ui->view->setModel(model);
        ui->view->hideColumn(AvausKohdennusModel::KUMPPANI);        
    } else {        
        model = new AvausEraModel(erat, this);
        ui->view->setModel(model);        
        ui->view->setItemDelegateForColumn(AvausEraModel::KUMPPANI,
                                          new KumppaniValintaDelegaatti(this));
        ui->view->horizontalHeader()->resizeSection(AvausEraModel::KUMPPANI, 300);       
    }    


    ui->view->horizontalHeader()->setSectionResizeMode( AvausEraKantaModel::NIMI, QHeaderView::Stretch);
    ui->view->setItemDelegateForColumn( TilinavausModel::SALDO, new EuroDelegaatti);

    connect( model, &AvausEraKantaModel::dataChanged, this, &AvausEraDlg::paivitaSumma);
    connect( model, &AvausEraKantaModel::dataChanged, this, &AvausEraDlg::lisaaTarvittaessa);

    connect( ui->buttonBox, &QDialogButtonBox::helpRequested, [] { kp()->ohje("maaritykset/tilinavaus"); });

    paivitaSumma();
}

AvausEraDlg::~AvausEraDlg()
{
    delete ui;
}

QList<AvausEra> AvausEraDlg::erat() const
{
    return model->erat();
}

void AvausEraDlg::paivitaSumma()
{
    ui->summaLabel->setText(tr("Yhteensä %L1 €").arg( model->summa() / 100.0, 10, 'f', 2  ));
}

void AvausEraDlg::lisaaTarvittaessa()
{
    AvausEraModel* avaus = qobject_cast<AvausEraModel*>(model);
    if( avaus && ui->view->currentIndex().row() == model->rowCount() - 1 &&
           !model->data(ui->view->currentIndex()).toString().isEmpty()  ) {
        QModelIndex current = ui->view->currentIndex();
        avaus->lisaaRivi();
        ui->view->setCurrentIndex(current.sibling(current.row(), current.column()+1));
    }
}
