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
#include "apuririvit.h"

#include "db/kirjanpito.h"
#include "model/tosite.h"
#include "model/tositeviennit.h"
#include "db/tositetyyppimodel.h"
#include "rekisteri/asiakastoimittajadlg.h"
#include "model/maksutapamodel.h"
#include "alv/alvilmoitustenmodel.h"

#include <QSortFilterProxyModel>
#include <QDebug>
#include <QJsonDocument>
#include <QKeyEvent>
#include <QSettings>
#include <QTimer>

TuloMenoApuri::TuloMenoApuri(QWidget *parent, Tosite *tosite) :
    ApuriWidget (parent, tosite),
    ui(new Ui::TuloMenoApuri),
    rivit_(new ApuriRivit(this)),
    maksutapaModel_(new MaksutapaModel(this))
{
    ui->setupUi(this);

    // Viimeisin maksutapa säilyy jotta maksuperusteinen elämä on helpompaa
    viimeMaksutapa_ = kp()->settings()->value( kp()->asetukset()->asetus(AsetusModel::UID) + "/ViimeMaksutapa" ).toString();

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
    ui->tilellaView->horizontalHeader()->setSectionResizeMode(ApuriRivit::TILI, QHeaderView::Stretch);
    if( !kp()->asetukset()->onko(AsetusModel::AlvVelvollinen))
        ui->tilellaView->hideColumn(ApuriRivit::ALV);


    connect( ui->tiliEdit, &TilinvalintaLine::textChanged, this, &TuloMenoApuri::tiliMuuttui );

    connect( ui->maaraEdit, &KpEuroEdit::textEdited, this, &TuloMenoApuri::maaraMuuttui);
    connect( ui->verotonEdit, &KpEuroEdit::textEdited, this, &TuloMenoApuri::verotonMuuttui);

    ui->alvProssa->addItems(QStringList() << "25,50 %" << "24,00 %" << "14,00 %" << "10,00 %");
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
    connect( ui->asiakasToimittaja, &AsiakasToimittajaValinta::muuttui, this, &TuloMenoApuri::kumppaniValittu);

    connect( ui->vastatiliLine, &TilinvalintaLine::textChanged, this, &TuloMenoApuri::vastatiliMuuttui);
    connect( ui->laskuNumeroEdit, &QLineEdit::textChanged, tosite, &Tosite::asetaLaskuNumero);

    connect( tosite, &Tosite::otsikkoMuuttui, this, &TuloMenoApuri::tositteelle);
    connect( tosite, &Tosite::pvmMuuttui, this, &TuloMenoApuri::pvmMuuttui);
    connect( tosite, &Tosite::eraPvmMuuttui, ui->erapaivaEdit,[this] (const QDate& erapvm) { if(erapvm != ui->erapaivaEdit->date() && ui->erapaivaEdit->isVisible()) ui->erapaivaEdit->setDate(erapvm); } );
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
    kp()->settings()->setValue(kp()->asetukset()->asetus(AsetusModel::UID) + "/ViimeMaksutapa", viimeMaksutapa_);
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
        ui->viiteEdit->setText( lasku.value("viite").toString().remove(QRegularExpression("^0+")));
        ui->laskuNumeroEdit->setText( lasku.value("numero").toString());

        QVariantList alvit = map.value("alv").toList();
        for(int i=0; i < alvit.count(); i++) {
            QVariantMap alvi = alvit.value(i).toMap();
            TositeVienti vienti;

            ui->alvCombo->setCurrentIndex(
                        ui->alvCombo->findData(alvi.value("alvkoodi")));            
            setAlvProssa(alvi.value("alvprosentti").toDouble());

            Euro netto( alvi.value("netto").toString());
            Euro vero( alvi.value("vero").toString());
            Euro brutto = netto + vero;

            ui->maaraEdit->setEuro(brutto);
            emit ui->maaraEdit->textEdited( ui->maaraEdit->text());

            if( i < alvit.count() - 1)
                lisaaRivi();
        }

    } else {
        Euro summa( map.value("summa").toString());
        if( summa && !ui->maaraEdit->euro()) {
            ui->maaraEdit->setEuro(summa);
            emit ui->maaraEdit->textEdited( ui->maaraEdit->text() );
        }
        if( !map.value("viite").toString().isEmpty() && ui->viiteEdit->text().isEmpty())
            ui->viiteEdit->setText( map.value("viite").toString() );

        if( !map.value("laskunnumero").toString().isEmpty() && ui->laskuNumeroEdit->text().isEmpty())
            ui->laskuNumeroEdit->setText(map.value("laskunnumero").toString());


        if(( !map.value("kumppaninimi").toString().isEmpty() || !map.value("kumppaniytunnus").toString().isEmpty())
                && !ui->asiakasToimittaja->id())
            ui->asiakasToimittaja->tuonti( map );

        if( map.value("erapvm").isValid() && !ui->erapaivaEdit->date().isValid())
            ui->erapaivaEdit->setDate( map.value("erapvm").toDate());

        if( map.value("maksutapa").toString() == "kateinen") {
            int maksutapaind = ui->maksutapaCombo->findData(kp()->tilit()->tiliTyypilla(TiliLaji::KATEINEN).numero(), MaksutapaModel::TiliRooli);
            if( maksutapaind >= 0)
                ui->maksutapaCombo->setCurrentIndex(maksutapaind);
        }

    }
    QTimer::singleShot(0, this, [this] {this->tositteelle();});
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
    rivit_->asetaTyyppi(menoa ? TositeVienti::VientiTyyppi::OSTO : TositeVienti::VientiTyyppi::MYYNTI, !menoa);

    alusta( menoa );

    rivit_->clear();
    ui->viiteEdit->clear();    
    ui->erapaivaEdit->setNull();    

    ui->viiteEdit->setText( tosite()->viite());
    ui->laskuPvm->setDate( tosite()->laskupvm().isValid() ? tosite()->laskupvm() : tosite()->pvm());
    ui->erapaivaEdit->setDate( tosite()->erapvm());    
    ui->laskuNumeroEdit->setText( tosite()->laskuNumero());

    if( tosite()->kumppani() || !tosite()->kumppaninimi().isEmpty())
        ui->asiakasToimittaja->valitse(tosite()->kumppanimap());
    else
        ui->asiakasToimittaja->clear();


    for( auto vienti : tosite()->viennit()->viennit() ) {

        // Jos vastakirjaus käsitellään itse
        // Muuten riveille


        if( vienti.tyyppi() % 100 == TositeVienti::VASTAKIRJAUS) {
            if(vienti.eraId()) {
                valitutErat_.insert(vienti.tili(), vienti.era());
            }

            Tili* vastatili = kp()->tilit()->tili( vienti.tili());                        
            if( vastatili ) {
                int maksutapaindeksi = ui->maksutapaCombo->count() - 1;
                bool uusiEra = vienti.eraId() && vienti.eraId() == vienti.id(); // Tämä vienti aloittaa uuden erän

                for(int i=0; i < ui->maksutapaCombo->count()-1; i++) {
                    const int maksutavanTili = ui->maksutapaCombo->itemData(i, MaksutapaModel::TiliRooli).toInt();
                    const bool uusiEraMaksutavassa = ui->maksutapaCombo->itemData(i, MaksutapaModel::UusiEraRooli).toBool();

                    if( maksutavanTili == vastatili->numero() && uusiEra == uusiEraMaksutavassa) {
                        maksutapaindeksi = i;
                        break;
                    }
                }
                ui->maksutapaCombo->setCurrentIndex( maksutapaindeksi );
                ui->vastatiliLine->valitseTiliNumerolla( vastatili->numero() );

                if( vastatili->eritellaankoTase()) {
                    if(uusiEra)
                        ui->eraCombo->valitseUusiEra();
                    else
                        ui->eraCombo->valitse( vienti.era() );
                }
                maksutapaMuuttui();

                if( !vienti.arkistotunnus().isEmpty())
                    arkistotunnus_ = vienti.arkistotunnus();
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

    if( rivit_->rowCount())
        ui->tilellaView->selectRow(0);


    tiliMuuttui();    


}

bool TuloMenoApuri::teeTositteelle()
{        
    if( resetoidaanko())
        return false;

    qlonglong summa = 0l;

    QVariantList viennit = rivit_->viennit( tosite() );

    for( const auto& vienti : qAsConst(viennit)) {
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

    if( vastatili.eritellaankoTase())
        vasta.setEra(ui->eraCombo->valittuEra() ) ;   

    if( summa > 0)
        vasta.setDebet( summa );
    else
        vasta.setKredit( 0 - summa );

    vasta.setSelite(otsikko);
    if( !arkistotunnus_.isEmpty())
        vasta.setArkistotunnus(arkistotunnus_);

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

    bool laskulle = (vastatili.onko(TiliLaji::OSTOVELKA) || vastatili.onko(TiliLaji::MYYNTISAATAVA))
            && vastatili.eritellaankoTase()
            && ui->maksutapaCombo->currentData(MaksutapaModel::UusiEraRooli).toBool();

    if( laskulle ) {
        tosite()->asetaLaskupvm( ui->laskuPvm->date());
        tosite()->asetaErapvm( ui->erapaivaEdit->date());
        tosite()->asetaViite( ui->viiteEdit->text());
    } else {
        tosite()->asetaLaskupvm(QDate());
        tosite()->asetaErapvm(QDate());
        tosite()->asetaViite(QString());
    }

    tosite()->viennit()->asetaViennit(viennit);

    if(summa)
        viimeMaksutapa_ = ui->maksutapaCombo->currentText();

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

    if( !resetoidaanko() || ( !tosite()->id() && tosite()->viennit()->vienti(0).arkistotunnus().isEmpty() ) ) {
        // Vastaviennillä arkistotunnus kun tehdään tiliotteelta uutta riviä

        if( tasapoisto ) {
            ui->poistoSpin->setValue( tili.luku("tasaerapoisto") / 12 );
            poistoAikaMuuttuu();
        }

        if( !kp()->alvIlmoitukset()->onkoIlmoitettu(tosite()->pvm())) {
            if( kp()->asetukset()->onko(AsetusModel::AlvVelvollinen) )
            {
                int verotyyppi = tili.luku("alvlaji");
                bool maksuperuste = kp()->onkoMaksuperusteinenAlv(tosite()->pvm()) && ( ui->vastatiliLine->valittuTili().onko(TiliLaji::OSTOVELKA)
                                                                    || ui->vastatiliLine->valittuTili().onko(TiliLaji::MYYNTISAATAVA));

                if( verotyyppi == AlvKoodi::OSTOT_NETTO && tosite()->tyyppi() == TositeTyyppi::TULO && tili.onko(TiliLaji::TASE))
                    verotyyppi = AlvKoodi::MYYNNIT_NETTO;
                if( verotyyppi == AlvKoodi::OSTOT_NETTO && maksuperuste)
                    verotyyppi = AlvKoodi::MAKSUPERUSTEINEN_OSTO;
                if( verotyyppi == AlvKoodi::MYYNNIT_NETTO && maksuperuste)
                    verotyyppi = AlvKoodi::MAKSUPERUSTEINEN_MYYNTI;                

                // Varmistetaan, että verotyyppi säilyy
                QString filtteri = veroFiltteri_->filterRegularExpression().pattern();
                const QRegularExpression uusifiltteri = QRegularExpression(filtteri.left( filtteri.length() - 1 ) + "|" + QString::number(verotyyppi) + ")");
                veroFiltteri_->setFilterRegularExpression( uusifiltteri );

                ui->alvCombo->setCurrentIndex( ui->alvCombo->findData( verotyyppi, VerotyyppiModel::KoodiRooli ) );
                double pohjaAlv = tili.alvprosentti();
                // Automaattinen alv-muutos
                double alv = pohjaAlv == 24.0 ? yleinenAlv(tosite()->pvm()) / 100.0 : pohjaAlv;
                setAlvProssa( alv );
            } else {
                ui->alvCombo->setCurrentIndex( ui->alvCombo->findData( AlvKoodi::EIALV, VerotyyppiModel::KoodiRooli) );
            }
        }

        if( tili.luku("vastatili") && rivit_->rowCount()<2) {
            int vastatili = tili.luku("vastatili");
            ui->vastatiliLine->valitseTiliNumerolla(vastatili);
            int maksutapaind = ui->maksutapaCombo->findData(vastatili, MaksutapaModel::TiliRooli);
            if( maksutapaind >= 0)
                ui->maksutapaCombo->setCurrentIndex(maksutapaind);
            else
                ui->maksutapaCombo->setCurrentIndex(ui->maksutapaCombo->count()-1);
        }

        if(tili.luku("kohdennus"))
            ui->kohdennusCombo->valitseKohdennus( tili.luku("kohdennus") );
        
        emit rivit_->dataChanged( rivit_->index(rivilla(),  ApuriRivit::TILI),
                                 rivit_->index(rivilla(), ApuriRivit::TILI));

        tositteelle();
    }

}

void TuloMenoApuri::verolajiMuuttui()
{    

    int alvkoodi = ui->alvCombo->currentData( VerotyyppiModel::KoodiRooli ).toInt();
    rivi()->setAlvkoodi(  alvkoodi );
    emit rivit_->dataChanged(rivit_->index(rivilla(), ApuriRivit::ALV),rivit_->index(rivilla(), ApuriRivit::EUROA));

    bool naytaMaara = rivi()->naytaBrutto();
    bool naytaVeroton =  rivi()->naytaNetto();

    ui->maaraLabel->setVisible(naytaMaara);
    ui->maaraEdit->setVisible(naytaMaara);

    ui->verotonLabel->setVisible(naytaVeroton);
    ui->verotonEdit->setVisible(naytaVeroton);

    if( !naytaMaara && !rivi()->nettoSyotetty() && !resetoidaanko()) {
        qlonglong maara = rivi()->brutto();
        ui->verotonEdit->setCents(maara);
        rivi()->setNetto(maara);
        emit rivit_->dataChanged(rivit_->index(rivilla(), ApuriRivit::EUROA),rivit_->index(rivilla(), ApuriRivit::EUROA));
    }


    ui->alvProssa->setVisible( !ui->alvCombo->currentData(VerotyyppiModel::NollaLajiRooli).toBool() );
    ui->vahennysCheck->setVisible( rivi()->naytaVahennysvalinta());
    ui->vahennysCheck->setChecked( false );

    if( !resetoidaanko()) {
        veroprossaMuuttui();

        if( !ui->alvCombo->currentData(VerotyyppiModel::NollaLajiRooli).toBool() && alvProssa() < 1e-5) {
            setAlvProssa(yleinenAlv(tosite()->pvm()));
        }
    }


    tositteelle();
}

void TuloMenoApuri::pvmMuuttui(const QDate &pvm)
{    
    if(!resetoidaanko())
        ui->laskuPvm->setDate(pvm);
    haeKohdennukset();
    paivitaVeroFiltterit(pvm, ui->alvCombo->currentData().toInt());
    tositteelle();
}

void TuloMenoApuri::maaraMuuttui()
{
    qlonglong maara = ui->maaraEdit->asCents();
    rivi()->setBrutto( maara );
    ui->verotonEdit->setCents( rivi()->netto() );
    
    emit rivit_->dataChanged( rivit_->index(ApuriRivit::EUROA, rivilla()),
                             rivit_->index(ApuriRivit::EUROA, rivilla()));

    tositteelle();
}

void TuloMenoApuri::verotonMuuttui()
{
    qlonglong veroton = ui->verotonEdit->asCents();

    rivi()->setNetto( veroton );
    ui->maaraEdit->setCents( rivi()->brutto());
    
    emit rivit_->dataChanged( rivit_->index(ApuriRivit::EUROA, rivilla()),
                             rivit_->index(ApuriRivit::EUROA, rivilla()));


    tositteelle();
}

void TuloMenoApuri::veroprossaMuuttui()
{
    double verokanta = alvProssa();
    rivi()->setAlvprosentti( verokanta  );
    ui->maaraEdit->setCents( rivi()->brutto());
    ui->verotonEdit->setCents( rivi()->netto() );
    tositteelle();
    
    emit rivit_->dataChanged( rivit_->index(ApuriRivit::ALV, rivilla()),
                             rivit_->index(ApuriRivit::ALV, rivilla()));
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
    int vastatilinumero = ui->vastatiliLine->valittuTilinumero();

    // Jotta saldoa ei haettaisi tolkuttoman montaa kertaa,
    // varmistetaan että vastatili on todella muuttunut
    if( vastatilinumero == vastatili_ &&
        vastatiliSaldoPaivitetty_.msecsTo(QDateTime::currentDateTime()) < 1000 ) return;


    Tili vastatili = kp()->tilit()->tiliNumerolla( vastatilinumero );
    const bool uusiEraMaksutapa = ui->maksutapaCombo->currentData(MaksutapaModel::UusiEraRooli).toBool();

    bool eritellankotaso = vastatili.eritellaankoTase();

    ui->eraLabel->setVisible( eritellankotaso);
    ui->eraCombo->setVisible( eritellankotaso);
    ui->eraCombo->asetaTili( vastatili.numero() , ui->asiakasToimittaja->id());

    if( vastatili_ != vastatilinumero ) {
        // Vastatili on todellisuudessa muuttunut, vastatilin mukainen tase-erä
        if( valitutErat_.contains(vastatili_)) {
            ui->eraCombo->valitse(valitutErat_.value(vastatili_));
        } else {
            if( uusiEraMaksutapa )
                ui->eraCombo->valitseUusiEra();
            else
                ui->eraCombo->valitseEiEraa();
        }
    }

    vastatili_ = vastatilinumero;
    vastatiliSaldoPaivitetty_ = QDateTime::currentDateTime();


    const QString lisatietovalinta = kp()->asetukset()->asetus(AsetusModel::LaskuLisatiedot);

    bool laskulle =
            (lisatietovalinta == "KAIKKI" ? true :
            (lisatietovalinta == "EI" ? false :
            (vastatili.onko(TiliLaji::OSTOVELKA) || vastatili.onko(TiliLaji::MYYNTISAATAVA))
            && vastatili.eritellaankoTase()
            && ui->maksutapaCombo->currentData(MaksutapaModel::UusiEraRooli).toBool() ));


    ui->viiteLabel->setVisible( laskulle );
    ui->viiteEdit->setVisible( laskulle );

    ui->laskupvmLabel->setVisible( laskulle );
    ui->laskuPvm->setVisible( laskulle );

    ui->laskunnumeroLabel->setVisible(laskulle);
    ui->laskuNumeroEdit->setVisible(laskulle);

    ui->erapaivaLabel->setVisible( laskulle );
    ui->erapaivaEdit->setVisible( laskulle );    

    emit tosite()->tarkastaSarja( vastatili.onko(TiliLaji::KATEINEN));
    paivitaVeroFiltterit(tosite()->pvm(), ui->alvCombo->currentData().toInt());

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
    if(alkupvm.isValid())
        ui->loppuEdit->setDateRange(alkupvm, QDate());
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

void TuloMenoApuri::paivitaVeroFiltterit(const QDate &pvm, int verokoodi)
{
    bool maksuperuste = kp()->onkoMaksuperusteinenAlv(pvm) &&
            ( ui->vastatiliLine->valittuTili().onko(TiliLaji::OSTOVELKA) || ui->vastatiliLine->valittuTili().onko(TiliLaji::MYYNTISAATAVA) )
                && ui->vastatiliLine->valittuTili().eritellaankoTase() ;
    if( menoa_) {
        veroFiltteri_->setFilterRegularExpression( QRegularExpression(  maksuperuste ?
                                            "^(0|2[4-9]|927| " + QString::number(verokoodi) + ")"
                                            : "^(0|2[1-69]|927|" + QString::number(verokoodi) + ")" ) );
        if( verokoodi == AlvKoodi::OSTOT_NETTO && maksuperuste)
            ui->alvCombo->setCurrentIndex( ui->alvCombo->findData(AlvKoodi::MAKSUPERUSTEINEN_OSTO, VerotyyppiModel::KoodiRooli) );
        else if( verokoodi == AlvKoodi::MAKSUPERUSTEINEN_OSTO && !maksuperuste)
            ui->alvCombo->setCurrentIndex( ui->alvCombo->findData(AlvKoodi::OSTOT_NETTO, VerotyyppiModel::KoodiRooli) );
        else
            ui->alvCombo->setCurrentIndex( ui->alvCombo->findData(verokoodi, VerotyyppiModel::KoodiRooli) );
    } else {
        veroFiltteri_->setFilterRegularExpression( QRegularExpression( maksuperuste ?
                                           "^(0|1[4-9]|" + QString::number(verokoodi) + ")"
                                           : "^(0|1[1-79]|" + QString::number(verokoodi) + ")" ) );
        if(verokoodi == AlvKoodi::MYYNNIT_NETTO && maksuperuste)
            ui->alvCombo->setCurrentIndex( ui->alvCombo->findData(AlvKoodi::MAKSUPERUSTEINEN_MYYNTI, VerotyyppiModel::KoodiRooli) );
        else if(verokoodi == AlvKoodi::MAKSUPERUSTEINEN_MYYNTI && !maksuperuste)
            ui->alvCombo->setCurrentIndex( ui->alvCombo->findData(AlvKoodi::MYYNNIT_NETTO, VerotyyppiModel::KoodiRooli) );
        else
            ui->alvCombo->setCurrentIndex( ui->alvCombo->findData(verokoodi, VerotyyppiModel::KoodiRooli) );
    }

}

void TuloMenoApuri::haeRivi(const QModelIndex &index)
{
    bool resetoinnissa = resetoidaanko();
    if(!resetoinnissa) resetointiKaynnissa_ = true;

    int rivilla = index.row();
    
    ApuriRivi* rivi = rivit_->rivi(rivilla);
    int tilinumero = rivi->tilinumero();

    if( !tilinumero) {
        if( tosite()->tyyppi() == TositeTyyppi::TULO )
            tilinumero = kp()->tilit()->tiliTyypilla(TiliLaji::LVTULO).numero();
        else
            tilinumero = kp()->tilit()->tiliTyypilla(TiliLaji::MENO).numero();
        ui->tiliEdit->valitseTiliNumerolla( tilinumero );
        tiliMuuttui();
    } else {
        ui->tiliEdit->valitseTiliNumerolla(tilinumero);
    }

    paivitaVeroFiltterit( tosite()->pvm(), rivi->alvkoodi() );
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
    if(!resetoinnissa) resetointiKaynnissa_ = false;
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

    int viimeIndeksi = ui->maksutapaCombo->findText( viimeMaksutapa_);
    if( viimeIndeksi > -1)
        ui->maksutapaCombo->setCurrentIndex( viimeIndeksi );
    else if( ui->maksutapaCombo->count())
        ui->maksutapaCombo->setCurrentIndex(0);

    ui->vastatiliLine->suodataTyypilla("[AB]");
    maksutapaMuuttui();

    bool alv = kp()->asetukset()->onko( AsetusModel::AlvVelvollinen );
    ui->alvLabel->setVisible(alv);
    ui->alvCombo->setVisible(alv);

    ui->erapaivaEdit->setDateRange(QDate(), QDate());

    paivitaVeroFiltterit(tosite()->pvm(), ui->alvCombo->currentData().toInt());

}

int TuloMenoApuri::rivilla() const
{

    if( ui->tilellaView->currentIndex().row() < 0 )
        return 0;
    return ui->tilellaView->currentIndex().row();

}

ApuriRivi *TuloMenoApuri::rivi()
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
    QRegularExpression valiRe("[^\\d\\.]");
    int vali = txt.indexOf(valiRe);
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

void TuloMenoApuri::kumppaniValittu(QVariantMap data)
{
    QVariant d(data);
    kumppaniTiedot(&d);
    ui->eraCombo->asetaTili(ui->vastatiliLine->valittuTilinumero(), data.value("id").toInt());
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

void TuloMenoApuri::eraValittu( EraMap era)
{
    if( !ui->asiakasToimittaja->id())
        ui->asiakasToimittaja->valitse(era.kumppani());
    valitutErat_.insert( vastatili_, era);
}

