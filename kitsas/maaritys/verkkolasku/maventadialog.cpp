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
#include "maventadialog.h"
#include "ui_maventa.h"

#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"
#include "pilvi/pilvikysely.h"

#include <QMessageBox>
#include <QPushButton>

MaventaDialog::MaventaDialog(QWidget *parent) :
    QDialog(parent), ui( new Ui::MaventaDialog)
{
    ui->setupUi(this);

    connect( ui->api, &QLineEdit::textEdited, this, &MaventaDialog::muokattu);
    connect( ui->uuid, &QLineEdit::textEdited, this, &MaventaDialog::muokattu);
    connect( ui->buttonBox, &QDialogButtonBox::helpRequested, [] { kp()->ohje("maaritykset/verkkolasku"); });
}

void MaventaDialog::accept()
{
    QVariantMap map;
    map.insert("apikey", ui->api->text().trimmed());
    map.insert("clientid", ui->uuid->text().trimmed());
    QString osoite = QString("%1/maventa/%2").arg(kp()->pilvi()->finvoiceOsoite()).arg(kp()->asetus("Ytunnus"));
    PilviKysely *pk= new PilviKysely( kp()->pilvi(), KpKysely::POST, osoite);
    connect( pk, &PilviKysely::vastaus, this, &MaventaDialog::vastaus);
    pk->kysy(map);
}

void MaventaDialog::vastaus(QVariant *data)
{
    QVariantMap map = data->toMap();
    if( map.isEmpty()) {
         QMessageBox::critical(this, tr("Liittäminen epäonnistui"), tr("Maventaan kirjautuminen epäonnistui.\n\n"
                                                                       "Tarkasta kirjoittamasi avaimet."));
    } else {
        emit liitetty(data);
        QDialog::accept();
    }
}

void MaventaDialog::muokattu()
{
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
                ui->api->text().trimmed().length()==36 &&
                ui->uuid->text().trimmed().length()== 36);
}

