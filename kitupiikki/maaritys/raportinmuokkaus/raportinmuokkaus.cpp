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
#include "raportinmuokkaus.h"
#include "ui_raportinmuokkaus.h"
#include "raporttimuokkausmodel.h"
#include "db/kirjanpito.h"

#include "ui_raporttinimikedialogi.h"
#include "raportinmuokkausdialogi.h"

#include <QJsonDocument>
#include <QDialog>
#include <QHeaderView>

RaportinMuokkaus::RaportinMuokkaus(QWidget *parent)
    : MaaritysWidget(parent),
      ui( new Ui::RaportinMuokkaus),
      model_( new RaporttiMuokkausModel(this))
{
    ui->setupUi(this);
    ui->view->setModel(model_);
    ui->view->horizontalHeader()->setSectionResizeMode(RaporttiMuokkausModel::TEKSTI, QHeaderView::Stretch );

    connect(ui->raporttiCombo, &QComboBox::currentTextChanged, this, &RaportinMuokkaus::lataa);
    connect(ui->nimikeNappi, &QPushButton::clicked, this, &RaportinMuokkaus::muokkaaNimikkeet);
    connect(ui->muokkaaNappi, &QPushButton::clicked, this, &RaportinMuokkaus::muokkaa);
    connect(ui->view->selectionModel(), &QItemSelectionModel::currentChanged, this, &RaportinMuokkaus::paivitaNapit);

}

bool RaportinMuokkaus::nollaa()
{
    ui->raporttiCombo->clear();
    QStringList lista = kp()->asetukset()->avaimet("tase/");
    lista.append( kp()->asetukset()->avaimet("tulos/") );

    for(auto item : lista)
        ui->raporttiCombo->addItem(item);

    paivitaNapit(QModelIndex());
    return true;
}

void RaportinMuokkaus::lataa(const QString &raportti)
{
    QString str = kp()->asetukset()->asetus(raportti);
    QVariantMap map = QJsonDocument::fromJson( str.toUtf8() ).toVariant().toMap();

    nimi_.aseta(map.value("nimi"));
    muoto_.aseta(map.value("muoto"));
    model_->lataa(map.value("rivit").toList());
}

void RaportinMuokkaus::muokkaaNimikkeet()
{
    Ui::RaporttiNimikeDialogi ui;
    QDialog dlg(this);
    ui.setupUi(&dlg);

    nimi_.alustaListWidget(ui.nimiView);
    muoto_.alustaListWidget(ui.muotoView);

    if( dlg.exec() == QDialog::Accepted) {
        nimi_.lataa(ui.nimiView);
        muoto_.lataa(ui.muotoView);
    }

}

void RaportinMuokkaus::muokkaa()
{
    QModelIndex index = ui->view->currentIndex();
    if( index.isValid())
        model_->setData(index, RaportinmuokkausDialogi::muokkaa(index.data(Qt::EditRole).toMap()) );
}

void RaportinMuokkaus::paivitaNapit(const QModelIndex &index)
{
    ui->muokkaaNappi->setEnabled(index.isValid());
    ui->poistaNappi->setEnabled(index.isValid());
}

