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
#include "tavallinenlaskudialogi.h"
#include "ui_laskudialogi.h"

#include "ennakkohyvitysmodel.h"
#include "ennakkohyvitysdialogi.h"
#include "model/tosite.h"
#include "model/tositevienti.h"
#include "model/tositeviennit.h"
#include "model/tositerivi.h"
#include "model/tositerivit.h"
#include "db/kirjanpito.h"

TavallinenLaskuDialogi::TavallinenLaskuDialogi(Tosite *tosite, QWidget *parent)
    : RivillinenLaskuDialogi(tosite, parent),
      ennakkoModel_(new EnnakkoHyvitysModel(this))
{
    connect( ui->toimitusDate, &KpDateEdit::dateChanged, this, &TavallinenLaskuDialogi::paivitaToistojakso);
    connect( ui->jaksoDate, &KpDateEdit::dateChanged, this, &TavallinenLaskuDialogi::paivitaToistojakso);
    connect( ui->maksuCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &TavallinenLaskuDialogi::paivitaToistojakso);

    connect( ennakkoModel_, &EnnakkoHyvitysModel::modelReset, this, &TavallinenLaskuDialogi::maksuTapaMuuttui);
    connect( ui->hyvitaEnnakkoNappi, &QPushButton::clicked, this, &TavallinenLaskuDialogi::hyvitaEnnakko);

    ui->toistoPvmPaattyy->setNull();

    toistoTositteelta();
}

void TavallinenLaskuDialogi::tositteelle()
{
    RivillinenLaskuDialogi::tositteelle();

    int toistoIndex = ui->tabWidget->indexOf( ui->tabWidget->findChild<QWidget*>("toisto") );
    if( ui->tabWidget->isTabEnabled(toistoIndex) && ui->toistoGroup->isChecked()) {
        tosite()->lasku().setToistoJaksoPituus( ui->toistoJaksoSpin->value() );

        const int paivat = ui->toistoEnnenRadio->isChecked() ?
                    0 - ui->toistoLaskuaikaSpin->value() :
                    0 + ui->toistoLaskuaikaSpin->value();

        tosite()->lasku().setToistoPvm( ui->jaksoDate->date().addDays( paivat ) );
        tosite()->lasku().setToistoHinnastonMukaan( ui->toistoHinnastoCheck->isChecked() );

    } else {
        tosite()->lasku().setToistoPvm( QDate());
    }

}


void TavallinenLaskuDialogi::toistoTositteelta()
{    
    const Lasku& lasku = tosite()->constLasku();
    const QDate& toistopaiva = lasku.toistoPvm();

    ui->toistoGroup->setChecked( toistopaiva.isValid() );
    if( lasku.toistoPvm().isValid() ) {
        ui->toistoJaksoSpin->setValue( lasku.toistoJaksoPituus() );

        const int paivaa = lasku.jaksopvm().daysTo(toistopaiva );
        ui->toistoLaskuaikaSpin->setValue( qAbs(paivaa) );
        ui->toistoEnnenRadio->setChecked( paivaa <= 0 );
        ui->toistoJalkeenRadio->setChecked( paivaa >= 0);
        ui->toistoHinnastoCheck->setChecked( lasku.toistoHinnastonMukaan() );
    }

    ui->tallennaToistoNappi->setVisible( tosite()->id() );
    ui->lopetaToistoNappi->setVisible( lasku.toistoJaksoPituus());
}

void TavallinenLaskuDialogi::paivitaToistojakso()
{
    QDate alku = ui->toimitusDate->date();
    const QDate& loppu = ui->jaksoDate->date();
    int kk = 0;

    if( loppu.isValid()) {
        while( alku < loppu) {
            kk++;
            if( alku.month() == 12) {
                alku.setDate( alku.year()+1, 1, alku.day() );
            } else {
                alku.setDate( alku.year(), alku.month()+1, alku.day());
            }
        }
    }

    int maksutapa = ui->maksuCombo->currentData().toInt();
    int toistoIndex = ui->tabWidget->indexOf( ui->tabWidget->findChild<QWidget*>("toisto") );
    ui->tabWidget->setTabEnabled( toistoIndex,
                ( maksutapa == Lasku::LASKU || maksutapa == Lasku::KUUKAUSITTAINEN)
                && kk > 0);

    if( !tosite()->lasku().toistoJaksoPituus())
        ui->toistoJaksoSpin->setValue( kk > 0 ? kk : 12);
}

void TavallinenLaskuDialogi::asiakasMuuttui()
{
    ennakkoModel_->lataaErat(ui->asiakas->id());
    RivillinenLaskuDialogi::asiakasMuuttui();
}

void TavallinenLaskuDialogi::maksuTapaMuuttui()
{
    ui->hyvitaEnnakkoNappi->setVisible(
                ui->maksuCombo->currentData().toInt() != Lasku::ENNAKKOLASKU &&
                ennakkoModel_->rowCount()
            );
    RivillinenLaskuDialogi::maksuTapaMuuttui();
}

void TavallinenLaskuDialogi::hyvitaEnnakko()
{
    EnnakkoHyvitysDialogi dlg(ennakkoModel_, this);
    if( dlg.exec() == QDialog::Accepted) {
        const int eraId = dlg.eraId();
        const Euro euro = dlg.euro();

        KpKysely* kysely = kpk("/tositeet");
        kysely->lisaaAttribuutti("vienti", dlg.eraId());
        connect( kysely, &KpKysely::vastaus,
                 [this, eraId, euro] (QVariant* data)
                    { this->ennakkoTietoSaapuu(data, eraId, euro); });
        kysely->kysy();
    }
}

void TavallinenLaskuDialogi::ennakkoTietoSaapuu(QVariant *data, int eraId, Euro euro)
{
    Tosite haettu;
    haettu.lataaData(data);
    for(int i=0; i < haettu.viennit()->rowCount(); i++) {
        const TositeVienti& vienti = haettu.viennit()->vienti(i);
        if( vienti.eraId() == eraId) {
            TositeRivi rivi;
            rivi.setTili( vienti.tili() );
            rivi.setMyyntiKpl(1.0);
            rivi.setLaskutetaanKpl("1");
            rivi.setANetto( 0 - euro.toDouble() );
            rivi.setAlvKoodi( vienti.alvKoodi() == AlvKoodi::ENNAKKOLASKU_MYYNTI ?
                                  AlvKoodi::MYYNNIT_NETTO : vienti.alvKoodi());
            rivi.setAlvProsentti( vienti.alvProsentti() );
            rivi.setNimike(
                        tulkkaa("enhyri", tosite()->lasku().kieli())
                            .arg(haettu.lasku().numero()));
            rivi.laskeYhteensa();
            tosite()->rivit()->lisaaRivi(rivi);
        }
    }
}

