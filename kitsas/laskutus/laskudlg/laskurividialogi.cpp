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
#include "db/kirjanpito.h"

LaskuRiviDialogi::LaskuRiviDialogi(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LaskuRiviDialogi)
{
    ui->setupUi(this);

    ui->alennusSyyCombo->setModel(new AlennusTyyppiModel);    

    connect( ui->laskutetaanEdit, &KpKplEdit::textEdited, this, &LaskuRiviDialogi::maaraMuutos );
    connect( ui->alvCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &LaskuRiviDialogi::veroMuutos );

    connect( ui->aNetto, &KpEuroEdit::euroMuuttui, this, [this] (Euro euro) { if(!paivitys_) { rivi_.setANetto(euro.toDouble()); paivita(); }} );
    connect( ui->aBrutto, &KpEuroEdit::euroMuuttui, this, [this](Euro euro) { if(!paivitys_) { rivi_.setABrutto(euro.toDouble()); paivita(); }} );

    connect( ui->alennusSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this] (double prossat) { if(!paivitys_) { rivi_.setEuroAlennus(0); rivi_.setAleProsentti(prossat); paivita(); }} );

    connect( ui->euroAleEdit, &KpEuroEdit::euroMuuttui, this, [this](Euro euro) { if(!paivitys_) { rivi_.setAleProsentti(0); rivi_.setEuroAlennus(euro.toDouble()); paivita(); }} );
    connect( ui->bruttoAleEdit, &KpEuroEdit::euroMuuttui, this, [this](Euro euro) { if(!paivitys_) { rivi_.setAleProsentti(0); rivi_.setBruttoEuroAlennus(euro.toDouble()); paivita(); }} );

    connect( ui->nettoYhteensa, &KpEuroEdit::euroMuuttui, this, [this](Euro euro) { if(!paivitys_) { rivi_.setNettoYhteensa(euro.toDouble()); paivita(); }} );
    connect( ui->verollinenEdit, &KpEuroEdit::euroMuuttui, this, [this](Euro euro) { if(!paivitys_) { rivi_.setBruttoYhteensa(euro); paivita(); }} );

    connect( ui->alkupvmEdit, &KpDateEdit::dateChanged, this, [this](const QDate& date) { this->ui->loppupvmEdit->setEnabled(date.isValid()); this->ui->loppupvmEdit->setDateRange(date, QDate()); } );

    bool alv = kp()->asetukset()->onko(AsetusModel::AlvVelvollinen);    

    ui->alvLabel->setVisible(alv);
    ui->alvCombo->setVisible(alv);

    ui->aBruttoLabel->setVisible(alv);
    ui->aBrutto->setVisible(alv);
    ui->bruttoAleLabel->setVisible(alv);
    ui->bruttoAleEdit->setVisible(alv);
    ui->verollinenLabel->setVisible(alv);
    ui->verollinenEdit->setVisible(alv);

    ui->alkupvmEdit->setNullable(true);
    ui->loppupvmEdit->setNullable(true);
}

LaskuRiviDialogi::~LaskuRiviDialogi()
{
    delete ui;
}

void LaskuRiviDialogi::lataa(const TositeRivi &rivi, const QDate &pvm, LaskuAlvCombo::AsiakasVeroLaji asiakasVerolaji, bool ennakkolasku,
                             KitsasInterface* interface)
{
    rivi_ = rivi;
    paivitys_ = true;

    ui->nimikeEdit->setText( rivi.nimike() );
    ui->kuvausEdit->setText( rivi.kuvaus());
    ui->koodiEdit->setText( rivi.tuotekoodi());

    ui->toimitettuEdit->setText( rivi.toimitettuKpl() );
    ui->jalkitoimitusEdit->setText( rivi.jalkitoimitusKpl() );
    ui->laskutetaanEdit->setText( rivi.laskutetaanKpl());

    if( rivi.unKoodi().isEmpty() || rivi.unKoodi() == "ZZ")
        ui->yksikkoCombo->setYksikko( rivi.yksikko());
    else
        ui->yksikkoCombo->setUNkoodi(rivi.unKoodi());

    ui->alvCombo->alusta( asiakasVerolaji, rivi.alvkoodi(), ennakkolasku, pvm);
    ui->alvCombo->aseta( rivi.alvkoodi(), rivi.alvProsentti());

    ui->tiliEdit->valitseTiliNumerolla( rivi.tili() );

    const KohdennusModel& kohdennukset = interface->kohdennukset();

    ui->kohdennusLabel->setVisible( kohdennukset.kohdennuksia() );
    ui->kohdennusCombo->setVisible( kohdennukset.kohdennuksia());

    ui->kohdennusCombo->valitseKohdennus( rivi.kohdennus() );

    ui->merkkausLabel->setVisible( kohdennukset.merkkauksia());
    ui->merkkausCombo->setVisible( kohdennukset.merkkauksia());

    ui->merkkausCombo->haeMerkkaukset(pvm);
    ui->merkkausCombo->setSelectedItems( rivi.merkkaukset() );

    if( rivi.aleProsentti() > 0.005)
        ui->alennusSpin->setValue( rivi.aleProsentti() );
    else if( rivi.euroAlennus() > 0.005)
        ui->euroAleEdit->setValue( rivi.euroAlennus() );

    if( rivi.alennusSyy())
        ui->alennusSyyCombo->setCurrentIndex(
                    ui->alennusSyyCombo->findData(rivi.alennusSyy()) );

    ui->lisatietoEdit->setPlainText( rivi.lisatiedot() );

    ui->alkupvmEdit->setDate( rivi.jaksoAlkaa() );
    ui->loppupvmEdit->setDate( rivi.jaksoLoppuu() );
    ui->loppupvmEdit->setEnabled( rivi.jaksoAlkaa().isValid() );

    paivitys_ = false;

    paivita();
}

TositeRivi LaskuRiviDialogi::rivi()
{
    rivi_.setNimike( ui->nimikeEdit->text());
    rivi_.setKuvaus( ui->kuvausEdit->text());
    rivi_.setTuoteKoodi( ui->koodiEdit->text());

    rivi_.setToimitettuKpl( ui->toimitettuEdit->text());
    rivi_.setJalkitoimitusKpl( ui->jalkitoimitusEdit->text());

    if( ui->yksikkoCombo->unKoodi().isEmpty()) {
        rivi_.setUNkoodi("");
        rivi_.setYksikko( ui->yksikkoCombo->yksikko());        
    } else
        rivi_.setUNkoodi(ui->yksikkoCombo->unKoodi());


    rivi_.setTili( ui->tiliEdit->valittuTilinumero());
    rivi_.setKohdennus( ui->kohdennusCombo->kohdennus() );
    rivi_.setMerkkaukset( ui->merkkausCombo->selectedDatas());

    rivi_.setAlennusSyy( ui->alennusSyyCombo->currentData().toInt() );
    rivi_.setLisatiedot( ui->lisatietoEdit->toPlainText());

    rivi_.setJaksoAlkaa( ui->alkupvmEdit->date() );
    rivi_.setJaksoLoppuu( rivi_.jaksoAlkaa().isValid() ? ui->loppupvmEdit->date() : QDate() );

    return rivi_;
}

void LaskuRiviDialogi::paivita()
{
    if(paivitys_)
        return;

    paivitys_ = true;

    if(  qAbs( ui->aNetto->value() - rivi_.aNetto() ) > 0.005)
        ui->aNetto->setValue( rivi_.aNetto() );

    if( qAbs( ui->aBrutto->value() - rivi_.aBrutto()) > 0.005)
        ui->aBrutto->setValue( rivi_.aBrutto() );

    if( qAbs( ui->alennusSpin->value() - rivi_.laskettuAleProsentti()) > 0.005 )
        ui->alennusSpin->setValue( rivi_.laskettuAleProsentti() );

    if( qAbs( ui->euroAleEdit->value() - rivi_.laskennallinenEuroAlennus()) > 0.005 )
        ui->euroAleEdit->setValue( rivi_.laskennallinenEuroAlennus() );


    if( qAbs( ui->bruttoAleEdit->value() - rivi_.laskennallinenBruttoEuroAlennus()) > 0.005)
        ui->bruttoAleEdit->setValue( rivi_.laskennallinenBruttoEuroAlennus() );

    if( qAbs( ui->nettoYhteensa->value() - rivi_.nettoYhteensa() ) > 0.005)
        ui->nettoYhteensa->setValue( rivi_.nettoYhteensa() );

    if( ui->verollinenEdit->euro().cents() != rivi_.bruttoYhteensa().cents())
        ui->verollinenEdit->setEuro( rivi_.bruttoYhteensa() );

    paivitys_ = false;
}

void LaskuRiviDialogi::maaraMuutos()
{
    rivi_.setLaskutetaanKpl( ui->laskutetaanEdit->text() );
    rivi_.setMyyntiKpl( ui->laskutetaanEdit->kpl() );
    paivita();

}

void LaskuRiviDialogi::veroMuutos()
{
    rivi_.setAlvKoodi( ui->alvCombo->veroKoodi() );
    rivi_.setAlvProsentti( ui->alvCombo->veroProsentti());
    paivita();
}

