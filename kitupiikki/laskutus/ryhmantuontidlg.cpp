/*
   Copyright (C) 2018 Arto Hyvättinen

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
#include "ryhmantuontidlg.h"
#include "ui_ryhmantuontidlg.h"
#include "ryhmantuontimodel.h"

#include "db/kirjanpito.h"

RyhmanTuontiDlg::RyhmanTuontiDlg(const QString &tiedostonnimi, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RyhmanTuontiDlg),
    model(new RyhmanTuontiModel(this))
{
    ui->setupUi(this);
    model->lataaCsv(tiedostonnimi);
    ui->view->setModel(model);
    ui->otsikkoCheck->setChecked(model->onkoOtsikkoRivi());

    for(int i=0; i < RyhmanTuontiModel::YTUNNUS; i++)
        ui->combo->addItem( RyhmanTuontiModel::otsikkoTeksti(i), i );

    connect( ui->otsikkoCheck, &QPushButton::toggled, model, &RyhmanTuontiModel::asetaOtsikkoRivi);
    connect( ui->view->selectionModel(), &QItemSelectionModel::selectionChanged, this, &RyhmanTuontiDlg::naytaSarake);
    connect( ui->combo, &QComboBox::currentTextChanged, this, &RyhmanTuontiDlg::vaihdaSarake);
    connect( ui->ohjeNappi, &QPushButton::clicked, []{ kp()->ohje("laskutus"); });

    ui->view->selectColumn(0);
}

RyhmanTuontiDlg::~RyhmanTuontiDlg()
{
    delete ui;
    delete model;
}

void RyhmanTuontiDlg::vaihdaSarake()
{
    if( !ui->view->selectionModel()->selectedColumns().isEmpty())
    {
        int sarake = ui->view->selectionModel()->selectedColumns().first().column();
        if( ui->combo->currentIndex() != model->muoto(sarake))
        {
            model->asetaMuoto( sarake, ui->combo->currentData().toInt() );
            qApp->processEvents();
            ui->view->selectColumn(sarake);
        }
    }

}

void RyhmanTuontiDlg::naytaSarake(const QItemSelection &valinta)
{
    if( !valinta.indexes().isEmpty())
    {
        int sarake = valinta.indexes().first().column();
        ui->combo->setCurrentIndex( model->muoto(sarake) );
    }
}
