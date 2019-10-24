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
#include "finvoicemaaritys.h"
#include "ui_verkkolaskumaaritys.h"
#include "db/kirjanpito.h"

#include <QFileDialog>

FinvoiceMaaritys::FinvoiceMaaritys(QWidget *parent)
    : TallentavaMaaritysWidget ("maaritys/finvoice", parent),
      ui_(new Ui::VerkkolaskuMaaritys)
{
    ui_->setupUi(this);

    connect( ui_->kansioNappi, &QPushButton::clicked, this, &FinvoiceMaaritys::valitseKansio);
}

FinvoiceMaaritys::~FinvoiceMaaritys()
{
    delete ui_;
}

bool FinvoiceMaaritys::tallenna()
{
    // Erillinen asetus sille, että verkkolaskutus on käytössä,
    // jotta se tulee helpommaksi tarkastaa!

    kp()->asetukset()->aseta("VerkkolaskuKaytossa",
                             !ui_->osoiteEdit->text().isEmpty() &&
                             !ui_->valittajaEdit->text().isEmpty() &&
                             !ui_->kansioEdit->text().isEmpty());

    return TallentavaMaaritysWidget::tallenna();
}

void FinvoiceMaaritys::valitseKansio()
{
    QString kansio = QFileDialog::getExistingDirectory(this, tr("Valitse verkkolaskujen tallennuskansio"),
                                                       QDir::homePath());
    if( !kansio.isEmpty())
        ui_->kansioEdit->setText(kansio);
}
