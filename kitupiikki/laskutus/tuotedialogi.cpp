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


TuoteDialogi::TuoteDialogi(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TuoteDialogi)
{
    ui->setupUi(this);

    ui->kohdennusCombo->setModel( new KohdennusProxyModel(this) );
    // ui->tiliEdit->suodataTyypilla("C.*");

    bool alv = kp()->asetukset()->onko("AlvVelvollinen");
    ui->alvLabel->setVisible(alv);
    ui->alvCombo->setVisible(alv);
    ui->bruttoLabel->setVisible(alv);
    ui->bruttoEdit->setVisible(alv);

    connect( ui->nettoEdit, &KpEuroEdit::textChanged,
             this, &TuoteDialogi::laskeBrutto);
    connect( ui->alvCombo, &LaskuAlvCombo::currentTextChanged,
             this, &TuoteDialogi::laskeBrutto);
    connect( ui->bruttoEdit, &KpEuroEdit::textChanged,
             this, &TuoteDialogi::laskeNetto);
}

TuoteDialogi::~TuoteDialogi()
{
    delete ui;
}

void TuoteDialogi::muokkaa(int id)
{

}

void TuoteDialogi::uusi()
{
    ui->kohdennusCombo->setCurrentIndex(
                ui->kohdennusCombo->findData(0, KohdennusModel::IdRooli));

    ui->yksikkoEdit->setText("kpl");
    ui->tiliEdit->valitseTiliNumerolla(3000);

    show();
}

void TuoteDialogi::laskeBrutto()
{
    double netto = ui->nettoEdit->value();
    double brutto = netto * ( 100 + ui->alvCombo->veroProsentti() ) / 100.0;
    if( qRound(brutto * 100) != ui->bruttoEdit->asCents())
        ui->bruttoEdit->setValue(brutto);
    bool alv = ui->alvCombo->veroKoodi() == AlvKoodi::MYYNNIT_NETTO;
    ui->bruttoLabel->setVisible(alv);
    ui->bruttoEdit->setVisible(alv);
}

void TuoteDialogi::laskeNetto()
{
    double brutto = ui->bruttoEdit->value();
    double netto = (100 * brutto) / (100 + ui->alvCombo->veroProsentti());
    if( qRound(netto*100) != ui->nettoEdit->asCents())
        ui->nettoEdit->setValue(netto);
}
