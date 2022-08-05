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
#include "pilvi/pilvimodel.h"

#include <QMessageBox>

TavallinenLaskuDialogi::TavallinenLaskuDialogi(Tosite *tosite, QWidget *parent)
    : RivillinenLaskuDialogi(tosite, parent),
      ennakkoModel_(new EnnakkoHyvitysModel(this))
{
    connect( ui->toimitusDate, &KpDateEdit::dateChanged, this, &TavallinenLaskuDialogi::paivitaToistojakso);
    connect( ui->jaksoDate, &KpDateEdit::dateChanged, this, &TavallinenLaskuDialogi::paivitaToistojakso);
    connect( ui->maksuCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &TavallinenLaskuDialogi::paivitaToistojakso);

    connect( ennakkoModel_, &EnnakkoHyvitysModel::modelReset, this, &TavallinenLaskuDialogi::maksuTapaMuuttui);
    connect( ui->hyvitaEnnakkoNappi, &QPushButton::clicked, this, &TavallinenLaskuDialogi::hyvitaEnnakko);

    connect( ui->toistoGroup, &QGroupBox::toggled, this, &TavallinenLaskuDialogi::paivitaNapit);
    connect( ui->toistoJaksoSpin, qOverload<int>(&QSpinBox::valueChanged), this, &TavallinenLaskuDialogi::paivitaNapit );
    connect( ui->toistoLaskuaikaSpin, qOverload<int>(&QSpinBox::valueChanged), this, &TavallinenLaskuDialogi::paivitaNapit);
    connect( ui->toistoErapaivaSpin, qOverload<int>(&QSpinBox::valueChanged), this, &TavallinenLaskuDialogi::paivitaNapit);
    connect( ui->toistoEnnenRadio, &QRadioButton::toggled, this, &TavallinenLaskuDialogi::paivitaNapit);
    connect( ui->toistoPvmPaattyy, &KpDateEdit::dateChanged, this, &TavallinenLaskuDialogi::paivitaNapit);
    connect( ui->toistoHinnastoCheck, &QRadioButton::toggled, this, &TavallinenLaskuDialogi::paivitaNapit);

    connect( ui->tallennaToistoNappi, &QPushButton::clicked, this, &TavallinenLaskuDialogi::tallennaToisto);
    connect( ui->lopetaToistoNappi, &QPushButton::clicked, this, &TavallinenLaskuDialogi::lopetaToisto);

    ui->toistoPvmPaattyy->setNull();

    toistoTositteelta();    
    merkkaaTallennettu();
}

void TavallinenLaskuDialogi::tositteelle()
{
    RivillinenLaskuDialogi::tositteelle();

    int toistoIndex = ui->tabWidget->indexOf( ui->tabWidget->findChild<QWidget*>("toisto") );
    if( ui->tabWidget->isTabEnabled(toistoIndex) && ui->toistoGroup->isChecked()) {


        tosite()->lasku().setToisto( toistoPvm(), ui->toistoJaksoSpin->value(),
                                     ui->toistoHinnastoCheck->isChecked(), ui->toistoPvmPaattyy->date());

    } else {
        tosite()->lasku().lopetaToisto();
    }

}

QDate TavallinenLaskuDialogi::toistoPvm() const
{
    const int paivat = ui->toistoEnnenRadio->isChecked() ?
                0 - ui->toistoLaskuaikaSpin->value() :
                0 + ui->toistoLaskuaikaSpin->value();
    return ui->jaksoDate->date().addDays(paivat);
}


void TavallinenLaskuDialogi::toistoTositteelta()
{    
    const Lasku& lasku = tosite()->constLasku();
    const QDate& toistopaiva = lasku.toistoPvm();

    paivitaToistojakso();

    ui->toistoGroup->setChecked( toistopaiva.isValid() );
    if( lasku.toistoPvm().isValid() ) {
        ui->toistoJaksoSpin->setValue( lasku.toistoJaksoPituus() );

        const int paivaa = lasku.jaksopvm().daysTo(toistopaiva );
        ui->toistoLaskuaikaSpin->setValue( qAbs(paivaa) );
        ui->toistoEnnenRadio->setChecked( paivaa <= 0 );
        ui->toistoJalkeenRadio->setChecked( paivaa >= 0);
        ui->toistoHinnastoCheck->setChecked( lasku.toistoHinnastonMukaan() );
        ui->toistoPvmPaattyy->setDate( lasku.toistoLoppuu() );
    }

    ui->tallennaToistoNappi->setVisible( tosite()->id() );
    ui->lopetaToistoNappi->setVisible( lasku.toistoJaksoPituus());
}

void TavallinenLaskuDialogi::paivitaToistojakso()
{
    int toistoIndex = ui->tabWidget->indexOf( ui->tabWidget->findChild<QWidget*>("toisto") );
    bool pilvessa = qobject_cast<PilviModel*>(kp()->yhteysModel());
    if(!pilvessa) {
        ui->tabWidget->setTabEnabled(toistoIndex, false);
        return;
    }

    QDate alku = ui->toimitusDate->date();
    const QDate& loppu = ui->jaksoDate->date();

    int kk = 0;

    if( loppu.isValid()) {
        // Lasketaan kuukaudet alun jo lopun välillä
        qlonglong alkuluku = alku.year() * 12 * 31 + alku.month() * 31 + alku.day();
        const qlonglong loppuluku = loppu.year() * 12 * 31 + loppu.month() * 31 + loppu.day();
        while( alkuluku < loppuluku) {
            kk++;
            alkuluku += 31;
        }
    }

    int maksutapa = ui->maksuCombo->currentData().toInt();
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

        KpKysely* kysely = kpk("/tositteet");
        kysely->lisaaAttribuutti("vienti", eraId);
        connect( kysely, &KpKysely::vastaus, this,
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
            rivi.setEnnakkoEra( eraId );
            rivi.setMyyntiKpl(1.0);
            rivi.setLaskutetaanKpl("1");
            rivi.setANetto( 0 - euro.toDouble() );
            rivi.setAlvKoodi( vienti.alvKoodi() == AlvKoodi::ENNAKKOLASKU_MYYNTI ?
                                  AlvKoodi::MYYNNIT_NETTO : vienti.alvKoodi());
            rivi.setAlvProsentti( vienti.alvProsentti() );
            rivi.setNimike(
                        tulkkaa("enhyri", tosite()->lasku().kieli())
                            .arg(haettu.lasku().numero()));            
            tosite()->rivit()->lisaaRivi(rivi);
        }
    }
}

void TavallinenLaskuDialogi::tallennaToisto()
{
    KpKysely *kysely = kpk(QString("/tositteet/%1").arg(tosite()->id()), KpKysely::PATCH);
    connect( kysely, &KpKysely::vastaus, this, [this]  {this->merkkaaTallennettu(); this->paivitaNapit();});
    QVariantMap map;
    map.insert("laskutoisto", Lasku::toistoMap( toistoPvm(), ui->toistoJaksoSpin->value(),
                                           ui->toistoHinnastoCheck->isChecked(), ui->toistoPvmPaattyy->date() ));
    kysely->kysy( map );
}

void TavallinenLaskuDialogi::lopetaToisto()
{
    KpKysely *kysely = kpk(QString("/tositteet/%1").arg(tosite()->id()), KpKysely::PATCH);
    connect( kysely, &KpKysely::vastaus, this, [this]  {this->ui->toistoGroup->setChecked(false);
                                                  this->merkkaaTallennettu();});
    QVariantMap map;
    map.insert("laskutoisto", QVariant());
    kysely->kysy( map );
}

void TavallinenLaskuDialogi::merkkaaTallennettu()
{
    tallennettuKaytossa_ = ui->toistoGroup->isChecked();
    tallennettuJakso_ = ui->toistoJaksoSpin->value();
    tallennettuLaskutus_ = ui->toistoLaskuaikaSpin->value();
    tallennettuEnnen_ = ui->toistoEnnenRadio->isChecked();
    tallennettuPaattyy_ = ui->toistoPvmPaattyy->date();
    tallennettuHinnastolla_ = ui->toistoHinnastoCheck->isChecked();
    paivitaNapit();
}

bool TavallinenLaskuDialogi::onkoTallennettu()
{
    if( !ui->toistoGroup->isChecked() && !tallennettuKaytossa_)
        return true;
    else
        return tallennettuKaytossa_ == ui->toistoGroup->isChecked() &&
                tallennettuJakso_ == ui->toistoJaksoSpin->value() &&
                tallennettuLaskutus_ == ui->toistoLaskuaikaSpin->value() &&
                tallennettuEnnen_ == ui->toistoEnnenRadio->isChecked() &&
                ( (!tallennettuPaattyy_.isValid() && !ui->toistoPvmPaattyy->date().isValid())
                  || tallennettuPaattyy_ == ui->toistoPvmPaattyy->date() ) &&
                tallennettuHinnastolla_ == ui->toistoHinnastoCheck->isChecked();
}

void TavallinenLaskuDialogi::paivitaNapit()
{    
    ui->tallennaToistoNappi->setEnabled( !onkoTallennettu() && ui->toistoGroup->isChecked() );
    ui->lopetaToistoNappi->setEnabled( tallennettuKaytossa_ );
}

bool TavallinenLaskuDialogi::tarkasta()
{
    if( maksutapa() == Lasku::ENNAKKOLASKU && !asiakasId_ ) {
        QMessageBox::critical(this, tr("Ennakkolasku"),
                              tr("Ennakkolasku voidaan laatia vain asiakasrekisterissä "
                                 "olevalle asiakkaalle. Lisää ensin asiakas rekisteriin"));
        return false;
    }

    return RivillinenLaskuDialogi::tarkasta();
}

