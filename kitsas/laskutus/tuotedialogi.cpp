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
#include "tuotedialogi.h"
#include "ui_tuotedialogi.h"

#include "kirjaus/kohdennusproxymodel.h"
#include "db/kirjanpito.h"
#include <QDebug>

TuoteDialogi::TuoteDialogi(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TuoteDialogi)
{
    ui->setupUi(this);

    ui->kohdennusCombo->suodataPaivalla(kp()->paivamaara());

    bool alv = kp()->asetukset()->onko("AlvVelvollinen");
    ui->alvLabel->setVisible(alv);
    ui->alvCombo->setVisible(alv);
    ui->bruttoLabel->setVisible(alv);
    ui->bruttoEdit->setVisible(alv);

    connect( ui->nettoEdit, &KpEuroEdit::textEdited,
             this, &TuoteDialogi::laskeBrutto);
    connect( ui->alvCombo, &LaskuAlvCombo::currentTextChanged,
             this, &TuoteDialogi::laskeBrutto);
    connect( ui->bruttoEdit, &KpEuroEdit::textEdited,
             this, &TuoteDialogi::laskeNetto);
}

TuoteDialogi::~TuoteDialogi()
{
    delete ui;
}

void TuoteDialogi::muokkaa(const QVariantMap &map)
{
    muokattavanaId_ = map.value("id").toInt();
    ui->nimikeEdit->setText( map.value("nimike").toString() );
    ui->yksikkoEdit->setText( map.value("yksikko").toString());
    ui->alvCombo->setCurrentIndex(
                ui->alvCombo->findData( map.value("alvkoodi").toInt() +
                                        map.value("alvprosentti").toInt() * 100));

    double netto = map.value("ahinta").toDouble();
    double brutto = netto * ( 100.0 + ui->alvCombo->veroProsentti() ) / 100.0;

    if( qAbs(qRound64(netto * 1000) - qRound64(netto*100)*10) > 0.005 ) {
        ui->bruttoEdit->setValue(brutto);
        laskeNetto();
    } else {
        ui->nettoEdit->setValue(netto);
        laskeBrutto();
    }

    ui->tiliEdit->valitseTiliNumerolla( map.value("tili").toInt() );    
    ui->kohdennusCombo->setCurrentIndex(
                ui->kohdennusCombo->findData( map.value("kohdennus", 0), KohdennusModel::IdRooli ));
    show();
}

void TuoteDialogi::uusi()
{
    ui->kohdennusCombo->setCurrentIndex(
                ui->kohdennusCombo->findData(0, KohdennusModel::IdRooli));

    ui->yksikkoEdit->setText("kpl");
    ui->tiliEdit->valitseTiliNumerolla(kp()->asetukset()->luku("OletusMyyntitili"));

    show();
}

void TuoteDialogi::accept()
{
    QVariantMap map;
    map.insert("nimike", ui->nimikeEdit->text());
    map.insert("yksikko", ui->yksikkoEdit->text());
    if( brutto_ > 1e-5) {
        double netto = (100.0 * brutto_) / (100.0 + ui->alvCombo->veroProsentti());
        map.insert("ahinta", netto);
    } else {
        map.insert("ahinta", ui->nettoEdit->value());
    }
    map.insert("alvkoodi", ui->alvCombo->veroKoodi());
    map.insert("alvprosentti", ui->alvCombo->currentData().toInt() / 100);
    map.insert("alvkoodi", ui->alvCombo->veroKoodi());
    map.insert("tili", ui->tiliEdit->valittuTilinumero());
    map.insert("kohdennus", ui->kohdennusCombo->currentData(KohdennusModel::IdRooli));

    if( muokattavanaId_ )
        map.insert("id", muokattavanaId_);

    kp()->tuotteet()->paivitaTuote(map);
    QDialog::accept();
}

void TuoteDialogi::laskeBrutto()
{
    double netto = ui->nettoEdit->value();
    double brutto = netto * ( 100.0 + ui->alvCombo->veroProsentti() ) / 100.0;
    if( qRound64(brutto * 100) != ui->bruttoEdit->asCents())
        ui->bruttoEdit->setValue(brutto);
    bool alv = ui->alvCombo->veroKoodi() == AlvKoodi::MYYNNIT_NETTO;
    ui->bruttoLabel->setVisible(alv);
    ui->bruttoEdit->setVisible(alv);
    brutto_ = 0;
}

void TuoteDialogi::laskeNetto()
{
    brutto_ = ui->bruttoEdit->value();
    double netto = (100.0 * brutto_) / (100.0 + ui->alvCombo->veroProsentti());
    qDebug() << brutto_ << "  "  << netto;

    if( qRound64(netto*100.0) != ui->nettoEdit->asCents())
        ui->nettoEdit->setValue(netto);
}

