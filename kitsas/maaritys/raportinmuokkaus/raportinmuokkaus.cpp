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
#include <QInputDialog>

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
    connect( ui->lisaaEnnenNappi, &QPushButton::clicked, this, &RaportinMuokkaus::lisaaEnnen);
    connect( ui->lisaaJalkeenNappi, &QPushButton::clicked, this, &RaportinMuokkaus::lisaaJalkeen);
    connect( ui->poistaNappi, &QPushButton::clicked, this, &RaportinMuokkaus::poista);
    connect( ui->kopioiNappi, &QPushButton::clicked, this, &RaportinMuokkaus::kopioiRaportti);
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

bool RaportinMuokkaus::onkoMuokattu()
{
    QString raportti = ui->raporttiCombo->currentText();
    QString str = kp()->asetukset()->asetus(raportti);
    return str != data();
}

bool RaportinMuokkaus::tallenna()
{
    kp()->asetukset()->aseta( ui->raporttiCombo->currentText(), data() );
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

    ui.nimiView->lataa(nimi_);
    ui.muotoView->lataa(muoto_);


    connect( ui.buttonBox, &QDialogButtonBox::helpRequested, [] { kp()->ohje("maaritykset/raportit/"); });

    if( dlg.exec() == QDialog::Accepted) {
        nimi_ = ui.nimiView->tekstit();
        muoto_ = ui.muotoView->tekstit();
        emit tallennaKaytossa(onkoMuokattu());
    }

}

void RaportinMuokkaus::muokkaa()
{
    QModelIndex index = ui->view->currentIndex();
    if( index.isValid())
        model_->setData(index, RaportinmuokkausDialogi::muokkaa(index.data(Qt::EditRole).toMap()) );
    ilmoitaMuokattu();

}

void RaportinMuokkaus::paivitaNapit(const QModelIndex &index)
{
    ui->muokkaaNappi->setEnabled(index.isValid());
    ui->poistaNappi->setEnabled(index.isValid());
}

void RaportinMuokkaus::ilmoitaMuokattu()
{
    emit tallennaKaytossa(onkoMuokattu());
}

void RaportinMuokkaus::kopioiRaportti()
{
    QString uusi = QInputDialog::getText(this, tr("Raportin kopioiminen"), tr("Anna uuden raportin tunnus. \nTunnusta ei näytetä käyttäjälle"));
    if( uusi.isEmpty())
        return;
    QString nykyinen = ui->raporttiCombo->currentText();
    QString alkuosa = nykyinen.left(nykyinen.indexOf("/"));
    QString uusiTunnus = alkuosa + "/" + uusi;
    muoto_ = Monikielinen();

    kp()->asetukset()->aseta(uusiTunnus, data());
    nollaa();
    ui->raporttiCombo->setCurrentText(uusiTunnus);
    muokkaaNimikkeet();
}

void RaportinMuokkaus::lisaaEnnen()
{
    QModelIndex index = ui->view->currentIndex();
    QVariantMap data = RaportinmuokkausDialogi::uusi();
    if( !data.isEmpty()) {
        if( index.isValid())
            model_->lisaaRivi(index.row(), data);
        else
            model_->lisaaRivi(0, data );
        ilmoitaMuokattu();
    }
}

void RaportinMuokkaus::lisaaJalkeen()
{
    QModelIndex index = ui->view->currentIndex();    
    QVariantMap data = RaportinmuokkausDialogi::uusi();
    if( !data.isEmpty()) {
        if( index.isValid())
            model_->lisaaRivi(index.row() + 1,data );
        else
            model_->lisaaRivi(0, data );
        ilmoitaMuokattu();
    }
}

void RaportinMuokkaus::poista()
{
    QModelIndex index = ui->view->currentIndex();
    if( index.isValid())
        model_->poistaRivi(index.row());
    ilmoitaMuokattu();
}

QString RaportinMuokkaus::data() const
{
    QVariantMap map;
    map.insert("nimi", nimi_.map());
    map.insert("muoto", muoto_.map());
    map.insert("rivit", model_->rivit());
    return QString::fromUtf8(QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact));
}

