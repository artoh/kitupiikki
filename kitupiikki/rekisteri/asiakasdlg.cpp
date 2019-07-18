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
#include "asiakasdlg.h"
#include "ui_asiakasdlg.h"

#include "model/asiakas.h"
#include "postinumerot.h"
#include "maamodel.h"

#include "validator/ytunnusvalidator.h"

#include <QRegularExpressionValidator>

AsiakasDlg::AsiakasDlg(QWidget *parent, Asiakas *asiakas) :
    QDialog(parent),
    ui(new Ui::AsiakasDlg),
    asiakas_(asiakas)
{
    ui->setupUi(this);
    QString maa = asiakas->maa();

    ui->nimiEdit->setText( asiakas->nimi());

    ui->maaCombo->setModel( new MaaModel(this));
    ui->maaCombo->setCurrentIndex( ui->maaCombo->findData(maa, MaaModel::KoodiRooli) );

    if( maa == "fi")
        ui->ytunnusEdit->setText( asiakas->ytunnus());
    else
        ui->alvEdit->setText( asiakas->alvtunnus());
    maaMuuttui();

    ui->ytunnusEdit->setValidator(new YTunnusValidator(false));

    ui->emailEdit->setText( asiakas->email());
    ui->lahiEdit->setPlainText(asiakas->osoite());
    ui->postinumeroEdit->setText( asiakas->postinumero());
    ui->kaupunkiEdit->setText( asiakas->kaupunki());

    ui->verkkolaskuEdit->setText( asiakas->ovt());
    ui->valittajaEdit->setText( asiakas->operaattori());

    connect( ui->postinumeroEdit, &QLineEdit::textChanged, this, &AsiakasDlg::haeToimipaikka);
    connect( ui->maaCombo, &QComboBox::currentTextChanged, this, &AsiakasDlg::maaMuuttui);
    connect( ui->ytunnusEdit, &QLineEdit::editingFinished, this, &AsiakasDlg::ymuuttui);
}

AsiakasDlg::~AsiakasDlg()
{
    delete ui;
}

void AsiakasDlg::accept()
{
    asiakas_->set("nimi", ui->nimiEdit->text());
    QString maa = ui->maaCombo->currentData(MaaModel::KoodiRooli).toString();

    asiakas_->set("maa", maa);
    if( maa == "fi")
        asiakas_->setYTunnus(ui->ytunnusEdit->text());
    else
        asiakas_->set("alvtunnus", ui->alvEdit->text());

    asiakas_->set("email", ui->emailEdit->text());
    asiakas_->set("osoite", ui->lahiEdit->toPlainText());
    asiakas_->set("postinumero", ui->postinumeroEdit->text());
    asiakas_->set("kaupunki", ui->kaupunkiEdit->text());
    asiakas_->set("ovt", ui->verkkolaskuEdit->text());
    asiakas_->set("operaattori", ui->valittajaEdit->text());

    QDialog::accept();
}

void AsiakasDlg::maaMuuttui()
{
    QString maa = ui->maaCombo->currentData(MaaModel::KoodiRooli).toString();
    ui->yLabel->setVisible( maa == "fi");
    ui->ytunnusEdit->setVisible( maa == "fi");
    ui->alvLabel->setVisible( maa != "fi");
    ui->alvEdit->setVisible( maa != "fi");
    ui->alvEdit->setValidator( new QRegularExpressionValidator(QRegularExpression( ui->maaCombo->currentData(MaaModel::AlvRegExpRooli).toString() ), this) );
}

void AsiakasDlg::haeToimipaikka()
{
    QString toimipaikka = Postinumerot::toimipaikka( ui->postinumeroEdit->text() );
    if( !toimipaikka.isEmpty() && ui->maaCombo->currentData(MaaModel::KoodiRooli).toString() == "fi")
        ui->kaupunkiEdit->setText(toimipaikka);
}

void AsiakasDlg::ymuuttui()
{
    QString ytunnus = ui->ytunnusEdit->text();
    if( ytunnus.length() == 9 )
        ui->verkkolaskuEdit->setText( "0037" + ytunnus.remove('-') );

}
