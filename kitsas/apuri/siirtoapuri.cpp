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

    connect( ui->tililtaEraCombo, &EraCombo::valittu, [this] (int eraId, double avoinna, const QString& selite, int kumppani)
        {this->eraValittu(false, eraId, avoinna, selite, kumppani);});
    connect( ui->tililleEraCombo, &EraCombo::valittu, [this] (int eraId, double avoinna, const QString& selite, int kumppani)
        {this->eraValittu(true, eraId, avoinna, selite, kumppani);});

    ui->tililtaMerkkausCC->haeMerkkaukset( tosite->pvm() );
    ui->tililleMerkkausCC->haeMerkkaukset( tosite->pvm() );
    ui->tililtaKohdennusCombo->suodataPaivalla( tosite->pvm());
    ui->tililleKohdennusCombo->suodataPaivalla( tosite->pvm());

    connect( tosite, &Tosite::pvmMuuttui, ui->tililtaMerkkausCC, &CheckCombo::haeMerkkaukset );
    connect( tosite, &Tosite::pvmMuuttui, ui->tililleMerkkausCC, &CheckCombo::haeMerkkaukset );
    connect( tosite, &Tosite::pvmMuuttui, ui->tililtaKohdennusCombo, &KohdennusCombo::suodataPaivalla);
    connect( tosite, &Tosite::pvmMuuttui, ui->tililleKohdennusCombo, &KohdennusCombo::suodataPaivalla);    

    connect( tosite, &Tosite::pvmMuuttui, this, &SiirtoApuri::tositteelle);
    connect( tosite, &Tosite::otsikkoMuuttui, this, &SiirtoApuri::tositteelle);
}

SiirtoApuri::~SiirtoApuri()
{
    delete ui;
}

bool SiirtoApuri::teeTositteelle()
{

    qlonglong senttia = ui->euroEdit->asCents();

    QDate pvm = tosite()->data(Tosite::PVM).toDate();    
    QString otsikko = tosite()->otsikko();

    int kumppani = debetKumppani_ ? debetKumppani_ : kreditKumppani_;
    if( debetKumppani_ && kreditKumppani_ && debetKumppani_ != kreditKumppani_)
        kumppani = 0;


    QVariantList viennit;

    TositeVienti debet;
    debet.setPvm( pvm);
    debet.setTili( ui->tililleEdit->valittuTilinumero());
    debet.setDebet( senttia );
    debet.setSelite(otsikko);
    debet.setEra( ui->tililleEraCombo->eraMap());
    debet.setTyyppi( TositeVienti::SIIRTO);
    debet.setKumppani(kumppani);

    viennit.append(debet);

    TositeVienti kredit;
    kredit.setPvm( pvm );
    kredit.setTili( ui->tililtaEdit->valittuTilinumero());
    kredit.setKredit( senttia );
    kredit.setSelite(otsikko);
    kredit.setEra( ui->tililtaEraCombo->eraMap());
    kredit.setTyyppi( TositeVienti::SIIRTO );
    kredit.setKumppani(kumppani);

    viennit.append(kredit);

    if( !kreditAlkuperaiset_.isEmpty())
        erikoisviennit(kreditAlkuperaiset_, senttia / 100.0, viennit );
    if( !debetAlkuperaiset_.isEmpty())
        erikoisviennit(debetAlkuperaiset_, senttia / 100.0, viennit);

    tosite()->viennit()->asetaViennit(viennit);
    tosite()->asetaKumppani(kumppani);


    return true;

}

void SiirtoApuri::teeReset()
{
    QVariantList vientilista = tosite()->viennit()->viennit().toList();
    if( vientilista.count() >= 2 )
    {
        ui->tililleEdit->valitseTiliNumerolla( vientilista.at(0).toMap().value("tili").toInt() );
        ui->euroEdit->setValue( vientilista.at(0).toMap().value("debet").toDouble() );
        ui->tililtaEdit->valitseTiliNumerolla( vientilista.at(1).toMap().value("tili").toInt() );        
        ui->tililleEraCombo->valitse( vientilista.at(0).toMap().value("era").toMap().value("id").toInt() );
        debetKumppani_ = vientilista.at(0).toMap().value("kumppani").toMap().value("id").toInt();
        ui->tililtaEraCombo->valitse( vientilista.at(1).toMap().value("era").toMap().value("id").toInt() );        
        kreditKumppani_ = vientilista.at(1).toMap().value("kumppani").toMap().value("id").toInt();
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

void SiirtoApuri::haeAlkuperaistosite(bool debet, int eraId)
{
    KpKysely *kysely = kpk("/tositteet");
    kysely->lisaaAttribuutti("vienti", eraId);
    connect(kysely, &KpKysely::vastaus, [debet, this] (QVariant* data)
        { this->tositeSaapuu(debet, data); });
    kysely->kysy();
}

void SiirtoApuri::tositeSaapuu(bool debet, QVariant *data)
{
    QVariantList lista = data->toMap().value("viennit").toList();
    for(const auto& item : lista) {
        // Tarvitaan vain, jos maksuperusteisia alveja
        if( item.toMap().value("alvkoodi").toInt() / 100 == 4) {
            if( debet)
                debetAlkuperaiset_ = lista;
            else
                kreditAlkuperaiset_ = lista;
            teeTositteelle();
            break;
        }
    }
}

void SiirtoApuri::erikoisviennit(const QVariantList& alkupviennit, double eurot, QVariantList &viennit)
{
    double osuusErasta = 0.0;
    for(const auto& item : alkupviennit) {
        TositeVienti evienti(item.toMap());
        if( evienti.tyyppi() % 100 == TositeVienti::VASTAKIRJAUS && qAbs(eurot) > 1e-5) {
            osuusErasta = qAbs( eurot / (evienti.debet() - evienti.kredit()) );
        } else if( evienti.alvKoodi() / 100 == 4) {
            // Maksuperusteinen kohdentamaton
            qlonglong sentit = qRound64( osuusErasta * (evienti.kredit() - evienti.debet()) * 100.0);
            TositeVienti mpDebet;
            mpDebet.setPvm( tosite()->pvm() );
            mpDebet.setTili(evienti.tili());
            if( sentit > 0)
                mpDebet.setDebet(sentit);
            else
                mpDebet.setKredit(0-sentit);
            mpDebet.setSelite(tosite()->otsikko());
            mpDebet.setEra(evienti.era());
            mpDebet.setAlvKoodi(AlvKoodi::TILITYS);
            viennit.append(mpDebet);

            TositeVienti mpKredit;
            mpKredit.setPvm(tosite()->pvm());
            if( evienti.tili() == kp()->tilit()->tiliTyypilla(TiliLaji::KOHDENTAMATONALVVELKA).numero() ||
                evienti.alvKoodi() == AlvKoodi::ENNAKKOLASKU_MYYNTI + AlvKoodi::MAKSUPERUSTEINEN_KOHDENTAMATON) {
                mpKredit.setTili(kp()->tilit()->tiliTyypilla(TiliLaji::ALVVELKA).numero());
                mpKredit.setAlvKoodi(evienti.alvKoodi() % 100 + AlvKoodi::ALVKIRJAUS);
            } else if( evienti.tili() == kp()->tilit()->tiliTyypilla(TiliLaji::KOHDENTAMATONALVSAATAVA).numero()) {
                mpKredit.setTili(kp()->tilit()->tiliTyypilla(TiliLaji::ALVSAATAVA).numero());
                mpKredit.setAlvKoodi(evienti.alvKoodi() % 100 + AlvKoodi::ALVVAHENNYS);
            }
            mpKredit.setAlvProsentti(evienti.alvProsentti());
            if( sentit > 0)
                mpKredit.setKredit(sentit);
            else
                mpKredit.setDebet(0-sentit);
            mpKredit.setSelite(tosite()->otsikko());
            viennit.append(mpKredit);
        }
    }
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

void SiirtoApuri::eraValittu(bool debet, int eraId, double avoinna, const QString &selite, int kumppani)
{
    if( !ui->euroEdit->asCents() && avoinna > 1e-5)
        ui->euroEdit->setValue(avoinna);
    if( tosite()->otsikko().isEmpty())
        tosite()->asetaOtsikko(selite);

    if(debet)
        debetKumppani_ = kumppani;
    else
        kreditKumppani_ = kumppani;

    haeAlkuperaistosite(debet, eraId);
    teeTositteelle();
}

