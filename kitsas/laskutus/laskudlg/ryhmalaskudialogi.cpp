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
#include "ryhmalaskudialogi.h"
#include "../ryhmalasku/ryhmalaskutab.h"

#include "ui_laskudialogi.h"

#include "model/tosite.h"
#include "db/kirjanpito.h"
#include "rivivientigeneroija.h"

#include <QMessageBox>

RyhmaLaskuDialogi::RyhmaLaskuDialogi(Tosite *tosite, QWidget *parent) :
    RivillinenLaskuDialogi(tosite, parent),
    ryhmalaskuTab_(new RyhmalaskuTab),
    tallennusTosite_(new Tosite(this))
{
    setWindowTitle(tr("Ryhmälasku"));

    int toistoIndex = ui->tabWidget->indexOf( ui->tabWidget->findChild<QWidget*>("toisto") );
    ui->tabWidget->removeTab(toistoIndex);

    ui->tabWidget->addTab( ryhmalaskuTab_, QIcon(":/pic/asiakkaat.png"), tr("Laskutettavat"));

    ui->asiakasLabel->hide();
    ui->asiakas->hide();
    ui->osoiteLabel->hide();
    ui->osoiteEdit->hide();
    ui->emailLabel->hide();
    ui->email->hide();
    ui->asviiteLabel->hide();
    ui->asViiteEdit->hide();
    ui->kieliCombo->hide();
    ui->valmisNappi->hide();
    ui->laskutusCombo->hide();

    ui->luonnosNappi->hide();

}

void RyhmaLaskuDialogi::tallenna(int /* tilaan */)
{
    jono_ = ryhmalaskuTab_->model()->laskutettavat();
    if( jono_.isEmpty()) {
        QMessageBox::information(this, tr("Ryhmälasku"),
                                 tr("Lisää ensin laskun saajat Laskutettavat-välilehdelle."));
    } else {
        tositteelle();
        tallennaSeuraava();
    }
}

void RyhmaLaskuDialogi::tallennaSeuraava()
{
    if( jono_.isEmpty()) {
        // Kaikki tallennettu
        emit kp()->onni(tr("Laskut tallennettu Lähtevät-kansioon"));
        emit kp()->kirjanpitoaMuokattu();
        QDialog::accept();
        return;
    }
    LaskutettavatModel::Laskutettava nykyinen = jono_.takeLast();

    tallennusTosite_->lataa(tosite()->tallennettava());

    tallennusTosite_->lasku().setLahetystapa(nykyinen.lahetystapa());
    tallennusTosite_->lasku().setKieli( nykyinen.kieli() );
    tallennusTosite_->asetaKumppani( nykyinen.map() );
    tallennusTosite_->lasku().setEmail( nykyinen.email() );
    tallennusTosite_->lasku().setOsoite( nykyinen.osoite() );

    RiviVientiGeneroija rivigeneroija(kp());
    rivigeneroija.generoiViennit(tallennusTosite_);

    connect( tallennusTosite_, &Tosite::laskuTallennettu, this, &RyhmaLaskuDialogi::tallennaSeuraava);
    tallennusTosite_->tallennaLasku(Tosite::VALMISLASKU);
}

