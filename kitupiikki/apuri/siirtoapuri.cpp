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

    connect( ui->tililtaEdit, &TilinvalintaLine::textChanged, this, &SiirtoApuri::tililtaMuuttui);
    connect( ui->tililleEdit, &TilinvalintaLine::textChanged, this, &SiirtoApuri::tililleMuuttui);
    connect( ui->euroEdit, &KpEuroEdit::textChanged, this, &SiirtoApuri::tositteelle);

    connect( ui->tililtaEraCombo, &EraCombo::valittu, this, &SiirtoApuri::eraValittu);
    connect( ui->tililleEraCombo, &EraCombo::valittu, this, &SiirtoApuri::eraValittu);

    ui->tililtaMerkkausCC->haeMerkkaukset( tosite->pvm() );
    ui->tililleMerkkausCC->haeMerkkaukset( tosite->pvm() );
    ui->tililtaKohdennusCombo->suodataPaivalla( tosite->pvm());
    ui->tililleKohdennusCombo->suodataPaivalla( tosite->pvm());

    connect( tosite, &Tosite::pvmMuuttui, ui->tililtaMerkkausCC, &CheckCombo::haeMerkkaukset );
    connect( tosite, &Tosite::pvmMuuttui, ui->tililleMerkkausCC, &CheckCombo::haeMerkkaukset );
    connect( tosite, &Tosite::pvmMuuttui, ui->tililtaKohdennusCombo, &KohdennusCombo::suodataPaivalla);
    connect( tosite, &Tosite::pvmMuuttui, ui->tililleKohdennusCombo, &KohdennusCombo::suodataPaivalla);
}

SiirtoApuri::~SiirtoApuri()
{
    delete ui;
}

bool SiirtoApuri::teeTositteelle()
{


    qlonglong senttia = ui->euroEdit->asCents();

    QDate pvm = tosite()->data(Tosite::PVM).toDate();    
    QVariant otsikko = tosite()->data(Tosite::OTSIKKO);

    QVariantList viennit;

    TositeVienti debet;
    debet.setPvm( pvm);
    debet.setTili( ui->tililleEdit->valittuTilinumero());
    debet.setDebet( senttia );
    debet.set(TositeVienti::SELITE, otsikko);
    debet.setEra( ui->tililleEraCombo->valittuEra() );
    debet.setTyyppi( TositeVienti::SIIRTO);
    viennit.append(debet);

    TositeVienti kredit;
    kredit.setPvm( pvm );
    kredit.setTili( ui->tililtaEdit->valittuTilinumero());
    kredit.setKredit( senttia );
    kredit.set( TositeVienti::SELITE, otsikko);
    kredit.setEra( ui->tililtaEraCombo->valittuEra());
    kredit.setTyyppi( TositeVienti::SIIRTO );
    viennit.append(kredit);

    tosite()->viennit()->asetaViennit(viennit);


    return true;

}

void SiirtoApuri::teeReset()
{
    QVariantList vientilista = tosite()->viennit()->viennit().toList();
    if( vientilista.count() == 2 )
    {
        ui->tililleEdit->valitseTiliNumerolla( vientilista.at(0).toMap().value("tili").toInt() );
        ui->euroEdit->setValue( vientilista.at(0).toMap().value("debet").toDouble() );
        ui->tililtaEdit->valitseTiliNumerolla( vientilista.at(1).toMap().value("tili").toInt() );

        ui->tililleEraCombo->valitse( vientilista.at(0).toMap().value("era").toMap().value("id").toInt() );
        ui->tililtaEraCombo->valitse( vientilista.at(1).toMap().value("era").toMap().value("id").toInt() );
    } else {
        ui->euroEdit->setCents(0);
        ui->tililleEdit->clear();
        ui->tililtaEdit->clear();
        tililtaMuuttui();
        tililleMuuttui();
    }

}

void SiirtoApuri::paivitaKateislaji()
{
    Tili tililta = ui->tililtaEdit->valittuTili();
    Tili tilille = ui->tililleEdit->valittuTili();

    emit tosite()->tarkastaSarja( tililta.onko(TiliLaji::KATEINEN) ||
                                tilille.onko(TiliLaji::KATEINEN));

}

void SiirtoApuri::otaFokus()
{
    ui->tililtaEdit->setFocus(Qt::TabFocusReason);
}

void SiirtoApuri::tililtaMuuttui()
{
    Tili tili = ui->tililtaEdit->valittuTili();
    bool kohdennukset = kp()->kohdennukset()->kohdennuksia() &&
            tili.onko(TiliLaji::TULOS);
    bool merkkaukset = kp()->kohdennukset()->merkkauksia() &&
            tili.onko( TiliLaji::TULOS);
    bool erat = tili.eritellaankoTase();

    ui->tililtaKohdennusLabel->setVisible(kohdennukset);
    ui->tililtaKohdennusCombo->setVisible(kohdennukset);
    ui->tililtaMerkkausLabel->setVisible(merkkaukset);
    ui->tililtaMerkkausCC->setVisible(merkkaukset);

    ui->tililtaEraLabel->setVisible(erat);
    ui->tililtaEraCombo->setVisible(erat);
    if( erat )
        ui->tililtaEraCombo->lataa( tili.numero() );

    tositteelle();
    paivitaKateislaji();
}

void SiirtoApuri::tililleMuuttui()
{    
    Tili tili = ui->tililleEdit->valittuTili();
    bool kohdennukset = kp()->kohdennukset()->kohdennuksia() &&
            tili.onko(TiliLaji::TULOS);
    bool merkkaukset = kp()->kohdennukset()->merkkauksia() &&
            tili.onko( TiliLaji::TULOS);
    bool erat = tili.eritellaankoTase();

    ui->tililleKohdennusLabel->setVisible(kohdennukset);
    ui->tililleKohdennusCombo->setVisible(kohdennukset);
    ui->tililleMerkkausLabel->setVisible(merkkaukset);
    ui->tililleMerkkausCC->setVisible(merkkaukset);

    ui->tililleEraLabel->setVisible(erat);
    ui->tililleEraCombo->setVisible(erat);
    if( erat )
        ui->tililleEraCombo->lataa(tili.numero());

    tositteelle();
    paivitaKateislaji();
}

void SiirtoApuri::eraValittu(int /* eraId */, double avoinna)
{
    if( !ui->euroEdit->asCents() && avoinna > 1e-5)
        ui->euroEdit->setValue(avoinna);

    tositteelle();
}

