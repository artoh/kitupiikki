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
#include "huoneistoeranvalintadialog.h"

#include "db/kirjanpito.h"
#include "laskutus/huoneisto/huoneistomodel.h"

#include "ui_eranvalintadialog.h"

HuoneistoEranValintaDialog::HuoneistoEranValintaDialog(int nykyinen, QWidget *parent)
    : EranValintaDialog(parent),
      nykyinen_(nykyinen)
{
    asetaModel(kp()->huoneistot());
    ui->view->horizontalHeader()->setSectionResizeMode(HuoneistoModel::NIMI, QHeaderView::Stretch);

    kp()->huoneistot()->paivita();

    ui->alkuDate->hide();
    ui->viiva->hide();
    ui->loppuDate->hide();
    ui->avoimetCheck->hide();


}

QVariantMap HuoneistoEranValintaDialog::valittu() const
{
    QVariantMap map;

    int huoneistoId = ui->view->currentIndex().data(HuoneistoModel::IdRooli).toInt();
    const QString huoneistoNimi = ui->view->currentIndex().data(HuoneistoModel::NimiRooli).toString();

    ViiteNumero viite(ViiteNumero::HUONEISTO, huoneistoId);

    map.insert("id", viite.eraId());
    map.insert("selite", huoneistoNimi);

    QVariantMap huoneistoMap;
    huoneistoMap.insert("nimi", huoneistoNimi);
    map.insert("huoneisto", huoneistoMap);
    map.insert("id", huoneistoId);

    return map;
}

void HuoneistoEranValintaDialog::paivitaNykyinen()
{
    if( !nykyinen_)
        return;

    for(int i=0; i < ui->view->model()->rowCount(); i++) {
        if( ui->view->model()->index(i,0).data(HuoneistoModel::IdRooli).toInt() == nykyinen_) {
            ui->view->selectRow(i);
            return;
        }
    }

}

