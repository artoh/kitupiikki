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
#include <QKeyEvent>

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
    ui->kohdennusCombo->valitseNaytettavat(KohdennusProxyModel::KOHDENNUKSET_PROJEKTIT);

    ui->tilellaView->setModel( rivit_);
    ui->tilellaView->horizontalHeader()->setSectionResizeMode(TmRivit::TILI, QHeaderView::Stretch);
    if( !kp()->asetukset()->onko(AsetusModel::ALV))
        ui->tilellaView->hideColumn(TmRivit::ALV);


    connect( ui->tiliEdit, &TilinvalintaLine::textChanged, this, &TuloMenoApuri::tiliMuuttui );

    connect( ui->maaraEdit, &KpEuroEdit::textEdited, this, &TuloMenoApuri::maaraMuuttui);
    connect( ui->verotonEdit, &KpEuroEdit::textEdited, this, &TuloMenoApuri::verotonMuuttui);

    ui->alvProssa->addItems(QStringList() << "24,00 %" << "14,00 %" << "10,00 %");
    ui->alvProssa->setValidator(new QRegularExpressionValidator(QRegularExpression("\\d{1,2}(,\\d{1,2})\\s?%?"),this));
    connect( ui->alvProssa, &QComboBox::currentTextChanged, this, &TuloMenoApuri::veroprossaMuuttui);

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
    connect( ui->eraCombo, &EraCombo::valittu, this, &TuloMenoApuri::vastatiliMuuttui);
    connect( ui->vastatiliLine, &TilinvalintaLine::textChanged, this, &TuloMenoApuri::vastatiliMuuttui);

    connect( ui->viiteEdit, &QLineEdit::textEdited, this, &TuloMenoApuri::tositteelle);
    connect( ui->laskuPvm, &KpDateEdit::dateChanged, this, &TuloMenoApuri::tositteelle);
    connect( ui->erapaivaEdit, &KpDateEdit::dateChanged, this, &TuloMenoApuri::tositteelle);

    connect( ui->asiakasToimittaja, &AsiakasToimittajaValinta::muuttui, this, &TuloMenoApuri::tositteelle);
    connect( ui->asiakasToimittaja, &AsiakasToimittajaValinta::valittu, this, &TuloMenoApuri::kumppaniValittu);

    connect( ui->vastatiliLine, &TilinvalintaLine::textChanged, this, &TuloMenoApuri::vastatiliMuuttui);
    connect( ui->laskuNumeroEdit, &QLineEdit::textChanged, tosite, &Tosite::asetaLaskuNumero);

    connect( tosite, &Tosite::pvmMuuttui, this, &TuloMenoApuri::pvmMuuttui);
    connect( tosite, &Tosite::eraPvmMuuttui, ui->erapaivaEdit,[this] (const QDate& erapvm) { if(erapvm != ui->erapaivaEdit->date()) ui->erapaivaEdit->setDate(erapvm); } );
    connect( ui->eraCombo, &EraCombo::currentTextChanged, this, &TuloMenoApuri::tositteelle);
    connect( ui->eraCombo, &EraCombo::valittu, this, &TuloMenoApuri::eraValittu);

    ui->maksutapaCombo->installEventFilter(this);
    ui->laskuNumeroEdit->installEventFilter(this);
    ui->laskuPvm->installEventFilter(this);
    ui->viiteEdit->installEventFilter(this);
    ui->erapaivaEdit->installEventFilter(this);
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
    tuonnissa_ = true;

    if( map.value("tyyppi").toInt() == TositeTyyppi::SAAPUNUTVERKKOLASKU) {

        QVariantMap lasku = map.value("lasku").toMap();
        ui->asiakasToimittaja->tuonti( map.value("toimittaja").toMap() );
        ui->laskuPvm->setDate( lasku.value("pvm").toDate() );
        ui->erapaivaEdit->setDate( lasku.value("erapvm").toDate());
        ui->viiteEdit->setText( lasku.value("viite").toString());
        ui->laskunnumeroLabel->setText( lasku.value("numero").toString());

        QVariantList alvit = map.value("alv").toList();
        for(int i=0; i < alvit.count(); i++) {
            QVariantMap alvi = alvit.value(i).toMap();
            TositeVienti vienti;

            ui->alvCombo->setCurrentIndex(
                        ui->alvCombo->findData(alvi.value("alvkoodi")));            
            setAlvProssa(alvi.value("alvprosentti").toDouble());
            ui->verotonEdit->setValue( alvi.value("netto").toDouble() );
            verotonMuuttui();

            if( i < alvit.count() - 1)
                lisaaRivi();
        }

    } else {
        if( qAbs(map.value("summa").toDouble()) > 1e-5) {
            ui->maaraEdit->setValue( map.value("summa").toDouble());
            emit ui->maaraEdit->textEdited( ui->maaraEdit->text() );
        }
        if( !map.value("viite").toString().isEmpty())
            ui->viiteEdit->setText( map.value("viite").toString() );

        if( !map.value("laskunnumero").toString().isEmpty())
            ui->laskuNumeroEdit->setText(map.value("laskunnumero").toString());


        if( !map.value("kumppaninimi").toString().isEmpty() || !map.value("kumppaniytunnus").toString().isEmpty())
            ui->asiakasToimittaja->tuonti( map );

        if( map.value("erapvm").isValid())
            ui->erapaivaEdit->setDate( map.value("erapvm").toDate());

        if( map.value("maksutapa").toString() == "kateinen") {
            int maksutapaind = ui->maksutapaCombo->findData(kp()->tilit()->tiliTyypilla(TiliLaji::KATEINEN).numero(), MaksutapaModel::TiliRooli);
            if( maksutapaind >= 0)
                ui->maksutapaCombo->setCurrentIndex(maksutapaind);
        }

    }
    tositteelle();
}

void TuloMenoApuri::salliMuokkaus(bool sallitaanko)
{
    for( QObject* object : children()) {
        QWidget* widget = qobject_cast<QWidget*>(object);
        if(!widget)
            continue;
        if(widget->objectName() == "loppuEdit")
            widget->setEnabled(ui->alkuEdit->date().isValid() && sallitaanko);
        else if(widget->objectName() == "tilellaView")
            widget->setEnabled(true);
        else
            widget->setEnabled(sallitaanko);
    }
        ui->poistaRiviNappi->setEnabled( sallitaanko && rivit_->rowCount() > 1 );
}

void TuloMenoApuri::teeReset()
{

    // Haetaan tietoja mallista ;)
    bool menoa = tosite()->tyyppi() < TositeTyyppi::TULO;
    tuonnissa_ = false;

    alusta( menoa );


    rivit_->clear();
    ui->viiteEdit->clear();    
    ui->erapaivaEdit->setNull();    

    ui->viiteEdit->setText( tosite()->viite());
    ui->laskuPvm->setDate( tosite()->laskupvm().isValid() ? tosite()->laskupvm() : tosite()->pvm());
    ui->erapaivaEdit->setDate( tosite()->erapvm());
    ui->laskuNumeroEdit->setText( tosite()->laskuNumero());

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
                {
                    // Jotta hyvityslasku saisi oman tyyppinsä
                    if( maksutapaModel_->index(maksutapaind,0).data(MaksutapaModel::UusiEraRooli).toBool() && vienti.eraId() > 0 &&
                            vienti.eraId() != vienti.id()) {
                        for(int i=0; i<maksutapaModel_->rowCount();i++) {
                            QModelIndex indeksi = maksutapaModel_->index(i,0);
                            if( indeksi.data(MaksutapaModel::TiliRooli).toInt() == vastatili->numero() && !indeksi.data(MaksutapaModel::UusiEraRooli).toBool())
                                maksutapaind = i;
                        }
                    }
                    ui->maksutapaCombo->setCurrentIndex(maksutapaind);
                }
                else
                    ui->maksutapaCombo->setCurrentIndex(ui->maksutapaCombo->count()-1);                

                ui->vastatiliLine->valitseTiliNumerolla( vastatili->numero() );

                if( vastatili->eritellaankoTase())
                    ui->eraCombo->valitse( vienti.eraId() );
                maksutapaMuuttui();
            }

        } else {
            if(vienti.selite() == tosite()->otsikko())
                vienti.setSelite(QString());
            rivit_->lisaa(vienti);
        }

    }
    if( !rivit_->rowCount())
        rivit_->lisaaRivi(menoa_ ? kp()->asetukset()->luku("OletusMenotili") : kp()->asetukset()->luku("OletusMyyntitili"));

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

    bool menoa = tosite()->tyyppi() == TositeTyyppi::MENO ||
                 tosite()->tyyppi() == TositeTyyppi::KULULASKU ||
                 tosite()->tyyppi() == TositeTyyppi::SAAPUNUTVERKKOLASKU;

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
    QVariantMap kumppaniMap;
    if( ui->asiakasToimittaja->id() > 0)
        kumppaniMap.insert("id", ui->asiakasToimittaja->id());
    if( !ui->asiakasToimittaja->nimi().isEmpty())
        kumppaniMap.insert("nimi", ui->asiakasToimittaja->nimi());
    if( !ui->asiakasToimittaja->ibanit().isEmpty())
        kumppaniMap.insert("iban", ui->asiakasToimittaja->ibanit());

    tosite()->asetaKumppani(kumppaniMap);
    vasta.setKumppani(kumppaniMap);


    if( summa || tosite()->data(Tosite::TILA).toInt() == Tosite::MALLIPOHJA) {
        viennit.insert(0, vasta);
    }

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
    int tili = menoa_ ? kp()->asetukset()->luku("OletusMenotili") : kp()->asetukset()->luku("OletusMyyntitili");

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
            setAlvProssa(tili.str("alvprosentti").toDouble() );
        } else {
            ui->alvCombo->setCurrentIndex( ui->alvCombo->findData( AlvKoodi::EIALV, VerotyyppiModel::KoodiRooli) );
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

        if(tili.luku("kohdennus"))
            ui->kohdennusCombo->valitseKohdennus( tili.luku("kohdennus") );

        emit rivit_->dataChanged( rivit_->index(TmRivit::TILI, rivilla()),
                                  rivit_->index(TmRivit::TILI, rivilla()));

        tositteelle();
    }

}

void TuloMenoApuri::verolajiMuuttui()
{    

    int alvkoodi = ui->alvCombo->currentData( VerotyyppiModel::KoodiRooli ).toInt();
    rivi()->setAlvkoodi(  alvkoodi );
    emit rivit_->dataChanged(rivit_->index(rivilla(), TmRivit::ALV),rivit_->index(rivilla(), TmRivit::EUROA));

    qDebug() << "alvkoodi " << alvkoodi;

    bool naytaMaara = rivi()->naytaBrutto();
    bool naytaVeroton =  rivi()->naytaNetto();

    ui->maaraLabel->setVisible(naytaMaara);
    ui->maaraEdit->setVisible(naytaMaara);

    ui->verotonLabel->setVisible(naytaVeroton);
    ui->verotonEdit->setVisible(naytaVeroton);

    if( !naytaMaara && !rivi()->nettoSyotetty()) {
        qlonglong maara = rivi()->brutto();
        ui->verotonEdit->setCents(maara);
        rivi()->setNetto(maara);
        emit rivit_->dataChanged(rivit_->index(rivilla(), TmRivit::EUROA),rivit_->index(rivilla(), TmRivit::EUROA));
    }


    ui->alvProssa->setVisible( !ui->alvCombo->currentData(VerotyyppiModel::NollaLajiRooli).toBool() );
    ui->vahennysCheck->setVisible( rivi()->naytaVahennysvalinta());
    ui->vahennysCheck->setChecked( false );

    if( !ui->alvCombo->currentData(VerotyyppiModel::NollaLajiRooli).toBool() && alvProssa() < 1e-5) {
        setAlvProssa(24.0);
    }


    tositteelle();
}

void TuloMenoApuri::pvmMuuttui(const QDate &pvm)
{    
    if(!resetoidaanko())
        ui->laskuPvm->setDate(pvm);
    haeKohdennukset();
    paivitaVeroFiltterit(pvm);
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
    double verokanta = alvProssa();
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
    ui->tiliInfo->setVisible(maksutapatili);
    ui->saldoInfo->setVisible(maksutapatili);

    vastatiliMuuttui();

    emit tosite()->tarkastaSarja( kp()->tilit()->tiliNumerolla(maksutapatili).onko(TiliLaji::KATEINEN) );
}

void TuloMenoApuri::vastatiliMuuttui()
{
    Tili vastatili = kp()->tilit()->tiliNumerolla( ui->vastatiliLine->valittuTilinumero() );

    bool eritellankotaso = (vastatili.eritellaankoTase() &&
             !ui->maksutapaCombo->currentData(MaksutapaModel::UusiEraRooli).toBool()) ||
              tosite()->viennit()->vienti(0).eraId() ;

    ui->eraLabel->setVisible( eritellankotaso);
    ui->eraCombo->setVisible( eritellankotaso);
    ui->eraCombo->lataa( vastatili.numero() , ui->asiakasToimittaja->id());
    if( ( !eritellankotaso || ui->maksutapaCombo->currentData(MaksutapaModel::UusiEraRooli).toBool()) && !tosite()->viennit()->vienti(0).eraId() && sender() != ui->eraCombo) {
        ui->eraCombo->valitse(-1);
    }

    bool laskulle = (vastatili.onko(TiliLaji::OSTOVELKA) || vastatili.onko(TiliLaji::MYYNTISAATAVA))
            && vastatili.eritellaankoTase()
            && ui->maksutapaCombo->currentData(MaksutapaModel::UusiEraRooli).toBool();
    ui->viiteLabel->setVisible( laskulle );
    ui->viiteEdit->setVisible( laskulle );

    ui->laskupvmLabel->setVisible( laskulle );
    ui->laskuPvm->setVisible( laskulle );

    ui->laskunnumeroLabel->setVisible(laskulle);
    ui->laskuNumeroEdit->setVisible(laskulle);

    ui->erapaivaLabel->setVisible( laskulle );
    ui->erapaivaEdit->setVisible( laskulle );    

    emit tosite()->tarkastaSarja( vastatili.onko(TiliLaji::KATEINEN));
    paivitaVeroFiltterit(tosite()->pvm());

    tositteelle();

    int vastatilinNumero = vastatili.numero();
    ui->tiliInfo->setText( vastatili.nimiNumero());
    ui->saldoInfo->clear();
    KpKysely* kysely = kpk("/saldot");
    if( kp()->yhteysModel() &&  vastatilinNumero && kysely) {
        kysely->lisaaAttribuutti("tili", vastatilinNumero);
        kysely->lisaaAttribuutti("pvm", kp()->paivamaara());
        connect( kysely, &KpKysely::vastaus, this, &TuloMenoApuri::vastaSaldoSaapuu);
        kysely->kysy();
    }
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
                                           "^(0|1[4-9])"
                                           : "^(0|1[1-79])");
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

    ui->alvCombo->setCurrentIndex( ui->alvCombo->findData( rivi->alvkoodi(), VerotyyppiModel::KoodiRooli ) );
    setAlvProssa( rivi->alvprosentti());    

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
        ui->tiliEdit->suodataTyypilla("(AP|BY|D).*");        
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

void TuloMenoApuri::setAlvProssa(double prosentti)
{
    ui->alvProssa->setCurrentText(QString("%L1 %").arg(prosentti,0,'f',2));
}

double TuloMenoApuri::alvProssa() const
{
    if(ui->alvCombo->currentData(VerotyyppiModel::NollaLajiRooli).toBool() )
        return 0.0;

    QString txt = ui->alvProssa->currentText();
    txt.replace(",",".");
    int vali = txt.indexOf(QRegularExpression("[^\\d\\.]"));
    if( vali > 0)
        txt = txt.left(vali);    
    return txt.toDouble();
}

void TuloMenoApuri::vastaSaldoSaapuu(QVariant *data)
{
    QVariantMap map = data->toMap();
    double saldo = map.value(QString::number(ui->vastatiliLine->valittuTilinumero())).toDouble();
    ui->saldoInfo->setText(QString("%L1 €").arg(saldo,0,'f',2));
}

bool TuloMenoApuri::eventFilter(QObject *target, QEvent *event)
{
    if( event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if(keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return) {
            ui->tiliEdit->setFocus();
        }
    }
    return ApuriWidget::eventFilter(target, event);
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

    if(! ui->maaraEdit->asCents() || tuonnissa_) {
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

void TuloMenoApuri::eraValittu(int /* eraid */, double /* avoinna */, const QString & /*selite*/, int kumppani)
{
    if( !ui->asiakasToimittaja->id())
        ui->asiakasToimittaja->set(kumppani);
}

QString TuloMenoApuri::viimeMaksutapa__ = QString();
