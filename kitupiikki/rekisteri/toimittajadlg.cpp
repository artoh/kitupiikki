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
#include "toimittajadlg.h"
#include "ui_toimittajadlg.h"

#include "maamodel.h"
#include "model/toimittaja.h"
#include "postinumerot.h"

#include "ibandelegaatti.h"
#include "validator/ytunnusvalidator.h"

#include "db/kirjanpito.h"

#include <QListWidgetItem>

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>

ToimittajaDlg::ToimittajaDlg(QWidget *parent) :
    QDialog (parent),
    ui(new Ui::ToimittajaDlg),
    toimittaja_(new Toimittaja(this))
{
    ui->setupUi(this);
    ui->yEdit->setValidator(new YTunnusValidator(false));

    connect( ui->postinumeroEdit, &QLineEdit::textChanged, this, &ToimittajaDlg::haeToimipaikka);
    connect( ui->maaCombo, &QComboBox::currentTextChanged, this, &ToimittajaDlg::maaMuuttui);
    connect( ui->tilitLista, &QListWidget::itemChanged, this, &ToimittajaDlg::tarkastaTilit);

    connect( ui->yEdit, &QLineEdit::textEdited, this, &ToimittajaDlg::haeYTunnarilla);
    connect( ui->yEdit, &QLineEdit::editingFinished, this, &ToimittajaDlg::haeYTunnarilla);

    connect( toimittaja_, &Toimittaja::ladattu, this, &ToimittajaDlg::toimittajaLadattu);
    connect( toimittaja_, &Toimittaja::tallennettu, this, &ToimittajaDlg::tallennettu);

    maaMuuttui();
    tarkastaTilit();
}

ToimittajaDlg::~ToimittajaDlg()
{
    delete ui;
}

void ToimittajaDlg::muokkaa(int id)
{
    toimittaja_->lataa(id);
}

void ToimittajaDlg::uusi(const QString &nimi)
{
    toimittaja_->clear();
    toimittaja_->set("nimi",nimi);
    toimittajaLadattu();
}

void ToimittajaDlg::tarkastaTilit()
{
    bool tyhjat = false;
    for(int i=0; i < ui->tilitLista->count(); i++)
        if( ui->tilitLista->item(i)->data(Qt::EditRole).toString().isEmpty())
            tyhjat = true;

    if( !tyhjat) {
        QListWidgetItem* item = new QListWidgetItem("", ui->tilitLista);
        item->setFlags( item->flags() | Qt::ItemIsEditable );
    }
}

void ToimittajaDlg::maaMuuttui()
{
    QString maa = ui->maaCombo->currentData(MaaModel::KoodiRooli).toString();
    ui->yLabel->setVisible( maa == "fi");
    ui->yEdit->setVisible( maa == "fi");
    ui->alvlabel->setVisible( maa != "fi");
    ui->alvEdit->setVisible( maa != "fi");
    ui->alvEdit->setValidator( new QRegularExpressionValidator(QRegularExpression( ui->maaCombo->currentData(MaaModel::AlvRegExpRooli).toString() ), this) );
}

void ToimittajaDlg::haeToimipaikka()
{
    QString toimipaikka = Postinumerot::toimipaikka( ui->postinumeroEdit->text() );
    if( !toimipaikka.isEmpty() && ui->maaCombo->currentData(MaaModel::KoodiRooli).toString() == "fi")
        ui->kaupunkiEdit->setText(toimipaikka);
}

void ToimittajaDlg::accept()
{

    toimittaja_->set("nimi", ui->nimiEdit->text());
    QString maa = ui->maaCombo->currentData(MaaModel::KoodiRooli).toString();
    toimittaja_->set("maa", maa);

    if( maa == "fi")
        toimittaja_->setYTunnus(ui->yEdit->text());
    else
        toimittaja_->set("alvtunnus", ui->alvEdit->text());

    toimittaja_->set("osoite", ui->osoiteEdit->toPlainText());
    toimittaja_->set("postinumero", ui->postinumeroEdit->text());
    toimittaja_->set("kaupunki", ui->kaupunkiEdit->text());
    toimittaja_->set("email", ui->emailEdit->text());

    QStringList tililista;
    for(int i=0; i<ui->tilitLista->count(); i++)
        if( !ui->tilitLista->item(i)->data(Qt::EditRole).toString().isEmpty())
            tililista.append( ui->tilitLista->item(i)->data(Qt::EditRole).toString() );
    toimittaja_->setTilit(tililista);

    toimittaja_->tallenna();

}

void ToimittajaDlg::toimittajaLadattu()
{
    ui->nimiEdit->setText( toimittaja_->nimi());
    ui->maaCombo->setModel( new MaaModel(this));
    QString maa = toimittaja_->maa();

    ui->maaCombo->setCurrentIndex( ui->maaCombo->findData(maa, MaaModel::KoodiRooli));

    if( maa == "fi")
        ui->yEdit->setText( toimittaja_->ytunnus());
    else
        ui->alvEdit->setText(toimittaja_->alvtunnus());

    ui->emailEdit->setText( toimittaja_->email());
    ui->osoiteEdit->setPlainText( toimittaja_->osoite());
    ui->postinumeroEdit->setText( toimittaja_->postinumero());
    ui->kaupunkiEdit->setText( toimittaja_->kaupunki());

    ui->tilitLista->setItemDelegate( new IbanDelegaatti() );

    for(auto tili : toimittaja_->tilit()) {
        QListWidgetItem* item = new QListWidgetItem(tili, ui->tilitLista);
        item->setFlags( item->flags() | Qt::ItemIsEditable );
    }

    exec();

}

void ToimittajaDlg::tallennettu(int id)
{
    QDialog::accept();
    emit toimittajaTallennettu(id, ui->nimiEdit->text());
}

void ToimittajaDlg::haeYTunnarilla()
{
    if( ui->yEdit->hasAcceptableInput() ) {
        QNetworkRequest request( QUrl("http://avoindata.prh.fi/bis/v1/" + ui->yEdit->text()));
        QNetworkReply *reply = kp()->networkManager()->get(request);
        connect( reply, &QNetworkReply::finished, this, &ToimittajaDlg::yTietoSaapuu);
    }
}

void ToimittajaDlg::yTietoSaapuu()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>( sender());
    QVariant var = QJsonDocument::fromJson( reply->readAll() ).toVariant();
    if( var.toMap().value("results").toList().isEmpty())
        return;

    QVariantMap tieto = var.toMap().value("results").toList().first().toMap();


    ui->nimiEdit->setText( tieto.value("name").toString() );
    QVariantMap osoite = tieto.value("addresses").toList().first().toMap();
    ui->osoiteEdit->setPlainText( osoite.value("street").toString() );
    ui->postinumeroEdit->setText( osoite.value("postCode").toString() );
    ui->kaupunkiEdit->setText( osoite.value("city").toString());


}
