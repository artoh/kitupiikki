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

#include <QListWidgetItem>

ToimittajaDlg::ToimittajaDlg(QWidget *parent, Toimittaja *toimittaja) :
    QDialog (parent),
    ui(new Ui::ToimittajaDlg),
    toimittaja_(toimittaja)
{
    ui->setupUi(this);

    ui->nimiEdit->setText( toimittaja->nimi());
    ui->maaCombo->setModel( new MaaModel(this));
    QString maa = toimittaja->maa();

    ui->maaCombo->setCurrentIndex( ui->maaCombo->findData(maa, MaaModel::KoodiRooli));

    if( maa == "fi")
        ui->yEdit->setText( toimittaja->ytunnus());
    else
        ui->alvEdit->setText(toimittaja->alvtunnus());

    ui->emailEdit->setText( toimittaja->email());
    ui->osoiteEdit->setPlainText( toimittaja->osoite());
    ui->postinumeroEdit->setText( toimittaja->postinumero());
    ui->kaupunkiEdit->setText( toimittaja->kaupunki());

    ui->tilitLista->setItemDelegate( new IbanDelegaatti() );

    for(auto tili : toimittaja->tilit()) {
        QListWidgetItem* item = new QListWidgetItem(tili, ui->tilitLista);
        item->setFlags( item->flags() | Qt::ItemIsEditable );
    }

    connect( ui->postinumeroEdit, &QLineEdit::textChanged, this, &ToimittajaDlg::haeToimipaikka);
    connect( ui->maaCombo, &QComboBox::currentTextChanged, this, &ToimittajaDlg::maaMuuttui);
    connect( ui->tilitLista, &QListWidget::itemChanged, this, &ToimittajaDlg::tarkastaTilit);

    maaMuuttui();
    tarkastaTilit();
}

ToimittajaDlg::~ToimittajaDlg()
{
    delete ui;
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

    QDialog::accept();

}
