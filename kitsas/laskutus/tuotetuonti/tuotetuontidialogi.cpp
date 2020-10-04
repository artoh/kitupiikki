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
#include "tuotetuontidialogi.h"
#include "tuotetuontimodel.h"
#include "tuotetuontidelegaatti.h"

#include "db/kirjanpito.h"

#include <QTableView>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QHeaderView>

TuoteTuontiDialogi::TuoteTuontiDialogi(const QString &tiedosto, QWidget *parent) :
    QDialog(parent),
    model_(new TuoteTuontiModel(this))
{


    QVBoxLayout *leiska = new QVBoxLayout(this);
    QTableView *view = new QTableView(this);


    view->setModel(model_);
    leiska->addWidget(view);

    QCheckBox *otsikkoCheck = new QCheckBox(tr("Otsikot ensimmäisellä rivillä"));
    leiska->addWidget(otsikkoCheck);

    bbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help);
    leiska->addWidget(bbox);

    connect( model_, &TuoteTuontiModel::otsikkorivit, otsikkoCheck, &QCheckBox::setChecked);
    connect(otsikkoCheck, &QCheckBox::clicked, model_, &TuoteTuontiModel::asetaOtsikkorivi);

    model_->lataaCsv(tiedosto);

    view->horizontalHeader()->setSectionResizeMode(TuoteTuontiModel::MALLI, QHeaderView::Stretch);
    view->setItemDelegateForColumn(TuoteTuontiModel::MUOTO, new TuoteTuontiDelegaatti(this));

    connect( bbox, &QDialogButtonBox::helpRequested, [] { kp()->ohje("/laskutus/tuotteet"); });
    connect( bbox, &QDialogButtonBox::accepted, this, &TuoteTuontiDialogi::accept);
    connect( bbox, &QDialogButtonBox::rejected, this, &TuoteTuontiDialogi::reject);


}

void TuoteTuontiDialogi::accept()
{
    bbox->button(QDialogButtonBox::Ok)->setEnabled(false);
    tallennusLista_ = model_->lista();
    tallennaSeuraava();
}

void TuoteTuontiDialogi::tallennaSeuraava()
{
    if( tallennusLista_.isEmpty()) {
        QDialog::accept();
        kp()->tuotteet()->lataa();
        return;
    }

    QVariantMap map = tallennusLista_.takeFirst().toMap();
    KpKysely *kysely = kpk("/tuotteet", KpKysely::POST);
    connect(kysely, &KpKysely::vastaus, this, &TuoteTuontiDialogi::tallennaSeuraava);
    kysely->kysy(map);
}
