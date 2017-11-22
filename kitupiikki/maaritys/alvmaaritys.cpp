/*
   Copyright (C) 2017 Arto Hyvättinen

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

#include "alvmaaritys.h"
#include "ui_arvonlisavero.h"
#include "db/kirjanpito.h"

#include "alvilmoitusdialog.h"

AlvMaaritys::AlvMaaritys() :
    ui(new Ui::AlvMaaritys)
{
    ui->setupUi(this);

    ui->kausiCombo->addItem("Kuukausi",QVariant(1));
    ui->kausiCombo->addItem("Neljännesvuosi",QVariant(3));
    ui->kausiCombo->addItem("Vuosi", QVariant(12));

    connect( ui->viimeisinEdit, SIGNAL(dateChanged(QDate)), this, SLOT(paivitaSeuraavat()));
    connect(ui->kausiCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(paivitaSeuraavat()));
    connect( ui->tilitaNappi, SIGNAL(clicked(bool)), this, SLOT(ilmoita()));
}

bool AlvMaaritys::nollaa()
{
    ui->kausiCombo->setCurrentIndex( ui->kausiCombo->findData( kp()->asetukset()->luku("AlvKausi") ) );
    ui->viimeisinEdit->setDate( kp()->asetukset()->pvm("AlvIlmoitus"));
    return true;
}

bool AlvMaaritys::onkoMuokattu()
{
    return ui->kausiCombo->currentData().toInt() != kp()->asetukset()->luku("AlvKausi") ||
            ui->viimeisinEdit->date() != kp()->asetukset()->pvm("AlvIlmoitus");
}

bool AlvMaaritys::tallenna()
{
    kp()->asetukset()->aseta("AlvKausi", ui->kausiCombo->currentData().toInt());
    kp()->asetukset()->aseta("AlvIlmoitus", ui->viimeisinEdit->date());
    return true;
}

void AlvMaaritys::paivitaSeuraavat()
{
    QDate viimeisin = ui->viimeisinEdit->date();
    int kausikk = ui->kausiCombo->currentData().toInt();

    seuraavaAlkaa = viimeisin.addDays(1);
    seuraavaLoppuu = seuraavaAlkaa.addMonths( kausikk ).addDays(-1);

    ui->seuraavaLabel->setText( QString("%1 - %2").arg( seuraavaAlkaa.toString(Qt::SystemLocaleShortDate))
                                                        .arg(seuraavaLoppuu.toString(Qt::SystemLocaleShortDate)) );
    ui->erapaivaLabel->setText( seuraavaLoppuu.addDays(12).toString(Qt::SystemLocaleShortDate) );

    emit tallennaKaytossa(onkoMuokattu());
}

void AlvMaaritys::ilmoita()
{
    QDate ilmoitettu = AlvIlmoitusDialog::teeAlvIlmoitus(seuraavaAlkaa, seuraavaLoppuu);
    if( ilmoitettu.isValid())
    {
        kp()->asetukset()->aseta("AlvIlmoitettu", ilmoitettu);
        ui->viimeisinEdit->setDate(ilmoitettu);
    }
}

