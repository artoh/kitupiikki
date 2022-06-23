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
#include "maksumuistusdialogi.h"

#include "model/tosite.h"
#include "model/lasku.h"
#include "model/tositevienti.h"
#include "model/tositeviennit.h"
#include "maksumuistutusmuodostaja.h"

#include "db/kirjanpito.h"
#include "ui_laskudialogi.h"

MaksumuistusDialogi::MaksumuistusDialogi(Tosite *tosite, QWidget *parent) :
    YksittainenLaskuDialogi(tosite, parent),
    muodostaja_(kp())
{
    ui->tabWidget->removeTab( ui->tabWidget->indexOf( ui->tabWidget->findChild<QWidget*>("rivit") ) );

    lataa();

    connect( ui->mmMuistutusCheck, &QCheckBox::clicked, this, &MaksumuistusDialogi::paivitaSumma);
    connect( ui->mmMuistutusMaara, &KpEuroEdit::textChanged, this, &MaksumuistusDialogi::paivitaSumma);
    connect( ui->mmViivastysCheck, &QCheckBox::clicked, this, &MaksumuistusDialogi::paivitaSumma);
    connect( ui->mmViivastysAlkaa, &KpDateEdit::dateChanged, this, &MaksumuistusDialogi::paivitaSumma);
    connect( ui->mmViivastysLoppuu, &KpDateEdit::dateChanged, this, &MaksumuistusDialogi::paivitaSumma);
    connect( ui->mmAvoin, &KpEuroEdit::textChanged, this, &MaksumuistusDialogi::paivitaSumma);

    ui->maksuCombo->hide();
    if( tosite->constLasku().valvonta() ) {
        ui->valvontaCombo->setEnabled(false);
        ui->tarkeCombo->setEnabled(false);
    } else {
        ui->valvontaLabel->hide();
        ui->valvontaCombo->hide();
        ui->tarkeCombo->hide();
    }

    ui->toimituspvmLabel->hide();
    ui->toimitusDate->hide();
    ui->jaksoViivaLabel->hide();
    ui->jaksoDate->hide();

    setWindowTitle(tr("Maksumuistutus %1").arg(tosite->lasku().numero()));
    ui->laskunPvmLabel->setText(tr("Maksumuistutuksen pvm"));
}

void MaksumuistusDialogi::lataa()
{
    const Lasku& lasku = tosite()->constLasku();
    ui->mmAvoin->setEuro( lasku.aiempiSaldo() );
    ui->mmMuistutusCheck->setChecked( lasku.muistutusmaksu().cents() );
    ui->mmMuistutusMaara->setValue( lasku.muistutusmaksu().toDouble() );
    ui->mmViivastysCheck->setChecked( lasku.korkoEuroa().cents() );
    ui->mmViivastysAlkaa->setDate( lasku.korkoAlkaa() );
    ui->mmViivastysLoppuu->setDate( lasku.korkoLoppuu());
    ui->mmViivastysMaara->setText( lasku.korkoEuroa().display() );

    Euro yhteensa = lasku.aiempiSaldo() + lasku.muistutusmaksu() + lasku.korkoEuroa();
    ui->mmYhteensa->setText(yhteensa.display());
}

void MaksumuistusDialogi::paivitaSumma()
{
    Euro korko = muodostaja_.laskeKorko( ui->mmAvoin->euro(),
                                        ui->mmViivastysAlkaa->date(),
                                        ui->mmViivastysLoppuu->date(),
                                        ui->mmViivastysCheck->isChecked() ? ui->viivkorkoSpin->value() : 0
                                        );
    Euro yhteensa = ui->mmAvoin->euro() +
            ( ui->mmMuistutusCheck->isChecked() ? Euro::fromDouble( ui->mmMuistutusMaara->value() ) : Euro(0) ) +
            korko;
    ui->mmViivastysMaara->setText( korko.display() );
    ui->mmYhteensa->setText( yhteensa.display() );
}

void MaksumuistusDialogi::valmisteleTallennus()
{
    // EraId on alkuperäisen laskun vastaviennin erä
    int eraId = tosite()->lasku().aiemmat().value(0).toMap().value("viennit").toList().value(0).toMap().value("era").toMap().value("id").toInt();

    muodostaja_.muodostaMuistutukset( tosite(),
                                      ui->laskuPvm->date(),
                                      eraId,
                                      ui->mmMuistutusCheck->isChecked() ? ui->mmMuistutusMaara->euro() : Euro(0),
                                      ui->mmAvoin->euro(),
                                      ui->mmViivastysAlkaa->date(),
                                      ui->mmViivastysLoppuu->date(),
                                      ui->mmViivastysCheck ? ui->viivkorkoSpin->value() : 0.0);
}
