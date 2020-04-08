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
#include "vakioviitedlg.h"
#include "ui_vakioviitedlg.h"
#include "db/kirjanpito.h"

VakioViiteDlg::VakioViiteDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::VakioViiteDlg)
{
    ui->setupUi(this);

    ui->tiliEdit->suodataTyypilla("C.*");
    connect( ui->buttonBox, &QDialogButtonBox::helpRequested, [] { kp()->ohje("/laskutus/vakioviite");} );
}

VakioViiteDlg::~VakioViiteDlg()
{
    delete ui;
}

void VakioViiteDlg::muokkaa(const QVariantMap &map)
{
    muokattavaViite_ = map.value("viite").toInt();
    ui->otsikkoEdit->setText(map.value("otsikko").toString());
    ui->tiliEdit->valitseTiliNumerolla(map.value("tili").toInt());
    ui->kohdennusCombo->valitseKohdennus(map.value("kohdennus").toInt());
    exec();
}

void VakioViiteDlg::uusi()
{
    ui->kohdennusCombo->setCurrentIndex(
                ui->kohdennusCombo->findData(0, KohdennusModel::IdRooli));
    ui->tiliEdit->valitseTiliNumerolla(3000);
    exec();
}

void VakioViiteDlg::accept()
{
    QVariantMap map;

    map.insert("otsikko", ui->otsikkoEdit->text());
    map.insert("tili", ui->tiliEdit->valittuTilinumero());
    map.insert("kohdennus", ui->kohdennusCombo->kohdennus());

    KpKysely *kysely = muokattavaViite_ ? kpk(QString("/vakioviitteet/%1").arg(muokattavaViite_), KpKysely::PUT) : kpk("/vakioviitteet", KpKysely::POST);
    connect( kysely, &KpKysely::vastaus, this, &VakioViiteDlg::tallennettu );
    kysely->kysy(map);
}

void VakioViiteDlg::tallennettu()
{
    QDialog::accept();
}
