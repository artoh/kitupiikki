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
#include "kayttooikeussivu.h"
#include "ui_kayttooikeudet.h"
#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"
#include "kutsudialog.h"
#include <QSortFilterProxyModel>

KayttoOikeusSivu::KayttoOikeusSivu() :
    ui(new Ui::KayttoOikeudet),
    model(new KayttooikeusModel(this))
{
    ui->setupUi(this);

    QSortFilterProxyModel *proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel(model);
    proxy->setSortRole(Qt::DisplayRole);

    ui->view->setModel(proxy);

    connect( ui->view, &QListView::clicked, this, &KayttoOikeusSivu::naytaValittu );
    connect( model, &KayttooikeusModel::modelReset, this, &KayttoOikeusSivu::malliNollattu);
    connect( ui->lisaysEdit, &QLineEdit::textEdited, this, &KayttoOikeusSivu::tarkastaNimi);
    connect( ui->lisaysEdit, &QLineEdit::returnPressed, this, &KayttoOikeusSivu::lisaaKayttaja);
    connect( ui->lisaysNappi, &QPushButton::clicked, this, &KayttoOikeusSivu::lisaaKayttaja);
    connect( ui->tallennaNappi, &QPushButton::clicked, this, &KayttoOikeusSivu::tallennaOikeudet);
    connect( ui->kaikkiNappi, &QPushButton::clicked, this, &KayttoOikeusSivu::kaikkiOikeudet);
    connect( ui->poistaNappi, &QPushButton::clicked, this, &KayttoOikeusSivu::poistaOikeudet);
    connect( ui->kutsuButton, &QPushButton::clicked, this, &KayttoOikeusSivu::kutsu);

    for(QCheckBox* box : findChildren<QCheckBox*>()) {
        connect(box, &QCheckBox::clicked, this, &KayttoOikeusSivu::tarkastaMuokattu);
    }
}

KayttoOikeusSivu::~KayttoOikeusSivu()
{
    delete ui;
}

bool KayttoOikeusSivu::nollaa()
{
    model->paivita();
    return true;
}

void KayttoOikeusSivu::naytaValittu(const QModelIndex &index)
{
    ui->nimiLabel->setText( index.data(KayttooikeusModel::NimiRooli).toString());
    ui->emailLabel->setText(index.data(KayttooikeusModel::EmailRooli).toString());

    oikeudetAlussa_ = index.data(KayttooikeusModel::OikeusRooli).toStringList();
    ui->omistajaLabel->setVisible(oikeudetAlussa_.contains("Om"));
    ui->oikeusBox->setDisabled(oikeudetAlussa_.contains("Om"));

    for(QCheckBox* box : findChildren<QCheckBox*>()) {
        box->setChecked( oikeudetAlussa_.contains( box->property("Oikeus").toString() ) );
    }    
    nykyisenEmail_ = ui->emailLabel->text();
    tarkastaMuokattu();
}

void KayttoOikeusSivu::malliNollattu()
{
    QModelIndex valittava = model->index(0);
    for(int i=0; i<model->rowCount();i++) {
        if( model->index(i).data(KayttooikeusModel::EmailRooli).toString() == nykyisenEmail_) {
            valittava = model->index(i);
            break;
        }
    }
    ui->view->selectionModel()->select(valittava, QItemSelectionModel::SelectCurrent);
    naytaValittu(valittava);
}

void KayttoOikeusSivu::tarkastaNimi()
{
    ui->lisaysNappi->setEnabled(false);
    QRegularExpression emailRe(R"(^.*@.*\.\w+$)");
    if( emailRe.match( ui->lisaysEdit->text()).hasMatch() ) {
        KpKysely* kysely = kpk( QString("%1/users/%2")
                                .arg(kp()->pilvi()->pilviLoginOsoite())
                                .arg(ui->lisaysEdit->text()));
        connect(kysely, &KpKysely::vastaus, [this] (QVariant* data) {
            this->ui->lisaysNappi->setEnabled( !data->toMap().isEmpty() );
            this->ui->kutsuButton->setEnabled( data->toMap().isEmpty());
            haettuNimi_ = data->toMap().value("name").toString();
        });
        connect(kysely, &KpKysely::virhe, [this] (int virhe) {
            this->ui->kutsuButton->setEnabled( virhe == 203);
        });
        kysely->kysy();
    } else {
        ui->lisaysNappi->setEnabled(false);
        ui->kutsuButton->setEnabled(false);
    }
}


void KayttoOikeusSivu::lisaaKayttaja()
{
    if( !ui->lisaysNappi->isEnabled())
        return;

    ui->lisaysNappi->setEnabled(false);
    nykyisenEmail_ = ui->lisaysEdit->text();
    ui->lisaysEdit->clear();
    QModelIndex uusi = model->lisaa(nykyisenEmail_, haettuNimi_);
    ui->view->clearSelection();
    ui->view->selectionModel()->select(uusi, QItemSelectionModel::SelectCurrent);
    naytaValittu(  uusi );
}

void KayttoOikeusSivu::tallennaOikeudet()
{
    KpKysely* kysely = kpk( QString("%1/permissions/%2")
                            .arg(kp()->pilvi()->pilviLoginOsoite())
                            .arg(kp()->pilvi()->pilviId()),
                            KpKysely::PATCH);
    kysely->connect(kysely, &KpKysely::vastaus, this, &KayttoOikeusSivu::tallennettu);

    QVariantMap map;
    map.insert("email", ui->emailLabel->text());
    map.insert("rights", oikeudetTaulussa());
    QVariantList lista;
    lista.append(map);

    kysely->kysy(lista);
}


void KayttoOikeusSivu::tallennettu()
{
    model->paivita();
    emit kp()->onni(tr("Käyttäjän oikeudet tallennettu"));
}

void KayttoOikeusSivu::tarkastaMuokattu()
{
    ui->tallennaNappi->setEnabled( oikeudetTaulussa() != oikeudetAlussa_ );
    ui->poistaNappi->setEnabled( !oikeudetTaulussa().isEmpty() );
}

void KayttoOikeusSivu::kaikkiOikeudet()
{
    for(QCheckBox* box : findChildren<QCheckBox*>())
        box->setChecked(true);
    tarkastaMuokattu();
}

void KayttoOikeusSivu::poistaOikeudet()
{
    for(QCheckBox* box : findChildren<QCheckBox*>())
        box->setChecked(false);
    tarkastaMuokattu();
}

void KayttoOikeusSivu::kutsu()
{
    QString email = KutsuDialog::kutsu(ui->lisaysEdit->text());
    if(!email.isNull()) {
        ui->lisaysEdit->clear();
        model->paivita();
    }
}


QStringList KayttoOikeusSivu::oikeudetTaulussa() const
{
    QStringList oikeudet;
    for(QCheckBox* box : findChildren<QCheckBox*>()) {
        if( box->isChecked())
            oikeudet.append(box->property("Oikeus").toString());
    }
    return oikeudet;
}

