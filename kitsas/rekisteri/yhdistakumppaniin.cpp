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
#include "yhdistakumppaniin.h"
#include "ui_yhdistakumppaniin.h"


#include "db/kirjanpito.h"
#include <QPushButton>

YhdistaKumppaniin::YhdistaKumppaniin(AsiakkaatModel* model, int id, const QString &nimi, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::YhdistaKumppaniin),
    id_(id),
    nimi_(nimi)
{
    ui->setupUi(this);

    ui->nimiLabel->setText(nimi);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    proxy = new YhdistamisProxyModel(this);
    proxy->asetaId(id);
    proxy->setSourceModel(model);
    ui->view->setModel(proxy);

    connect( ui->sopivatCheck, &QCheckBox::toggled, this, &YhdistaKumppaniin::suodata);
    connect( ui->view->selectionModel(), &QItemSelectionModel::selectionChanged, [this] { this->ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(this->ui->view->currentIndex().isValid()); });
}

YhdistaKumppaniin::~YhdistaKumppaniin()
{
    delete ui;
}

void YhdistaKumppaniin::accept()
{
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    QVariantMap map;
    map.insert("vanha", id_);
    map.insert("uusi", ui->view->currentIndex().data(AsiakkaatModel::IdRooli).toInt());

    KpKysely* kysely = kpk("/kumppanit/yhdista",KpKysely::POST);
    connect( kysely, &KpKysely::vastaus, this, &YhdistaKumppaniin::valmis);
    kysely->kysy(map);
}

void YhdistaKumppaniin::suodata(bool onko)
{
    proxy->suodataNimella(onko ? nimi_ : QString());
}

void YhdistaKumppaniin::valmis()
{
    emit kp()->kirjanpitoaMuokattu();
    QDialog::accept();
}
