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
#include "laskurividialogi.h"
#include "ui_laskurividialogi.h"

#include "../alennustyyppimodel.h"
#include "db/kitsasinterface.h"
#include "db/kohdennusmodel.h"

LaskuRiviDialogi::LaskuRiviDialogi(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LaskuRiviDialogi)
{
    ui->setupUi(this);

    ui->alennusSyyCombo->setModel(new AlennusTyyppiModel);    

    connect( ui->verotonEdit, &KpEuroEdit::textEdited, this, &LaskuRiviDialogi::anettoMuokattu );
    connect( ui->alvCombo, &QComboBox::currentTextChanged, this, &LaskuRiviDialogi::paivitaBrutto);
    connect( ui->alennusSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &LaskuRiviDialogi::paivitaAleProsentti);
    connect( ui->euroAleEdit, &KpEuroEdit::textEdited, this, &LaskuRiviDialogi::paivitaBrutto);
    connect( ui->laskutetaanEdit, &QLineEdit::textEdited, this, &LaskuRiviDialogi::paivitaBrutto);
}

LaskuRiviDialogi::~LaskuRiviDialogi()
{
    delete ui;
}

void LaskuRiviDialogi::lataa(const TositeRivi &rivi, const QDate &pvm, LaskuAlvCombo::AsiakasVeroLaji asiakasVerolaji, bool ennakkolasku,
                             KitsasInterface* interface)
{
    ui->nimikeEdit->setText( rivi.nimike() );
    ui->kuvausEdit->setText( rivi.kuvaus());

    ui->toimitettuEdit->setText( rivi.toimitettuKpl() );
    ui->jalkitoimitusEdit->setText( rivi.jalkitoimitusKpl() );
    ui->laskutetaanEdit->setText( rivi.laskutetaanKpl());

    if( rivi.unKoodi().isEmpty())
        ui->yksikkoCombo->setYksikko( rivi.yksikko());
    else
        ui->yksikkoCombo->setUNkoodi(rivi.unKoodi());

    ui->verotonEdit->setValue( rivi.aNetto() );
    anetto_ = rivi.aNetto();

    ui->alvCombo->alusta( asiakasVerolaji, rivi.alvkoodi(), ennakkolasku, pvm); // TODOTODOTODO

    ui->tiliEdit->valitseTiliNumerolla( rivi.tili() );

    const KohdennusModel& kohdennukset = interface->kohdennukset();

    ui->kohdennusLabel->setVisible( kohdennukset.kohdennuksia() );
    ui->kohdennusCombo->setVisible( kohdennukset.kohdennuksia());

    ui->kohdennusCombo->valitseKohdennus( rivi.kohdennus() );

    ui->merkkausLabel->setVisible( kohdennukset.merkkauksia());
    ui->merkkausCombo->setVisible( kohdennukset.merkkauksia());

    ui->merkkausCombo->haeMerkkaukset(pvm);
    ui->merkkausCombo->setSelectedItems( rivi.merkkaukset() );

    ui->alennusSpin->setValue( rivi.aleProsentti() );
    ui->euroAleEdit->setEuro( rivi.euroAlennus() );

    if( rivi.alennusSyy())
        ui->alennusSyyCombo->setCurrentIndex(
                    ui->alennusSyyCombo->findData(rivi.alennusSyy()) );

    ui->lisatietoEdit->setPlainText( rivi.lisatiedot() );

    ui->verollinenEdit->setValue( rivi.bruttoYhteensa());

}

TositeRivi LaskuRiviDialogi::rivi() const
{
    TositeRivi rivi;
    rivi.setNimike( ui->nimikeEdit->text() );
    rivi.setKuvaus( ui->kuvausEdit->text());

    rivi.setToimitettuKpl( ui->toimitettuEdit->text());
    rivi.setJalkitoimitusKpl( ui->jalkitoimitusEdit->text());
    rivi.setLaskutetaanKpl( ui->laskutetaanEdit->text());
    rivi.setMyyntiKpl( ui->laskutetaanEdit->text().toDouble() );

    rivi.setANetto(anetto_);
    rivi.setAlvKoodi( ui->alvCombo->veroKoodi() );
    rivi.setAlvProsentti( ui->alvCombo->veroProsentti());

    rivi.setBruttoYhteensa( ui->verollinenEdit->text());

    rivi.setTili( ui->tiliEdit->valittuTilinumero());
    rivi.setKohdennus( ui->kohdennusCombo->kohdennus() );
    rivi.setMerkkaukset( ui->merkkausCombo->selectedDatas());

    rivi.setAleProsentti( ui->alennusSpin->value());
    rivi.setEuroAlennus( ui->euroAleEdit->text());
    rivi.setAlennusSyy( ui->alennusSyyCombo->currentData().toInt() );
    rivi.setLisatiedot( ui->lisatietoEdit->toPlainText());

    return rivi;
}

void LaskuRiviDialogi::anettoMuokattu()
{
    anetto_ = ui->verotonEdit->value();
    paivitaBrutto();
}

void LaskuRiviDialogi::paivitaBrutto()
{
    TositeRivi trivi(rivi());
    trivi.laskeYhteensa();
    ui->verollinenEdit->setEuro(trivi.bruttoYhteensa());
}

void LaskuRiviDialogi::paivitaAleProsentti()
{
    ui->euroAleEdit->setCents(0);
    paivitaBrutto();
}

void LaskuRiviDialogi::paivitaEuroAlennus()
{
    ui->alennusSpin->setValue(0);
    paivitaBrutto();
}

void LaskuRiviDialogi::paivitaAHinta()
{
    TositeRivi trivi(rivi());
    trivi.laskeYksikko();
    anetto_ = trivi.aNetto();
    ui->verotonEdit->setValue( trivi.aNetto() );
}
