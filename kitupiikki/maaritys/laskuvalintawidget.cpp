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

#include "laskuvalintawidget.h"
#include "db/tositelajimodel.h"

#include "laskutus/laskumodel.h"

#include <QDebug>

LaskuValintaWidget::LaskuValintaWidget()
    : MaaritysWidget(),
      ui(new Ui::LaskuValinnat)
{
    ui->setupUi(this);

    ui->tositelajiCombo->setModel( kp()->tositelajit() );
    ui->saatavatiliEdit->suodataTyypilla("AS");
    ui->kateistiliEdit->suodataTyypilla("AR");

    ui->perusteCombo->addItem("Suoriteperusteinen", LaskuModel::SUORITEPERUSTE);
    ui->perusteCombo->addItem("Laskutusperusteinen", LaskuModel::LASKUTUSPERUSTE);
    ui->perusteCombo->addItem("Maksuperusteinen", LaskuModel::MAKSUPERUSTE);
    ui->perusteCombo->addItem("Käteiskuitti", LaskuModel::KATEISLASKU);

    connect( ui->tositelajiCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(ilmoitaMuokattu()));
    connect(ui->perusteCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(ilmoitaMuokattu()));
    connect(ui->saatavatiliEdit, SIGNAL(textChanged(QString)), this, SLOT(ilmoitaMuokattu()));
    connect(ui->kateistiliEdit, SIGNAL(textChanged(QString)), this, SLOT(ilmoitaMuokattu()));
    connect(ui->maksuaikaSpin, SIGNAL(valueChanged(int)), this, SLOT(ilmoitaMuokattu()));
    connect(ui->huomautusaikaEdit, SIGNAL(textChanged(QString)), this, SLOT(ilmoitaMuokattu()));
    connect(ui->viivastyskorkoEdit, SIGNAL(textChanged(QString)), this, SLOT(ilmoitaMuokattu()));
    connect(ui->seuraavaLasku, SIGNAL(valueChanged(int)), this, SLOT(ilmoitaMuokattu()));
    connect(ui->ibanEdit, SIGNAL(textChanged(QString)), this, SLOT(ilmoitaMuokattu()));
}

LaskuValintaWidget::~LaskuValintaWidget()
{
    delete ui;
}


bool LaskuValintaWidget::nollaa()
{
    ui->tositelajiCombo->setCurrentIndex( ui->tositelajiCombo->findData( kp()->asetukset()->luku("LaskuTositelaji") ) );
    ui->perusteCombo->setCurrentIndex( ui->perusteCombo->findData( kp()->asetukset()->luku("LaskuKirjausperuste", LaskuModel::SUORITEPERUSTE)));
    ui->saatavatiliEdit->valitseTiliNumerolla( kp()->asetukset()->luku("LaskuSaatavatili"));
    ui->kateistiliEdit->valitseTiliNumerolla( kp()->asetukset()->luku("LaskuKateistili"));
    ui->maksuaikaSpin->setValue( kp()->asetukset()->luku("LaskuMaksuaika", 14) );
    ui->huomautusaikaEdit->setText( kp()->asetus("LaskuHuomautusaika"));
    ui->viivastyskorkoEdit->setText( kp()->asetus("LaskuViivastyskorko"));
    ui->seuraavaLasku->setValue( kp()->asetukset()->luku("LaskuSeuraavaId", 1000));
    ui->ibanEdit->setText( kp()->asetus("IBAN"));
    return true;
}

bool LaskuValintaWidget::tallenna()
{
    kp()->asetukset()->aseta("LaskuTositelaji", ui->tositelajiCombo->currentData().toInt());
    kp()->asetukset()->aseta("LaskuKirjausperuste", ui->perusteCombo->currentData().toInt());
    kp()->asetukset()->aseta("LaskuSaatavatili", ui->saatavatiliEdit->valittuTilinumero());
    kp()->asetukset()->aseta("LaskuKateistili", ui->kateistiliEdit->valittuTilinumero());
    kp()->asetukset()->aseta("LaskuMaksuaika", ui->maksuaikaSpin->value());
    kp()->asetukset()->aseta("LaskuHuomautusaika", ui->huomautusaikaEdit->text());
    kp()->asetukset()->aseta("LaskuViivastyskorko", ui->viivastyskorkoEdit->text());
    kp()->asetukset()->aseta("LaskuSeuraavaId", ui->seuraavaLasku->value());
    kp()->asetukset()->aseta("IBAN", ui->ibanEdit->text());

    return true;
}

bool LaskuValintaWidget::onkoMuokattu()
{
    return ui->tositelajiCombo->currentData().toInt() != kp()->asetukset()->luku("LaskuTositelaji") ||
            ui->perusteCombo->currentData().toInt() != kp()->asetukset()->luku("LaskuKirjausperuste") ||
            ui->saatavatiliEdit->valittuTilinumero() != kp()->asetukset()->luku("LaskuSaatavatili") ||
            ui->kateistiliEdit->valittuTilinumero() != kp()->asetukset()->luku("LaskuKateistili") ||
            ui->maksuaikaSpin->value() != kp()->asetukset()->luku("LaskuMaksuaika") ||
            ui->huomautusaikaEdit->text() != kp()->asetukset()->asetus("LaskuHuomautusaika") ||
            ui->viivastyskorkoEdit->text() != kp()->asetukset()->asetus("LaskuViivastyskorko") ||
            ui->seuraavaLasku->value() != kp()->asetukset()->luku("LaskuSeuraavaId") ||
            ui->ibanEdit->text() != kp()->asetus("IBAN") ;

}



void LaskuValintaWidget::ilmoitaMuokattu()
{
    emit tallennaKaytossa( onkoMuokattu() );
}
