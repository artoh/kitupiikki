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

    connect( ui->nettoEdit, &KpYhEdit::textEdited,
             this, &TuoteDialogi::laskeBrutto);
    connect( ui->alvCombo, &LaskuAlvCombo::currentTextChanged,
             this, &TuoteDialogi::laskeBrutto);
    connect( ui->bruttoEdit, &KpYhEdit::textEdited,
             this, &TuoteDialogi::laskeNetto);

    ui->alvCombo->alusta();
}

TuoteDialogi::~TuoteDialogi()
{
    delete ui;
}

void TuoteDialogi::muokkaa(const Tuote &tuote)
{
    muokattavanaId_ = tuote.id();
    ui->nimikeList->lataa( tuote.constKielinen());

    if( tuote.unKoodi().isEmpty())
        ui->yksikkoCombo->setYksikko(tuote.yksikko());
    else
        ui->yksikkoCombo->setUNkoodi(tuote.unKoodi());

    ui->alvCombo->aseta( tuote.alvkoodi(), tuote.alvprosentti() );

    double netto = tuote.ahinta();
    double brutto = netto * ( 100.0 + ui->alvCombo->veroProsentti() ) / 100.0;

    if( qAbs(qRound64(netto * 1000) - qRound64(netto*100)*10) > 0.005 ) {
        ui->bruttoEdit->setValue(brutto);
        laskeNetto();
    } else {
        ui->nettoEdit->setValue(netto);
        laskeBrutto();
    }

    ui->tiliEdit->valitseTiliNumerolla( tuote.tili() );
    ui->kohdennusCombo->valitseKohdennus( tuote.kohdennus() );
    ui->koodiEdit->setText( tuote.koodi() );

    show();
}

void TuoteDialogi::uusi()
{
    ui->kohdennusCombo->setCurrentIndex(
                ui->kohdennusCombo->findData(0, KohdennusModel::IdRooli));

    ui->yksikkoCombo->setUNkoodi("C62");
    ui->tiliEdit->valitseTiliNumerolla(kp()->asetukset()->luku("OletusMyyntitili"));

    muokattavanaId_ = 0;

    show();
}

void TuoteDialogi::accept()
{
    Tuote tuote;
    tuote.setId( muokattavanaId_ );
    tuote.nimiKielinen().aseta( ui->nimikeList->tekstit());
    if( ui->yksikkoCombo->unKoodi().isEmpty())
        tuote.setYksikko( ui->yksikkoCombo->yksikko() );
    else
        tuote.setUnKoodi( ui->yksikkoCombo->unKoodi());

    if( brutto_ > 1e-5) {
        double netto = (100.0 * brutto_) / (100.0 + ui->alvCombo->veroProsentti());
        tuote.setAhinta(netto);
    } else {
        tuote.setAhinta(ui->nettoEdit->value());
    }
    tuote.setAlvkoodi( ui->alvCombo->veroKoodi());
    tuote.setAlvprosentti( ui->alvCombo->veroProsentti());
    tuote.setTili( ui->tiliEdit->valittuTilinumero());
    tuote.setKohdennus( ui->kohdennusCombo->kohdennus());
    tuote.setKoodi( ui->koodiEdit->text());

    kp()->tuotteet()->paivitaTuote(tuote);
    QDialog::accept();
}

void TuoteDialogi::laskeBrutto()
{
    double netto = ui->nettoEdit->value();
    double brutto = netto * ( 100.0 + ui->alvCombo->veroProsentti() ) / 100.0;
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

    ui->nettoEdit->setValue(netto);
}

