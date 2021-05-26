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

#include <QSqlQuery>

#include "siirrydlg.h"
#include "db/kirjanpito.h"

SiirryDlg::SiirryDlg() :
    ui(new Ui::SiirryDlg())
{
    ui->setupUi(this);

    if(!kp()->asetukset()->onko("erisarjaan"))
    {
        ui->tyyppiLabel->hide();
        ui->tyyppiCombo->hide();
    } else {
        ui->tyyppiCombo->addItems(kp()->tositeSarjat());
    }

    ui->nroEdit->setValidator( new QRegularExpressionValidator( QRegularExpression("\\d+")) );

    ui->kausiCombo->setModel( kp()->tilikaudet());
    ui->kausiCombo->setModelColumn( TilikausiModel::LYHENNE);

    connect( ui->tyyppiCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(tarkista()));
    connect( ui->nroEdit, SIGNAL(textChanged(QString)), this, SLOT(tarkista()));
    connect( ui->kausiCombo, &QComboBox::currentTextChanged, this, &SiirryDlg::kausiVaihtui);

    kausiVaihtui();
}

void SiirryDlg::tarkista()
{
    tosite = 0;
    ui->siirryNappi->setEnabled(false);

    for(const auto& var : qAsConst(lista_)) {
        QVariantMap map = var.toMap();
        if( map.value("sarja").toString() == ui->tyyppiCombo->currentText() &&
            map.value("tunniste").toString() == ui->nroEdit->text()) {
            tosite = map.value("id").toInt();
            ui->siirryNappi->setEnabled(true);
        }
    }
}

void SiirryDlg::kausiVaihtui()
{
    KpKysely* kysely = kpk("/tositteet");
    kysely->lisaaAttribuutti("alkupvm", ui->kausiCombo->currentData(TilikausiModel::AlkaaRooli).toDate());
    kysely->lisaaAttribuutti("loppupvm", ui->kausiCombo->currentData(TilikausiModel::PaattyyRooli).toDate());
    connect( kysely, &KpKysely::vastaus, this, &SiirryDlg::dataSaapui);
    kysely->kysy();
}

void SiirryDlg::dataSaapui(QVariant *data)
{
    lista_ = data->toList();
    tarkista();
}

int SiirryDlg::tositeId(QDate pvm, const QString& tositelaji)
{
    SiirryDlg dlg;

    dlg.ui->tyyppiCombo->setCurrentText(tositelaji);
    dlg.ui->kausiCombo->setCurrentText( kp()->tilikausiPaivalle(pvm).kausitunnus() );

    if( dlg.exec() == Accepted )
        return dlg.tosite;

    return 0;

}
