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
#include "yksittainenlaskudialogi.h"
#include "ui_laskudialogi.h"

#include "model/tosite.h"
#include "db/kirjanpito.h"


#include "laskutus/toimittaja/laskuntoimittaja.h"

#include <QMessageBox>

YksittainenLaskuDialogi::YksittainenLaskuDialogi(Tosite *tosite, QWidget *parent)
    : KantaLaskuDialogi(tosite, parent)
{
    connect( tosite, &Tosite::laskuTallennettu, this, &YksittainenLaskuDialogi::tallennettu);
    connect( tosite, &Tosite::tallennusvirhe, this, [this] () { QMessageBox::critical(this, tr("Tallennusvirhe"),
                                                                                      tr("Laskun tallennus epäonnistui")); });
}

void YksittainenLaskuDialogi::tallenna(int tilaan)
{
    if( tarkasta() ) {
        ui->tallennaNappi->setEnabled(false);
        ui->luonnosNappi->setEnabled(false);
        tositteelle();
        valmisteleTallennus();
        tosite_->tallennaLasku(tilaan);
    }
}

void YksittainenLaskuDialogi::tallennettu(QVariantMap tosite)
{
    // Nyt tallennus on saatettu loppuun saakka!
    // Tähän vielä laskun toimittaminen

    int tila = tosite.value("tila").toInt();

    if( tila == Tosite::LAHETETAAN) {
        QList<int> lista = QList<int>() << tosite.value("id").toInt();
        LaskunToimittaja::toimita(lista);
    }

    emit kp()->kirjanpitoaMuokattu();
    QDialog::accept();
}

