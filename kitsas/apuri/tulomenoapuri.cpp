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
#include "tulomenoapuri.h"
#include "ui_tulomenoapuri.h"
#include "tmrivit.h"

#include "db/kirjanpito.h"
#include "model/tosite.h"
#include "model/tositeviennit.h"
#include "db/tositetyyppimodel.h"
#include "rekisteri/asiakastoimittajadlg.h"
#include "model/maksutapamodel.h"

#include <QSortFilterProxyModel>
#include <QDebug>
#include <QJsonDocument>

TuloMenoApuri::TuloMenoApuri(QWidget *parent, Tosite *tosite) :
    ApuriWidget (parent, tosite),
    ui(new Ui::TuloMenoApuri),
    rivit_(new TmRivit(this)),
    maksutapaModel_(new MaksutapaModel(this))
{
    ui->setupUi(this);

    veroFiltteri_ = new QSortFilterProxyModel(this);
    veroFiltteri_->setFilterRole( VerotyyppiModel::KoodiTekstiRooli);
    veroFiltteri_->setSourceModel( kp()->alvTyypit());
    ui->alvCombo->setModel(veroFiltteri_);

    ui->alkuEdit->setNull();
    ui->loppuEdit->setNull();
    ui->erapaivaEdit->setNull();

    ui->maksutapaCombo->setModel( maksutapaModel_ );

    ui->tilellaView->setModel( rivit_);
    ui->tilellaView->horizontalHeader()->setSectionResizeMode(TmRivit::TILI, QHeaderView::Stretch);
    if( !kp()->asetukset()->onko(AsetusModel::ALV))
        ui->tilellaView->hideColumn(TmRivit::ALV);


    connect( ui->tiliEdit, &TilinvalintaLine::textChanged, this, &TuloMenoApuri::tiliMuuttui );

    connect( ui->maaraEdit, &KpEuroEdit::textEdited, this, &TuloMenoApuri::maaraMuuttui);
    connect( ui->verotonEdit, &KpEuroEdit::textEdited, this, &TuloMenoApuri::verotonMuuttui);

    connect( ui->alvSpin, SIGNAL( valueChanged(double) ), this, SLOT( veroprossaMuuttui()) );

    connect( ui->lisaaRiviNappi, &QPushButton::clicked, this, &TuloMenoApuri::lisaaRivi);
    connect( ui->poistaRiviNappi, &QPushButton::clicked, this, &TuloMenoApuri::poistaRivi);

    connect( ui->tilellaView->selectionModel(), &QItemSelectionModel::currentRowChanged , this, &TuloMenoApuri::haeRivi);
    connect( ui->seliteEdit, &QLineEdit::textChanged, this, &TuloMenoApuri::seliteMuuttui);
    connect( ui->alvCombo, &QComboBox::currentTextChanged, this, &TuloMenoApuri::verolajiMuuttui);
    connect( ui->vahennysCheck, &QCheckBox::stateChanged, this, &TuloMenoApuri::alvVahennettavaMuuttui);

    connect( ui->kohdennusCombo, &KohdennusCombo::kohdennusVaihtui, this, &TuloMenoApuri::kohdennusMuuttui);
    connect( ui->merkkauksetCC, &CheckCombo::currentTextChanged, this, &TuloMenoApuri::merkkausMuuttui );

    connect( ui->alkuEdit, &KpDateEdit::dateChanged, this, &TuloMenoApuri::jaksoAlkaaMuuttui);
    connect( ui->loppuEdit, &KpDateEdit::dateChanged, this, &TuloMenoApuri::jaksoLoppuuMuuttui);
    connect( ui->poistoSpin, SIGNAL(valueChanged(int)), this, SLOT(poistoAikaMuuttuu()));

    connect( ui->maksutapaCombo, &QComboBox::currentTextChanged, this, &TuloMenoApuri::maksutapaMuuttui);
    connect( ui->eraCombo, &QComboBox::currentTextChanged, this, &TuloMenoApuri::vastatiliMuuttui);
    connect( ui->vastatiliLine, &TilinvalintaLine::textChanged, this, &TuloMenoApuri::vastatiliMuuttui);

    connect( ui->viiteEdit, &QLineEdit::textEdited, this, &TuloMenoApuri::tositteelle);
    connect( ui->laskuPvm, &KpDateEdit::dateChanged, this, &TuloMenoApuri::tositteelle);
    connect( ui->erapaivaEdit, &KpDateEdit::dateChanged, this, &TuloMenoApuri::tositteelle);

    connect( tosite, &Tosite::pvmMuuttui, this, &TuloMenoApuri::haeKohdennukset );
    connect( tosite, &Tosite::pvmMuuttui, this, &TuloMenoApuri::paivitaVeroFiltterit);

    connect( ui->asiakasToimittaja, &AsiakasToimittajaValinta::muuttui, this, &TuloMenoApuri::tositteelle);
    connect( ui->asiakasToimittaja, &AsiakasToimittajaValinta::valittu, this, &TuloMenoApuri::kumppaniValittu);

    connect( ui->vastatiliLine, &TilinvalintaLine::textChanged, this, &TuloMenoApuri::vastatiliMuuttui);
    connect( tosite, &Tosite::pvmMuuttui, ui->laskuPvm, &KpDateEdit::setDate);
    connect( tosite, &Tosite::pvmMuuttui, this, &TuloMenoApuri::tositteelle);
    connect( ui->eraCombo, &EraCombo::currentTextChanged, this, &TuloMenoApuri::tositteelle);
}

TuloMenoApuri::~TuloMenoApuri()
{
    delete ui;
}

void TuloMenoApuri::otaFokus()
{
    ui->maksutapaCombo->setFocus();
}

void TuloMenoApuri::tuo(QVariantMap map)
{
    if( qAbs(map.value("summa").toDouble()) > 1e-5) {
        ui->maaraEdit->setValue( map.value("summa").toDouble());
        emit ui->maaraEdit->textEdited( ui->maaraEdit->text() );
    }
    if( !map.value("viite").toString().isEmpty())
        ui->viiteEdit->setText( map.value("viite").toString() );

    if( !map.value("kumppaninimi").toString().isEmpty() || !map.value("kumppaniytunnus").toString().isEmpty())
        ui->asiakasToimittaja->tuonti( map );

    if( map.value("erapvm").isValid())
        ui->erapaivaEdit->setDate( map.value("erapvm").toDate());

    tositteelle();
}

void TuloMenoApuri::teeReset()
{

    // Haetaan tietoja mallista ;)
    bool menoa = tosite()->tyyppi() == TositeTyyppi::MENO ||
                 tosite()->tyyppi() == TositeTyyppi::KULULASKU;

    alusta( menoa );


    rivit_->clear();
    ui->viiteEdit->clear();    
    ui->erapaivaEdit->setNull();

    ui->viiteEdit->setText( tosite()->viite());
    ui->laskuPvm->setDate( tosite()->laskupvm());
    ui->erapaivaEdit->setDate( tosite()->erapvm());

    if( tosite()->kumppani())
        ui->asiakasToimittaja->set(tosite()->kumppani(), tosite()->kumppaninimi());
    else
        ui->asiakasToimittaja->clear();

    QVariantList vientiLista = tosite()->viennit()->viennit().toList();

    for( auto item : tosite()->viennit()->viennit().toList()) {

        // Jos vastakirjaus käsitellään itse
        // Muuten riveille
        TositeVienti vienti( item.toMap() );

        if( vienti.tyyppi() % 100 == TositeVienti::VASTAKIRJAUS) {
            Tili* vastatili = kp()->tilit()->tili( vienti.tili());
            if( vastatili ) {

                int maksutapaind = ui->maksutapaCombo->findData(vastatili->numero(), MaksutapaModel::TiliRooli);
                if( maksutapaind >= 0)
                    ui->maksutapaCombo->setCurrentIndex(maksutapaind);
                else
                    ui->maksutapaCombo->setCurrentIndex(ui->maksutapaCombo->count()-1);
                maksutapaMuuttui();

                ui->vastatiliLine->valitseTiliNumerolla( vastatili->numero() );

                if( vastatili->eritellaankoTase())
                    ui->eraCombo->valitse( vienti.eraId() );
            }

        } else {
            rivit_->lisaa(vienti);
        }

    }
    if( !rivit_->rowCount())
        rivit_->lisaaRivi();    

    ui->tilellaView->setVisible( rivit_->rowCount() > 1 );
    ui->poistaRiviNappi->setEnabled( rivit_->rowCount() > 1 );
    ui->tilellaView->selectRow(0);    


    tiliMuuttui();    


}

bool TuloMenoApuri::teeTositteelle()
{        
    qlonglong summa = 0l;

    QVariantList viennit = rivit_->viennit( tosite() );

    for( auto vienti : viennit) {
        QVariantMap map = vienti.toMap();
        summa += qRound64( map.value("kredit",0).toDouble() * 100.0 );
        summa -= qRound64( map.value("debet",0).toDouble() * 100.0 );
    }

    if( summa ) {
        bool menoa = tosite()->tyyppi() == TositeTyyppi::MENO ||
                     tosite()->tyyppi() == TositeTyyppi::KULULASKU;

        QString otsikko = tosite()->otsikko();
        if( otsikko.isEmpty()) {
            otsikko = ui->asiakasToimittaja->nimi();
        }

        TositeVienti vasta;
        if( tosite()->viennit()->rowCount() && tosite()->viennit()->vienti(0).tyyppi() % 100 == TositeVienti::VASTAKIRJAUS)
            vasta.setId( tosite()->viennit()->vienti(0).id() );

        vasta.setTyyppi( (menoa ? TositeVienti::OSTO : TositeVienti::MYYNTI) + TositeVienti::VASTAKIRJAUS );        
        vasta.setPvm( tosite()->pvm());
        Tili vastatili = kp()->tilit()->tiliNumerolla( ui->vastatiliLine->valittuTilinumero() );
        vasta.insert("tili", vastatili.numero() );

        qDebug() << " Vastatili " << vastatili.numero() << " erittely "  << vastatili.eritellaankoTase() << " Erä " << ui->eraCombo->valittuEra();

        if( vastatili.eritellaankoTase())
            vasta.setEra(ui->eraCombo->valittuEra() ) ;

        if( summa > 0)
            vasta.setDebet( summa );
        else
            vasta.setKredit( 0 - summa );

        vasta.insert("selite", otsikko);


        // Asiakas tai toimittaja
        if( ui->asiakasToimittaja->id() > 0)
            vasta.setKumppani( ui->asiakasToimittaja->id() );
        else if( !ui->asiakasToimittaja->nimi().isEmpty())
            vasta.setKumppani( ui->asiakasToimittaja->nimi());

        viennit.insert(0, vasta);
    }

    if( ui->asiakasToimittaja->id() > 0)
        tosite()->asetaKumppani( ui->asiakasToimittaja->id() );
    else if( !ui->asiakasToimittaja->nimi().isEmpty())
        tosite()->asetaKumppani( ui->asiakasToimittaja->nimi() );
    else
        tosite()->asetaKumppani( QVariantMap());

    tosite()->asetaLaskupvm( ui->laskuPvm->date());
    tosite()->asetaErapvm( ui->erapaivaEdit->date());
    tosite()->asetaViite( ui->viiteEdit->text());

    tosite()->viennit()->asetaViennit(viennit);

    if(summa)
        viimeMaksutapa__ = ui->maksutapaCombo->currentText();

    return true;
}

void TuloMenoApuri::lisaaRivi()
{
    int tili = menoa_ ? kp()->asetukset()->luku("OletusMenotili") : kp()->asetukset()->luku("OletusMyyntiluku");

    ui->tilellaView->setVisible(true);
    ui->tilellaView->selectRow( rivit_->lisaaRivi(tili) );
    ui->poistaRiviNappi->setEnabled( true );    // Rivejä vähintään kaksi kun juuri lisätty
}

void TuloMenoApuri::poistaRivi()
{    
    rivit_->poistaRivi( rivilla() );

    ui->poistaRiviNappi->setEnabled( rivit_->rowCount() > 1 );
}

void TuloMenoApuri::tiliMuuttui()
{
    if( !rivit_->rowCount())
        return;

    Tili tili = ui->tiliEdit->valittuTili();
    rivi()->setTili(tili.numero());

    bool tasapoisto = tili.onko(TiliLaji::TASAERAPOISTO);
    ui->poistoLabel->setVisible(tasapoisto);
    ui->poistoSpin->setVisible(tasapoisto);

    if( !resetoidaanko() || !tosite()->id()) {

        if( tasapoisto ) {
            ui->poistoSpin->setValue( tili.luku("tasaerapoisto") / 12 );
            poistoAikaMuuttuu();
        }

        if( kp()->asetukset()->onko(AsetusModel::ALV) )
        {
            int verotyyppi = tili.luku("alvlaji");
            bool maksuperuste = kp()->onkoMaksuperusteinenAlv(tosite()->pvm()) && ( ui->vastatiliLine->valittuTili().onko(TiliLaji::OSTOVELKA)
                                                                || ui->vastatiliLine->valittuTili().onko(TiliLaji::MYYNTISAATAVA));

            if( verotyyppi == AlvKoodi::OSTOT_NETTO && maksuperuste)
                verotyyppi = AlvKoodi::MAKSUPERUSTEINEN_OSTO;
            if( verotyyppi == AlvKoodi::MYYNNIT_NETTO && maksuperuste)
                verotyyppi = AlvKoodi::MAKSUPERUSTEINEN_MYYNTI;

            ui->alvCombo->setCurrentIndex( ui->alvCombo->findData( verotyyppi, VerotyyppiModel::KoodiRooli ) );
            ui->alvSpin->setValue( tili.str("alvprosentti").toDouble() );
        }

        if( tili.luku("vastatili") && rivit_->rowCount()<2) {
            int vastatili = tili.luku("vastatili");
            ui->vastatiliLine->valitseTiliNumerolla(vastatili);
            int maksutapaind = ui->maksutapaCombo->findData(vastatili, MaksutapaModel::TiliRooli);
            if( maksutapaind >= 0)
                ui->maksutapaCombo->setCurrentIndex(vastatili);
            else
                ui->maksutapaCombo->setCurrentIndex(ui->maksutapaCombo->count()-1);
        }

        emit rivit_->dataChanged( rivit_->index(TmRivit::TILI, rivilla()),
                                  rivit_->index(TmRivit::TILI, rivilla()));

        tositteelle();
    }

}

void TuloMenoApuri::verolajiMuuttui()
{    

    int alvkoodi = ui->alvCombo->currentData( VerotyyppiModel::KoodiRooli ).toInt();
    rivi()->setAlvkoodi(  alvkoodi );

    qDebug() << "alvkoodi " << alvkoodi;

    bool naytaMaara = rivi()->naytaBrutto();
    bool naytaVeroton =  rivi()->naytaNetto();

    ui->maaraLabel->setVisible(naytaMaara);
    ui->maaraEdit->setVisible(naytaMaara);

    ui->verotonLabel->setVisible(naytaVeroton);
    ui->verotonEdit->setVisible(naytaVeroton);

    ui->alvSpin->setVisible( !ui->alvCombo->currentData(VerotyyppiModel::NollaLajiRooli).toBool() );
    ui->vahennysCheck->setVisible( rivi()->naytaVahennysvalinta());
    ui->vahennysCheck->setChecked( false );

    if( ui->alvCombo->currentData(VerotyyppiModel::NollaLajiRooli).toBool() ) {
        ui->alvSpin->setValue(0.0);
    } else  {
        if( ui->alvSpin->value() == 0.0)
            ui->alvSpin->setValue(24.0);
    }


    tositteelle();
}

void TuloMenoApuri::maaraMuuttui()
{
    qlonglong maara = ui->maaraEdit->asCents();
    rivi()->setBrutto( maara );
    ui->verotonEdit->setCents( rivi()->netto() );

    emit rivit_->dataChanged( rivit_->index(TmRivit::EUROA, rivilla()),
                              rivit_->index(TmRivit::EUROA, rivilla()));

    tositteelle();
}

void TuloMenoApuri::verotonMuuttui()
{
    qlonglong veroton = ui->verotonEdit->asCents();

    rivi()->setNetto( veroton );
    ui->maaraEdit->setCents( rivi()->brutto());

    emit rivit_->dataChanged( rivit_->index(TmRivit::EUROA, rivilla()),
                              rivit_->index(TmRivit::EUROA, rivilla()));


    tositteelle();
}

void TuloMenoApuri::veroprossaMuuttui()
{
    double verokanta = ui->alvSpin->value();
    rivi()->setAlvprosentti( verokanta  );
    ui->maaraEdit->setCents( rivi()->brutto());
    ui->verotonEdit->setCents( rivi()->netto() );
    tositteelle();

    emit rivit_->dataChanged( rivit_->index(TmRivit::ALV, rivilla()),
                              rivit_->index(TmRivit::ALV, rivilla()));
}

void TuloMenoApuri::alvVahennettavaMuuttui()
{
    rivi()->setAlvvahennys( !ui->vahennysCheck->isChecked() );
    tositteelle();
}

void TuloMenoApuri::seliteMuuttui()
{
    rivi()->setSelite(  ui->seliteEdit->text());
    tositteelle();
}

void TuloMenoApuri::maksutapaMuuttui()
{
    int maksutapatili = ui->maksutapaCombo->currentData(MaksutapaModel::TiliRooli).toInt();

    if( maksutapatili)
        ui->vastatiliLine->valitseTiliNumerolla(maksutapatili);

    ui->vastatiliLabel->setVisible( !maksutapatili  );
    ui->vastatiliLine->setVisible( !maksutapatili );

    vastatiliMuuttui();

    emit tosite()->tarkastaSarja( kp()->tilit()->tiliNumerolla(maksutapatili).onko(TiliLaji::KATEINEN) );
}

void TuloMenoApuri::vastatiliMuuttui()
{
    Tili vastatili = kp()->tilit()->tiliNumerolla( ui->vastatiliLine->valittuTilinumero() );

    bool eritellankotaso = vastatili.eritellaankoTase() && !ui->maksutapaCombo->currentData(MaksutapaModel::UusiEraRooli).toBool();

    ui->eraLabel->setVisible( eritellankotaso);
    ui->eraCombo->setVisible( eritellankotaso);
    ui->eraCombo->lataa( vastatili.numero() , ui->asiakasToimittaja->id());
    if( (vastatili.eritellaankoTase() || ui->maksutapaCombo->currentData(MaksutapaModel::UusiEraRooli).toBool()) && sender() != ui->eraCombo) {
        ui->eraCombo->valitse(-1);
    }

    bool laskulle = (vastatili.onko(TiliLaji::OSTOVELKA) || vastatili.onko(TiliLaji::MYYNTISAATAVA)) &&
            ui->eraCombo->valittuEra() == -1;
    ui->viiteLabel->setVisible( laskulle );
    ui->viiteEdit->setVisible( laskulle );

    ui->laskupvmLabel->setVisible( laskulle );
    ui->laskuPvm->setVisible( laskulle );
    ui->erapaivaLabel->setVisible( laskulle );
    ui->erapaivaEdit->setVisible( laskulle );    

    emit tosite()->tarkastaSarja( vastatili.onko(TiliLaji::KATEINEN));
    paivitaVeroFiltterit(tosite()->pvm());

    tositteelle();
}

void TuloMenoApuri::kohdennusMuuttui()
{
    rivi()->setKohdennus( ui->kohdennusCombo->kohdennus());
    tositteelle();    
}

void TuloMenoApuri::merkkausMuuttui()
{
    rivi()->setMerkkaukset( ui->merkkauksetCC->selectedDatas());
    tositteelle();

}

void TuloMenoApuri::jaksoAlkaaMuuttui()
{
    QDate alkupvm = ui->alkuEdit->date();
    rivi()->setJaksoalkaa( alkupvm);
    ui->loppuEdit->setEnabled( alkupvm.isValid() );
    tositteelle();
}

void TuloMenoApuri::jaksoLoppuuMuuttui()
{
    rivi()->setJaksopaattyy( ui->loppuEdit->date());
    tositteelle();
}

void TuloMenoApuri::poistoAikaMuuttuu()
{
    rivi()->setPoistoaika(  ui->poistoSpin->value() * 12);
    tositteelle();
}

void TuloMenoApuri::paivitaVeroFiltterit(const QDate &pvm)
{
    bool maksuperuste = kp()->onkoMaksuperusteinenAlv(pvm) && ( ui->vastatiliLine->valittuTili().onko(TiliLaji::OSTOVELKA)
                                                        || ui->vastatiliLine->valittuTili().onko(TiliLaji::MYYNTISAATAVA));
    int verokoodi = ui->alvCombo->currentData(VerotyyppiModel::KoodiRooli).toInt();
    if( menoa_) {
        veroFiltteri_->setFilterRegExp(  maksuperuste ?
                                            "^(0|2[4-8]|927)"
                                            : "^(0|2[1-7]|927)");
        if( verokoodi == AlvKoodi::OSTOT_NETTO && maksuperuste)
            ui->alvCombo->setCurrentIndex( ui->alvCombo->findData(AlvKoodi::MAKSUPERUSTEINEN_OSTO, VerotyyppiModel::KoodiRooli) );
        else if( verokoodi == AlvKoodi::MAKSUPERUSTEINEN_OSTO && !maksuperuste)
            ui->alvCombo->setCurrentIndex( ui->alvCombo->findData(AlvKoodi::OSTOT_NETTO, VerotyyppiModel::KoodiRooli) );
    } else {
        veroFiltteri_->setFilterRegExp( maksuperuste ?
                                           "^(0|1[4-8])"
                                           : "^(0|1[1-7])");
        if(verokoodi == AlvKoodi::MYYNNIT_NETTO && maksuperuste)
            ui->alvCombo->setCurrentIndex( ui->alvCombo->findData(AlvKoodi::MAKSUPERUSTEINEN_MYYNTI, VerotyyppiModel::KoodiRooli) );
        else if(verokoodi == AlvKoodi::MAKSUPERUSTEINEN_MYYNTI && !maksuperuste)
            ui->alvCombo->setCurrentIndex( ui->alvCombo->findData(AlvKoodi::MYYNNIT_NETTO, VerotyyppiModel::KoodiRooli) );
    }
}

void TuloMenoApuri::haeRivi(const QModelIndex &index)
{
    aloitaResetointi();
    int rivilla = index.row();

    TulomenoRivi* rivi = rivit_->rivi(rivilla);
    int tilinumero = rivi->tilinumero();

    qDebug() << "Hae " << rivi->netto() << " " << rivi->alvkoodi() << " " << rivi->viennit( tosite() );

    if( !tilinumero) {
        if( tosite()->tyyppi() == TositeTyyppi::TULO )
            tilinumero = kp()->tilit()->tiliTyypilla(TiliLaji::LVTULO).numero();
        else
            tilinumero = kp()->tilit()->tiliTyypilla(TiliLaji::MENO).numero();
    }

    ui->tiliEdit->valitseTiliNumerolla( tilinumero );
    tiliMuuttui();

    ui->alvSpin->setValue( rivi->alvprosentti() );
    ui->alvCombo->setCurrentIndex( ui->alvCombo->findData( rivi->alvkoodi(), VerotyyppiModel::KoodiRooli ) );

    verolajiMuuttui();
    ui->maaraEdit->setCents( rivi->brutto() );
    ui->verotonEdit->setCents( rivi->netto());

    ui->vahennysCheck->setChecked( !rivi->alvvahennys() );

    ui->poistoSpin->setValue( rivi->poistoaika() / 12 );

    ui->alkuEdit->setDate( rivi->jaksoalkaa() );
    ui->loppuEdit->setEnabled( rivi->jaksoalkaa().isValid());
    ui->loppuEdit->setDate( rivi->jaksopaattyy());


    ui->seliteEdit->setText( rivi->selite() );

    haeKohdennukset();
    lopetaResetointi();
}

void TuloMenoApuri::haeKohdennukset()
{
    int nykyinenKohdennus = rivit_->rowCount() ? rivi()->kohdennus(  ) : 0 ;
    QVariantList merkatut =  rivit_->rowCount() ?  rivi()->merkkaukset( ) : QVariantList();
    QDate pvm = tosite()->data(Tosite::PVM).toDate();

    ui->kohdennusCombo->valitseKohdennus(nykyinenKohdennus);
    ui->kohdennusCombo->suodataPaivalla(pvm);

    ui->kohdennusLabel->setVisible( ui->kohdennusCombo->count() > 1);
    ui->kohdennusCombo->setVisible( ui->kohdennusCombo->count() > 1);

    ui->merkkauksetCC->haeMerkkaukset(pvm);

    ui->merkkauksetLabel->setVisible( ui->merkkauksetCC->count());
    ui->merkkauksetCC->setVisible( ui->merkkauksetCC->count());

    ui->merkkauksetCC->setSelectedItems( merkatut );
}


void TuloMenoApuri::alusta(bool meno)
{
    menoa_ = meno;

    if(meno) {
        ui->tiliLabel->setText( tr("Meno&tili") );        
        ui->tiliEdit->suodataTyypilla("(AP|D).*");
        ui->toimittajaLabel->setText( tr("Toimittaja"));
        if( tosite()->tyyppi() == TositeTyyppi::KULULASKU )
            ui->toimittajaLabel->setText( tr("Laskuttaja"));
    } else {
        ui->tiliLabel->setText( tr("Tulo&tili"));
        ui->tiliEdit->suodataTyypilla("(AP|C).*");

        ui->toimittajaLabel->setText( tr("Asiakas"));
    }

    // Alustetaan maksutapacombo
    maksutapaModel_->lataa( meno ? MaksutapaModel::MENO : MaksutapaModel::TULO );

    if( viimeMaksutapa__.length())
        ui->maksutapaCombo->setCurrentIndex( ui->maksutapaCombo->findText( viimeMaksutapa__ ));
    if( ui->maksutapaCombo->currentIndex() < 0)
        ui->maksutapaCombo->setCurrentIndex(0);

    ui->vastatiliLine->suodataTyypilla("[AB]");
    maksutapaMuuttui();

    bool alv = kp()->asetukset()->onko( AsetusModel::ALV );
    ui->alvLabel->setVisible(alv);
    ui->alvCombo->setVisible(alv);

    ui->erapaivaEdit->setDateRange(QDate(), QDate());
    ui->loppuEdit->setDateRange( kp()->tilitpaatetty().addDays(1), QDate() );

    ui->asiakasToimittaja->alusta();
    paivitaVeroFiltterit(tosite()->pvm());

}

int TuloMenoApuri::rivilla() const
{

    if( ui->tilellaView->currentIndex().row() < 0 )
        return 0;
    return ui->tilellaView->currentIndex().row();

}

TulomenoRivi *TuloMenoApuri::rivi()
{    
    return rivit_->rivi(rivilla());
}

void TuloMenoApuri::kumppaniValittu(int kumppaniId)
{
    KpKysely *kysely = kpk(QString("/kumppanit/%1").arg(kumppaniId));
    connect(kysely, &KpKysely::vastaus, this, &TuloMenoApuri::kumppaniTiedot);
    kysely->kysy();

    ui->eraCombo->lataa(ui->vastatiliLine->valittuTilinumero(), kumppaniId);
}

void TuloMenoApuri::kumppaniTiedot(QVariant *data)
{
    QVariantMap map = data->toMap();

    if(! ui->maaraEdit->asCents() ) {
        if(menoa_  ) {
            if( map.contains("menotili"))
                ui->tiliEdit->valitseTiliNumerolla( map.value("menotili").toInt() );
        } else {
            if( map.contains("tulotili"))
                ui->tiliEdit->valitseTiliNumerolla( map.value("tulotili").toInt());
        }
    }

    if( tosite()->tyyppi() == TositeTyyppi::KULULASKU && tosite()->otsikko().isEmpty() )
        tosite()->asetaOtsikko( tr("Kululasku %1").arg(map.value("nimi").toString()) );

    if( !resetoidaanko() )
        teeTositteelle();
}

QString TuloMenoApuri::viimeMaksutapa__ = QString();
