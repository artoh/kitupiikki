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
    connect( ui->lisaysNappi, &QPushButton::clicked, this, &KayttoOikeusSivu::lisaaKayttaja);
    connect( ui->tallennaNappi, &QPushButton::clicked, this, &KayttoOikeusSivu::tallennaOikeudet);
    connect( ui->kaikkiNappi, &QPushButton::clicked, this, &KayttoOikeusSivu::kaikkiOikeudet);

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
    ui->tallennaNappi->setEnabled(false);
}

void KayttoOikeusSivu::malliNollattu()
{
    if( nykyisenEmail_.isEmpty()) {
        ui->view->selectionModel()->select(model->index(0), QItemSelectionModel::SelectCurrent);
        naytaValittu(model->index(0));

    } else {
        for(int i=0; i<model->rowCount(); i++) {
            const QModelIndex& index = model->index(i);
            if( index.data(KayttooikeusModel::EmailRooli).toString() == nykyisenEmail_) {
                ui->view->selectionModel()->select(index, QItemSelectionModel::SelectCurrent);
                naytaValittu(index);
                return;
            }
        }

        ui->view->clearSelection();
        ui->view->setCurrentIndex(QModelIndex());
        nollaaNakyma();
    }
}

void KayttoOikeusSivu::tarkastaNimi()
{
    ui->lisaysNappi->setEnabled(false);
    QRegularExpression emailRe(R"(^([\w-]*(\.[\w-]+)?)+@(\w+\.\w+)(\.\w+)*$)");
    if( emailRe.match( ui->lisaysEdit->text()).hasMatch() ) {
        KpKysely* kysely = kpk( QString("%1/users/%2")
                                .arg(kp()->pilvi()->pilviLoginOsoite())
                                .arg(ui->lisaysEdit->text()));
        connect(kysely, &KpKysely::vastaus, [this] (QVariant* data) {
            this->ui->lisaysNappi->setEnabled( !data->toMap().isEmpty() );
        });
        kysely->kysy();
    }
}

void KayttoOikeusSivu::nollaaNakyma()
{
    ui->emailLabel->setText(nykyisenEmail_);
    ui->omistajaLabel->hide();
    ui->oikeusBox->setEnabled(true);
    ui->tallennaNappi->setEnabled(false);

    for(QCheckBox* box : findChildren<QCheckBox*>()) {
        box->setChecked( false );
    }
    oikeudetAlussa_.clear();
    KpKysely* kysely = kpk( QString("%1/users/%2")
                            .arg(kp()->pilvi()->pilviLoginOsoite())
                            .arg(nykyisenEmail_) );
    connect(kysely, &KpKysely::vastaus, [this] (QVariant* data) {
        this->ui->nimiLabel->setText( data->toMap().value("name").toString() );
    });
    kysely->kysy();


}

void KayttoOikeusSivu::lisaaKayttaja()
{
    ui->lisaysNappi->setEnabled(false);
    nykyisenEmail_ = ui->lisaysEdit->text();
    ui->lisaysEdit->clear();
    malliNollattu();
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
}

void KayttoOikeusSivu::kaikkiOikeudet()
{
    for(QCheckBox* box : findChildren<QCheckBox*>())
        box->setChecked(true);
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

