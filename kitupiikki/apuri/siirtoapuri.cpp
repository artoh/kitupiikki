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
#include "siirtoapuri.h"
#include "ui_siirtoapuri.h"

#include "db/kirjanpito.h"
#include "db/kohdennusmodel.h"

#include "model/tosite.h"
#include "model/tositeviennit.h"

#include <QDebug>

SiirtoApuri::SiirtoApuri(QWidget *parent, Tosite *tosite) :
    ApuriWidget (parent, tosite),
    ui(new Ui::SiirtoApuri)
{
    ui->setupUi(this);

    bool merkkauksia = kp()->kohdennukset()->merkkauksia();
    ui->merkkausEdit->setVisible(merkkauksia);
    ui->merkkausLabel->setVisible(merkkauksia);

    connect( ui->tililtaEdit, &TilinvalintaLine::textChanged, this, &SiirtoApuri::tililtaMuuttui);
    connect( ui->tililleEdit, &TilinvalintaLine::textChanged, this, &SiirtoApuri::tililleMuuttui);
    connect( ui->euroEdit, &KpEuroEdit::textChanged, this, &SiirtoApuri::tositteelle);
}

SiirtoApuri::~SiirtoApuri()
{
    delete ui;
}

bool SiirtoApuri::tositteelle()
{
    if( resetointiKaynnissa_ )
        return false;

    double euroa = ui->euroEdit->value();
    QDate pvm = tosite()->data(Tosite::PVM).toDate();    
    QVariant otsikko = tosite()->data(Tosite::OTSIKKO);

    QVariantList viennit;

    QVariantMap debet;
    debet.insert("pvm", pvm );
    debet.insert("tili", ui->tililleEdit->valittuTilinumero());
    debet.insert("debet", euroa);
    debet.insert("selite", otsikko);
    viennit.append(debet);

    QVariantMap kredit;
    kredit.insert("pvm", pvm );
    kredit.insert("tili", ui->tililtaEdit->valittuTilinumero());
    kredit.insert("kredit", euroa);
    kredit.insert("selite", otsikko);
    viennit.append(kredit);

    tosite()->viennit()->asetaViennit(viennit);


    return true;

}

void SiirtoApuri::reset()
{
    resetointiKaynnissa_ = true;
    QVariantList vientilista = tosite()->viennit()->viennit().toList();
    if( vientilista.count() == 2 )
    {
        ui->tililleEdit->valitseTiliNumerolla( vientilista.at(0).toMap().value("tili").toInt() );
        ui->euroEdit->setValue( vientilista.at(0).toMap().value("debet").toDouble() );
        ui->tililtaEdit->valitseTiliNumerolla( vientilista.at(1).toMap().value("tili").toInt() );
    } else {
        ui->euroEdit->setCents(0);
        ui->tililleEdit->clear();
        ui->tililtaEdit->clear();
        tililtaMuuttui();
        tililleMuuttui();
    }
    resetointiKaynnissa_ = false;
}

void SiirtoApuri::tililtaMuuttui()
{
    Tili tili = ui->tililtaEdit->valittuTili();
    bool kohdennukset = kp()->kohdennukset()->kohdennuksia() &&
            tili.onko(TiliLaji::TULOS); // Tai kohdennettava tasetili;
    bool erat = tili.eritellaankoTase();

    ui->tililtaKohdennusLabel->setVisible(kohdennukset);
    ui->tililtaKohdennusCombo->setVisible(kohdennukset);

    ui->tililtaEraLabel->setVisible(erat);
    ui->tililtaEraCombo->setVisible(erat);

    tositteelle();
}

void SiirtoApuri::tililleMuuttui()
{    
    Tili tili = ui->tililleEdit->valittuTili();
    bool kohdennukset = kp()->kohdennukset()->kohdennuksia() &&
            tili.onko(TiliLaji::TULOS); // Tai kohdennettava tasetili;
    bool erat = tili.eritellaankoTase();

    ui->tililleKohdennusLabel->setVisible(kohdennukset);
    ui->tililleKohdennusCombo->setVisible(kohdennukset);

    ui->tililleEraLabel->setVisible(erat);
    ui->tililleEraCombo->setVisible(erat);

    tositteelle();
}
