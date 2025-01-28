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
#include "apuri/tiliote/tiliotekirjaaja.h"
#include "ui_tiliotekirjaaja.h"

#include "laskutaulutilioteproxylla.h"
#include "tilioteviennit.h"

#include <QPushButton>
#include <QSortFilterProxyModel>
#include "tilioteapuri.h"
#include "model/tosite.h"
#include "db/kirjanpito.h"
#include "laskutus/vakioviite/vakioviitemodel.h"
#include "../siirtoapuri.h"

#include <QShortcut>
#include <QSettings>
#include <QTimer>

TilioteKirjaaja::TilioteKirjaaja(TilioteApuri *apuri) :
    QDialog(apuri),
    ui(new Ui::TilioteKirjaaja),
    maksuProxy_(new QSortFilterProxyModel(this)),
    avoinProxy_( new QSortFilterProxyModel(this)),
    laskut_( new LaskuTauluTilioteProxylla(this, apuri->model())),
    rivit_(new ApuriRivit())
{
    alusta();

    resize(800,600);
    restoreGeometry( kp()->settings()->value("TilioteKirjaaja").toByteArray());
    ui->viennitView->setColumnHidden(ApuriRivit::ALV, !kp()->asetukset()->onko(AsetusModel::AlvVelvollinen));

}

TilioteKirjaaja::TilioteKirjaaja(SiirtoApuri *apuri):
    QDialog(apuri),
    ui(new Ui::TilioteKirjaaja),
    maksuProxy_(new QSortFilterProxyModel(this)),
    avoinProxy_( new QSortFilterProxyModel(this)),
    laskut_( new LaskuTauluModel(this)),
    rivit_(new ApuriRivit())
{
    alusta();
    ui->pvmEdit->setDate( apuri->tosite()->data(Tosite::PVM).toDate() );
    ui->alaTabs->hide();
    ui->alaTabs->setCurrentIndex(MAKSU);

    ui->tiliLabel->setText(tr("Vastatili"));
    ui->tiliLabel->setVisible(true);
    ui->tiliEdit->setVisible(true);
    ui->tiliEdit->suodataTyypilla("AR.*");
    ui->tiliEdit->valitseTili( kp()->tilit()->tiliTyypilla(TiliLaji::PANKKITILI) );
}

TilioteKirjaaja::~TilioteKirjaaja()
{
    kp()->settings()->setValue("TilioteKirjaaja", saveGeometry());
    delete ui;
}

void TilioteKirjaaja::asetaPvm(const QDate &pvm)
{    
    ui->pvmEdit->setDate(pvm);
}


void TilioteKirjaaja::accept()
{
    if( ui->okNappi->isEnabled() ) {
        if( !apuri()) {
            SiirtoApuri* siirto = qobject_cast<SiirtoApuri*>(parent());
            if( siirto ) {
                QModelIndex index = ui->maksuView->currentIndex();

                siirto->laskuMaksettu(
                    ui->pvmEdit->date(),
                    index.data(LaskuTauluModel::SeliteRooli).toString(),
                    index.data(LaskuTauluModel::TiliRooli).toInt(),
                    index.data(LaskuTauluModel::EraMapRooli).toMap(),
                    index.data(LaskuTauluModel::KumppaniMapRooli).toMap(),
                    ui->euroEdit->euro(),
                    menoa_,
                    ui->tiliEdit->valittuTilinumero()
                );
                QDialog::accept();
            }
        } else {
            const TilioteKirjausRivi rivi = tallennettava();

            if( riviIndeksi_ > -1) {
                apuri()->model()->asetaRivi(riviIndeksi_, rivi);
                QDialog::accept();
            } else {                
                apuri()->model()->lisaaRivi(rivi);
            }
            tyhjenna();
        }
    }
}

void TilioteKirjaaja::kirjaaUusia(const QDate &pvm)
{
    setWindowTitle( tr("Kirjaa tiliotteelle"));
    riviIndeksi_ = -1;
    tyhjenna();
    ui->pvmEdit->setDate(pvm);

    show();
    ylaTabMuuttui( ui->ylaTab->currentIndex() );
}

void TilioteKirjaaja::muokkaaRivia(int riviNro)
{
    setWindowTitle(tr("Muokkaa tiliotekirjausta"));

    riviIndeksi_ = riviNro;
    lataa(apuri()->model()->rivi(riviNro));
    tarkastaTallennus();
    show();
}


void TilioteKirjaaja::alaTabMuuttui(int tab)
{
    rivit_->asetaTyyppi(tyyppi(), !menoa_);

    ui->suodatusEdit->setVisible( tab == MAKSU || tab==VAKIOVIITE );
    ui->maksuView->setVisible( tab == MAKSU || tab == VAKIOVIITE );

    ui->tiliLabel->setVisible( tab != MAKSU && tab != VAKIOVIITE && tab != PIILOSSA);
    ui->tiliEdit->setVisible( tab != MAKSU && tab != VAKIOVIITE && tab != PIILOSSA);

    ui->kohdennusLabel->setVisible( tab != MAKSU && tab != VAKIOVIITE && kp()->kohdennukset()->kohdennuksia() );
    ui->kohdennusCombo->setVisible( tab != MAKSU && tab != VAKIOVIITE && kp()->kohdennukset()->kohdennuksia() );

    ui->merkkausLabel->setVisible(  tab != MAKSU && tab != SIIRTO && kp()->kohdennukset()->merkkauksia() );
    ui->merkkausCC->setVisible(  tab != MAKSU && tab != SIIRTO && kp()->kohdennukset()->merkkauksia() );

    ui->asiakasLabel->setVisible( tab != MAKSU );
    ui->asiakastoimittaja->setVisible( tab != MAKSU );

    ui->seliteLabel->setVisible(tab != MAKSU && tab != VAKIOVIITE);
    ui->seliteEdit->setVisible( tab != MAKSU && tab != VAKIOVIITE);


    if( tab == MAKSU ) {
        ui->maksuView->setModel(avoinProxy_);
        ui->maksuView->hideColumn( LaskuTauluModel::LAHETYSTAPA );
        laskut_->lataaAvoimetMaksettavat( menoa_ );

    } else if( tab == TULOMENO ) {
        bool menotili = ui->ylaTab->currentIndex() == TILILTA;

        ui->tiliLabel->setText( menotili ? tr("Menotili") : tr("Tulotili"));
        ui->asiakasLabel->setText( menotili ? tr("Toimittaja") : tr("Asiakas"));
        ui->tiliEdit->suodataTyypilla( menotili ? "D.*" : "C.*");
        Tili* valittuna = ui->tiliEdit->tili();
        if( (!valittuna || (menotili && !valittuna->onko(TiliLaji::MENO)) || (!menotili && !valittuna->onko(TiliLaji::TULO))) && !ui->euroEdit->euro() ) {
            ui->tiliEdit->valitseTiliNumerolla(  menotili ? kp()->asetukset()->luku("OletusMenotili") : kp()->asetukset()->luku("OletusMyyntitili") );    // TODO: Tod. oletukset
            const Tili& valittuTili = ui->tiliEdit->valittuTili();
            paivitaVeroFiltteri( valittuTili.alvlaji() );
            const double tiliProsentti = valittuTili.alvprosentti();
            const double vakioitu = tiliProsentti == 24.0 ? yleinenAlv(ui->pvmEdit->date()) / 100.0 : tiliProsentti;
            ui->alvProssaCombo->setCurrentText(QString("%L1 %").arg(vakioitu,0,'f',2));
            alvMuuttuu();
        }
    } else if ( tab == SIIRTO ) {
        ui->tiliLabel->setText( menoa_ ? tr("Tilille") : tr("Tililtä")  );
        ui->asiakasLabel->setText( menoa_ ? tr("Saaja") : tr("Maksaja"));
        ui->tiliEdit->suodataTyypilla( "[ABCD].*");

    } else if( tab == VAKIOVIITE) {
        ui->maksuView->setModel( kp()->vakioViitteet() );
    }


    tiliMuuttuu();
    ui->viennitView->horizontalHeader()->setSectionResizeMode(TilioteViennit::TILI, QHeaderView::Stretch);

    bool const voiLisataVienteja = tab != MAKSU && ui->alaTabs->currentIndex() != VAKIOVIITE;
    ui->lisaaVientiNappi->setVisible( voiLisataVienteja );
    ui->poistaVientiNappi->setVisible( voiLisataVienteja && rivit_->rowCount() > 1);
    ui->viennitView->setVisible( voiLisataVienteja && rivit_->rowCount() > 1);

}

void TilioteKirjaaja::euroMuuttuu()
{
   const Euro& euro = ui->euroEdit->euro();
    if( euro == rivi()->brutto())
    return;

   rivi()->setBrutto( euro );
   ui->verotonEdit->setEuro( rivi()->netto());

   aliRiviaMuokattu();

}

void TilioteKirjaaja::verotonMuuttuu()
{
   const Euro& euro = ui->verotonEdit->euro();
   if( euro == rivi()->netto())
        return;

   rivi()->setNetto( euro );
   ui->euroEdit->setEuro( rivi()->brutto() );

   aliRiviaMuokattu();
}

void TilioteKirjaaja::alvMuuttuu()
{

   int alvkoodi = ui->alvCombo->currentData( VerotyyppiModel::KoodiRooli ).toInt();
   if( !ladataan_) {
        rivi()->setAlvkoodi( alvkoodi );
   }


   bool naytaMaara = rivi()->naytaBrutto();
   bool naytaVeroton = rivi()->naytaNetto() && ui->alvCombo->isVisible();

   ui->euroLabel->setVisible( naytaMaara );
   ui->euroEdit->setVisible( naytaMaara );

   ui->verotonLabel->setVisible(naytaVeroton);
   ui->verotonEdit->setVisible(naytaVeroton);

   if( !naytaMaara && !rivi()->nettoSyotetty()) {
        Euro maara = rivi()->brutto();
        ui->verotonEdit->setEuro(maara);
        rivi()->setNetto(maara);
   }
   if( !naytaVeroton && rivi()->nettoSyotetty()) {
        Euro maara = rivi()->netto();
        ui->euroEdit->setEuro(maara);
        rivi()->setBrutto(maara);
   }

   ui->alvProssaCombo->setVisible( !ui->alvCombo->currentData(VerotyyppiModel::NollaLajiRooli).toBool() && ui->alvCombo->isVisible());

   ui->eiVahennysCheck->setVisible(  rivi()->naytaVahennysvalinta() && ui->alvProssaCombo->isVisible());
   if( !ladataan_ )
       ui->eiVahennysCheck->setChecked( false );

   if( !ui->alvCombo->currentData(VerotyyppiModel::NollaLajiRooli).toBool() && alvProssa() < 0.1 ) {
        const double alvProssa = yleinenAlv(ui->pvmEdit->date()) / 100.0;
        ui->alvProssaCombo->setCurrentText(QString("%L1 %").arg(alvProssa, 0, 'f', 2));
   }

   alvProssaMuttuu();   // Pitää päivittää joka tapauksessa!
   aliRiviaMuokattu();

}

void TilioteKirjaaja::alvProssaMuttuu()
{
   double prossa = alvProssa();
   if( !ladataan_) {
        rivi()->setAlvprosentti(prossa);
   }
   ui->euroEdit->setEuro( rivi()->brutto() );
   ui->verotonEdit->setEuro( rivi()->netto());
   aliRiviaMuokattu();
}

void TilioteKirjaaja::alvVahennettavaMuuttuu()
{
   rivi()->setAlvvahennys( !ui->eiVahennysCheck->isChecked() );
   aliRiviaMuokattu();
}

void TilioteKirjaaja::ylaTabMuuttui(int tab)
{
   menoa_ = (tab == TILILTA);

   if( menoa_ ) {
        ui->alaTabs->setTabText(MAKSU, tr("Maksett&u lasku"));
        ui->alaTabs->setTabIcon(TULOMENO, QIcon(":/pic/poista.png") ) ;
        ui->alaTabs->setTabIcon(SIIRTO, QIcon(":/pic/tililta.png"));
        ui->alaTabs->setTabText(TULOMENO, tr("&Meno"));
        if( ui->alaTabs->count() > VAKIOVIITE)
            ui->alaTabs->removeTab(VAKIOVIITE);
    } else {
        ui->alaTabs->setTabText(MAKSU, tr("Saap&uva maksu"));
        ui->alaTabs->setTabIcon(TULOMENO, QIcon(":/pic/lisaa.png") ) ;
        ui->alaTabs->setTabIcon(SIIRTO, QIcon(":/pic/tilille.png"));
        ui->alaTabs->setTabText(TULOMENO, tr("&Tulo"));
        if( ui->alaTabs->count() == VAKIOVIITE )
            ui->alaTabs->addTab(QIcon(":/pic/viivakoodi.png"), tr("&Vakioviite"));
    }

    alaTabMuuttui( ui->alaTabs->currentIndex() );

    if( !ladataan_ )
        paivitaVeroFiltteri( ui->alvCombo->currentData().toInt() );

}

void TilioteKirjaaja::tiliMuuttuu()
{
    Tili tili = ui->tiliEdit->valittuTili();
    rivi()->setTili( tili.numero() );
    aliRiviaMuokattu();

    bool erat = tili.eritellaankoTase() &&
            ui->alaTabs->currentIndex() != MAKSU;
    ui->eraLabel->setVisible(erat);
    ui->eraCombo->setVisible(erat);
    if( erat ) {
        ui->eraCombo->asetaTili(tili.numero(), ui->asiakastoimittaja->id());
        if( rivi()->era().id() == 0 )
            ui->eraCombo->valitseUusiEra();
    }

    bool tasapoisto = tili.onko(TiliLaji::TASAERAPOISTO);
    ui->poistoaikaLabel->setVisible(tasapoisto);
    ui->poistoaikaSpin->setVisible(tasapoisto);
    ui->poistoaikaSpin->setValue( tili.luku("tasaerapoisto") / 12 );

    bool jakso = tili.onko(TiliLaji::TULOS) &&
            (ui->alaTabs->currentIndex() == TULOMENO || ui->alaTabs->currentIndex() == SIIRTO);

    ui->jaksotusLabel->setVisible(jakso);
    ui->jaksoAlkaaEdit->setVisible(jakso);
    ui->jaksoViivaLabel->setVisible(jakso);
    ui->jaksoLoppuuEdit->setVisible(jakso);

    if( !ladataan_) {
        if(tili.luku("kohdennus"))
            ui->kohdennusCombo->valitseKohdennus(tili.luku("kohdennus"));

        if( kp()->asetukset()->onko(AsetusModel::AlvVelvollinen)) {
            paivitaVeroFiltteri( tili.alvlaji() );
            const double alvProssa = tili.alvprosentti() == 24.0 ? yleinenAlv(ui->pvmEdit->date()) / 100.0 : tili.alvprosentti();
            ui->alvProssaCombo->setCurrentText(QString("%L1 %").arg(alvProssa, 0, 'f', 2));
        }
    }

    const bool naytaAlv = kp()->asetukset()->onko(AsetusModel::AlvVelvollinen) &&
                          ( tili.onko(TiliLaji::TULOS) || tili.onko(TiliLaji::POISTETTAVA)) &&
                          ui->alaTabs->currentIndex() != MAKSU;
    ui->alvLabel->setVisible(naytaAlv);
    ui->alvCombo->setVisible(naytaAlv);

    alvMuuttuu();
    alvProssaMuttuu();

}


void TilioteKirjaaja::eraValittu(EraMap era)
{
    if( !ui->euroEdit->asCents() && era.saldo())
        ui->euroEdit->setEuro(menoa_ ? Euro(0) - era.saldo() : era.saldo());
    if( ui->seliteEdit->toPlainText().isEmpty())
        ui->seliteEdit->setText(era.nimi());
//    haeAlkuperaisTosite(eraId);

}

void TilioteKirjaaja::jaksomuuttuu(const QDate &pvm)
{
    ui->jaksoLoppuuEdit->setEnabled( pvm.isValid() );
    ui->jaksoLoppuuEdit->setDateRange( pvm, QDate() );
    if( !pvm.isValid())
        ui->jaksoLoppuuEdit->setNull();
}

void TilioteKirjaaja::valitseLasku()
{
    QModelIndex index = ui->maksuView->currentIndex();

    if( index.isValid() && ui->alaTabs->currentIndex() == MAKSU && ui->euroEdit->euro() == Euro::Zero) {
        double avoinna = index.data(LaskuTauluModel::AvoinnaRooli).toDouble();
        ui->euroEdit->setValue( avoinna  );

        tarkastaTallennus();
    }
}

void TilioteKirjaaja::suodata(const QString &teksti)
{
    if( teksti.startsWith("RF") || teksti.contains(  QRegularExpression("^\\d+$")))
    {
        maksuProxy_->setFilterKeyColumn( LaskuTauluModel::NUMERO );
        maksuProxy_->setFilterRegularExpression(QRegularExpression("^" + teksti));
    } else {
        maksuProxy_->setFilterKeyColumn( LaskuTauluModel::ASIAKASTOIMITTAJA);
        maksuProxy_->setFilterCaseSensitivity(Qt::CaseInsensitive);
        maksuProxy_->setFilterFixedString( teksti );
    }
//    if( ui->maksuView->model()->rowCount() && !ladataan_)
//        ui->alaTabs->setCurrentIndex(MAKSU);
}

void TilioteKirjaaja::tyhjenna()
{
    rivit_->clear();
    rivit_->lisaaRivi( oletustili() );
    nykyAliRiviIndeksi_ = 0;

    ui->asiakastoimittaja->clear();
    ui->maksuView->clearSelection();
    ui->pvmEdit->setFocus();
    ui->merkkausCC->haeMerkkaukset( ui->pvmEdit->date() );
    ui->euroEdit->clear();
    ui->seliteEdit->clear();    

    tiliMuuttuu();

    ui->viennitView->selectRow(0);
    ui->viennitView->hide();

    tarkastaTallennus();
    avoinProxy_->setFilterFixedString("€");
    QTimer::singleShot(50, this, &TilioteKirjaaja::tiliMuuttuu);

}

void TilioteKirjaaja::tarkastaTallennus()
{
    bool ok = ui->euroEdit->euro() &&
              ( ui->tiliEdit->valittuTilinumero() ||
                !ui->maksuView->selectionModel()->selectedRows().isEmpty() );
    ui->okNappi->setEnabled( ok );
}



void TilioteKirjaaja::kumppaniTiedot(const QVariantMap& data)
{
    if( menoa_ ) {
        if( data.contains("menotili"))
            ui->tiliEdit->valitseTiliNumerolla( data.value("menotili").toInt() );
        else
            tiliMuuttuu();
    } else {
        if( data.contains("tulotili"))
            ui->tiliEdit->valitseTiliNumerolla( data.value("tulotili").toInt());
        else
            tiliMuuttuu();
    }
}

void TilioteKirjaaja::haeAlkuperaisTosite(int eraId)
{
    KpKysely *kysely = kpk("/tositteet");
    kysely->lisaaAttribuutti("vienti",eraId);
    connect(kysely, &KpKysely::vastaus, this, &TilioteKirjaaja::tositeSaapuu);
    kysely->kysy();
}

void TilioteKirjaaja::tositeSaapuu(QVariant *data)
{
    QVariantMap map = data->toMap();
    alkuperaisRivit_ = map.value("viennit").toList();
}

void TilioteKirjaaja::lisaaVienti()
{
    const int uusiRivi = rivit_->lisaaRivi( oletustili(), ui->pvmEdit->date() );
    ui->viennitView->setVisible(true);
    ui->viennitView->selectRow(uusiRivi);
    ui->poistaVientiNappi->setVisible( true );
    QTimer::singleShot(50, this, &TilioteKirjaaja::tiliMuuttuu);
}

void TilioteKirjaaja::poistaVienti()
{
    ui->viennitView->setVisible( rivit_->rowCount() > 2 );
    ui->poistaVientiNappi->setVisible( rivit_->rowCount() > 2);
    rivit_->poistaRivi(nykyAliRiviIndeksi_);
    ui->viennitView->selectRow( qMin(nykyAliRiviIndeksi_, rivit_->rowCount()-1) );
}



void TilioteKirjaaja::alusta()
{
    ui->setupUi(this);
    ui->viennitView->setModel(rivit_);

    ui->ylaTab->addTab(QIcon(":/pic/lisaa.png"), tr("T&ilille"));
    ui->ylaTab->addTab(QIcon(":/pic/poista.png"), tr("Tililt&ä"));

    ui->alaTabs->addTab(QIcon(":/pic/lasku.png"), tr("Lask&un maksu"));
    ui->alaTabs->addTab(QIcon(":/pic/lisaa.png"), tr("&Tulo"));
    ui->alaTabs->addTab(QIcon(":/pic/tilille.png"), tr("&Siirto"));


    veroFiltteri_ = new QSortFilterProxyModel(this);
    veroFiltteri_->setFilterRole( VerotyyppiModel::KoodiTekstiRooli);
    veroFiltteri_->setSourceModel( kp()->alvTyypit());
    ui->alvCombo->setModel(veroFiltteri_);
    ui->alvProssaCombo->addItems( QStringList() << "25,50 %" << "24,00 %" << "14,00 %" << "10,00 %");
    ui->alvProssaCombo->setValidator(new QRegularExpressionValidator(QRegularExpression("\\d{1,2}(,\\d{1,2})\\s?%?"),this));

    alaTabMuuttui(0);

    connect( ui->euroEdit, &KpEuroEdit::euroMuuttui, this, &TilioteKirjaaja::euroMuuttuu);
    connect( ui->verotonEdit, &KpEuroEdit::euroMuuttui, this, &TilioteKirjaaja::verotonMuuttuu);

    connect( ui->alaTabs, &QTabBar::currentChanged, this, &TilioteKirjaaja::alaTabMuuttui);
    connect( ui->ylaTab, &QTabBar::currentChanged, this, &TilioteKirjaaja::ylaTabMuuttui);    

    maksuProxy_->setSourceModel( laskut_ );

    avoinProxy_->setSourceModel(maksuProxy_);
    avoinProxy_->setFilterRole(Qt::DisplayRole);
    avoinProxy_->setFilterKeyColumn(LaskuTauluModel::MAKSAMATTA);
    avoinProxy_->setFilterFixedString("€");

    ui->maksuView->setModel(avoinProxy_);
    ui->maksuView->setSortingEnabled(true);
    avoinProxy_->setDynamicSortFilter(true);


    ui->maksuView->hideColumn( LaskuTauluModel::LAHETYSTAPA );
    connect( ui->maksuView, &QTableView::clicked , this, &TilioteKirjaaja::valitseLasku);
    connect( ui->suodatusEdit, &QLineEdit::textEdited, this, &TilioteKirjaaja::suodata);

    connect( ui->suljeNappi, &QPushButton::clicked,
             this, &TilioteKirjaaja::tyhjenna);

    connect( ui->pvmEdit, &KpDateEdit::dateChanged, ui->merkkausCC, &CheckCombo::haeMerkkaukset);
    connect( ui->pvmEdit, &KpDateEdit::dateChanged, ui->kohdennusCombo, &KohdennusCombo::suodataPaivalla);


    tyhjenna();
    connect( ui->pvmEdit, &KpDateEdit::dateChanged, this, &TilioteKirjaaja::tarkastaTallennus);

    connect( ui->alvProssaCombo, &QComboBox::currentTextChanged, this, &TilioteKirjaaja::alvProssaMuttuu);
    connect( ui->alvCombo, &QComboBox::currentTextChanged, this, &TilioteKirjaaja::alvMuuttuu );
    connect( ui->tiliEdit, &TilinvalintaLine::textChanged, this, &TilioteKirjaaja::tiliMuuttuu);
    connect( ui->eiVahennysCheck, &QCheckBox::toggled, this, &TilioteKirjaaja::alvVahennettavaMuuttuu);

    connect( ui->maksuView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &TilioteKirjaaja::tarkastaTallennus);
    connect( ui->eraCombo, &EraCombo::valittu, this, &TilioteKirjaaja::eraValittu);
    connect( ui->jaksoAlkaaEdit, &KpDateEdit::dateChanged, this, &TilioteKirjaaja::jaksomuuttuu);


    connect( ui->asiakastoimittaja, &AsiakasToimittajaValinta::muuttui, this, &TilioteKirjaaja::kumppaniTiedot);
    connect( ui->ohjeNappi, &QPushButton::clicked, [] { kp()->ohje("kirjaus/tiliote"); });
    connect( ui->tyhjaaNappi, &QPushButton::clicked, this, &TilioteKirjaaja::tyhjenna);
    connect( laskut_, &LaskuTauluTilioteProxylla::modelReset, this, [this] { this->suodata(this->ui->suodatusEdit->text()); ui->maksuView->resizeColumnToContents(LaskuTauluModel::ASIAKASTOIMITTAJA); });

    connect( ui->viennitView->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &TilioteKirjaaja::riviVaihtuu);

    connect( ui->lisaaVientiNappi, &QPushButton::clicked, this, &TilioteKirjaaja::lisaaVienti);
    connect( ui->poistaVientiNappi, &QPushButton::clicked, this, &TilioteKirjaaja::poistaVienti);

    ylaTabMuuttui( ui->ylaTab->currentIndex() );
}

double TilioteKirjaaja::alvProssa()
{
    if(ui->alvCombo->currentData(VerotyyppiModel::NollaLajiRooli).toBool() )
        return 0.0;

    QString txt = ui->alvProssaCombo->currentText();
    txt.replace(",",".");
    QRegularExpression valiRe("[^\\d\\.]");
    int vali = txt.indexOf(valiRe);
    if( vali > 0)
        txt = txt.left(vali);
    return txt.toDouble();
}

TilioteApuri *TilioteKirjaaja::apuri() const
{
    return qobject_cast<TilioteApuri*>( parent() );
}

void TilioteKirjaaja::lataa(const TilioteKirjausRivi &rivi)
{
    ladataan_ = true;
    const Euro& maara = rivi.summa();

    rivit_->asetaTyyppi(rivi.tyyppi(), maara > Euro::Zero);
    rivit_->asetaRivit(rivi.rivit());

    ui->viennitView->selectRow(0);
    ui->viennitView->setVisible( rivit_->rowCount() > 1 );
    qApp->processEvents();
    naytaRivi();

    ui->pvmEdit->setDate(rivi.pvm());
    ui->asiakastoimittaja->valitse( rivi.kumppani() );

    if( rivit_->rowCount() < 2)
        ui->seliteEdit->setText( rivi.otsikko() );

    QString saajamaksaja = rivi.kumppani().value("nimi").toString();
    int valinpaikka = saajamaksaja.indexOf(QRegularExpression("\\W",QRegularExpression::UseUnicodePropertiesOption));
    if( valinpaikka > 2)
        saajamaksaja = saajamaksaja.left(valinpaikka);

    const int tili = rivit_->eka().tilinumero();

    if( maara) {
        ui->ylaTab->setCurrentIndex( rivi.summa() < Euro::Zero ? TILILTA: TILILLE);
    } else if( tili) {
        Tili* tiliObj = kp()->tilit()->tili(tili);
        ui->ylaTab->setCurrentIndex( tili && tiliObj->onko(TiliLaji::MENO) ? TILILTA : TILILLE );
    }

    if( rivi.tyyppi() == TositeVienti::SIIRTO)
        ui->alaTabs->setCurrentIndex( SIIRTO );
    else if( rivi.tyyppi() == TositeVienti::SUORITUS )
        ui->alaTabs->setCurrentIndex( MAKSU );
    else
        ui->alaTabs->setCurrentIndex(TULOMENO );

    // Etsitään valittava rivi
    avoinProxy_->setFilterFixedString("");

    bool maksu = false;
    const int eraId = rivit_->eka().eraId();
    ui->eraCombo->valitse(rivit_->eka().era() );

    if( eraId) {
        for(int i=0; i < avoinProxy_->rowCount(); i++) {
            if( avoinProxy_->data( avoinProxy_->index(i,0), LaskuTauluModel::EraIdRooli ).toInt() == eraId) {
                ui->maksuView->selectRow(i);
                maksu = true;
                break;
            }
        }
    }
    if( eraId && !maksu) {
        ui->alaTabs->setCurrentIndex( SIIRTO );        
    }

    ui->suodatusEdit->setText( saajamaksaja );
    if( !saajamaksaja.isEmpty())
        suodata(saajamaksaja);

    ladataan_ = false;
}

void TilioteKirjaaja::riviVaihtuu(const QModelIndex &current, const QModelIndex &previous)
{
    if( previous.isValid() && !ladataan_) {
        tallennaRivi();
    }

    ui->euroEdit->setProperty("EiMiinus", rivit_->rowCount() == 1);
    ui->verotonEdit->setProperty("EiMiinus", rivit_->rowCount() == 1);

    naytaRivi();
}


void TilioteKirjaaja::naytaRivi()
{
    const int rivi = ui->viennitView->currentIndex().row();
    if( rivi < 0)
        return;
    nykyAliRiviIndeksi_ = rivi;

    const ApuriRivi& ar = rivit_->at(rivi);


    // Yksirivinen ei voi olla "väärän" merkkinen!
    if( ar.naytaBrutto())
        ui->euroEdit->setEuro(rivit_->rowCount() == 1 ? ar.brutto().abs() : ar.brutto());
    else
        ui->verotonEdit->setEuro(rivit_->rowCount() == 1 ? ar.netto().abs() : ar.netto());
    if( !ar.netto())
        ui->verotonEdit->setEuro(Euro::Zero);

    ui->tiliEdit->valitseTiliNumerolla(ar.tilinumero());
    ui->eraCombo->valitse( ar.era());
    ui->kohdennusCombo->valitseKohdennus( ar.kohdennus() );
    ui->merkkausCC->setSelectedItems( ar.merkkaukset() );
    ui->seliteEdit->setText( ar.selite() );

    if(ar.jaksoalkaa().isValid())
        ui->jaksoAlkaaEdit->setDate( ar.jaksoalkaa() );
    else
        ui->jaksoAlkaaEdit->setNull();

    if(ar.jaksopaattyy().isValid())
        ui->jaksoLoppuuEdit->setDate( ar.jaksopaattyy() );
    else
        ui->jaksoLoppuuEdit->setNull();

    paivitaVeroFiltteri( ar.alvkoodi() );
    ui->alvProssaCombo->setCurrentText( ar.alvprosentti() ? QString("%L1 %").arg(ar.alvprosentti(), 0, 'f', 2 ) : QString() );
    qApp->processEvents();

    ui->eiVahennysCheck->setChecked( !ar.alvvahennys() );
    ui->poistoaikaSpin->setValue( ar.poistoaika() / 12 );

}


void TilioteKirjaaja::tallennaRivi()
{
    const int rivi = nykyAliRiviIndeksi_;
    if( rivi < 0 || ladataan_)
        return;

    ApuriRivi* aliRivi = rivit_->rivi(rivi);
    Tili tili = ui->tiliEdit->valittuTili();

    aliRivi->setTili( ui->tiliEdit->valittuTilinumero());
    aliRivi->setKohdennus( ui->kohdennusCombo->kohdennus());
    if( tili.eritellaankoTase())
        aliRivi->setEra(ui->eraCombo->eraMap());
    else
        aliRivi->setEra(EraMap());
    aliRivi->setMerkkaukset( ui->merkkausCC->selectedDatas());
    aliRivi->setJaksoalkaa( ui->jaksoAlkaaEdit->date());
    aliRivi->setJaksopaattyy( ui->jaksoLoppuuEdit->date());
    aliRivi->setSelite( ui->seliteEdit->toPlainText() );
    aliRivi->setPoistoaika( ui->poistoaikaSpin->isVisible() ? ui->poistoaikaSpin->value() * 12 : 0 );

    if( aliRivi->naytaBrutto())
        aliRivi->setBrutto( ui->euroEdit->euro() );
    else
        aliRivi->setNetto(  ui->verotonEdit->euro() );

}

TositeVienti::VientiTyyppi TilioteKirjaaja::tyyppi()
{
    switch( ui->alaTabs->currentIndex() ) {
    case MAKSU:
        return TositeVienti::SUORITUS;
    case TULOMENO:
    {
        ApuriRivi* aliRivi = rivit_->rivi(0);
        if( aliRivi ) {
            Tili* tili = kp()->tilit()->tili(aliRivi->tilinumero());
            if( tili && tili->onko(TiliLaji::TULO))
                return TositeVienti::MYYNTI;
            else if( tili && tili->onko(TiliLaji::MENO))
                return TositeVienti::OSTO;
        }
        return menoa_ ? TositeVienti::OSTO : TositeVienti::MYYNTI;
    }
    case SIIRTO:
        return TositeVienti::SIIRTO;
    default:
        return TositeVienti::TUNTEMATON;
    }
}

TilioteKirjausRivi TilioteKirjaaja::tallennettava()
{
    TilioteKirjausRivi rivi( apuri()->model() );
    if( riviIndeksi_ > -1)
        rivi = apuri()->model()->rivi(riviIndeksi_);

    rivi.asetaPvm( ui->pvmEdit->date() );
    rivi.asetaKumppani( ui->asiakastoimittaja->map());
    TositeVienti::VientiTyyppi vientiTyyppi = tyyppi();
    rivi.asetaTyyppi(vientiTyyppi);
    rivit_->asetaTyyppi(tyyppi(), !menoa_ );
    if( rivit_->rowCount() == 1) {
        rivi.asetaOtsikko( ui->seliteEdit->toPlainText() );
    }

    if( ui->alaTabs->currentIndex() == MAKSU) {
        QModelIndex index = ui->maksuView->currentIndex();
        const QString& selite = index.data(LaskuTauluModel::SeliteRooli).toString();
        rivi.asetaOtsikko(selite);
        rivi.asetaKumppani(index.data(LaskuTauluModel::KumppaniMapRooli).toMap() );
        ApuriRivi aliRivi;
        aliRivi.setSelite(selite);
        aliRivi.setTili( index.data(LaskuTauluModel::TiliRooli).toInt() );
        aliRivi.setEra(index.data(LaskuTauluModel::EraMapRooli).toMap());
        aliRivi.setBrutto( ui->euroEdit->euro() );
        rivit_->clear();
        rivit_->lisaaRivi(aliRivi);

    } else if( ui->alaTabs->currentIndex() == VAKIOVIITE) {
        QModelIndex index = ui->maksuView->currentIndex();
        const QVariantMap& map = index.data(VakioViiteModel::MapRooli).toMap();

        rivi.asetaOtsikko(map.value("otsikko").toString());
        rivi.asetaKumppani( ui->asiakastoimittaja->map() );
        rivi.asetaViite( map.value("viite").toString() );

        ApuriRivi aliRivi;
        aliRivi.setSelite( ui->seliteEdit->toPlainText());
        aliRivi.setTili( map.value("tili").toInt());
        aliRivi.setKohdennus( map.value("kohdennus").toInt());
        aliRivi.setBrutto( ui->euroEdit->euro() );
        rivit_->clear();
        rivit_->lisaaRivi(aliRivi);
    } else {
        tallennaRivi();
    }


    rivi.asetaRivit(rivit_);
    return rivi;
}

void TilioteKirjaaja::paivitaVeroFiltteri(const int verokoodi)
{
    if( !kp()->onkoAlvVelvollinen( ui->pvmEdit->date() ))  {
        veroFiltteri_->setFilterRegularExpression( QRegularExpression( "^0" ) );
        ui->alvCombo->setCurrentIndex( 0 );
        return;
    }


    Tili tili = ui->tiliEdit->valittuTili();

    if( tili.onko(TiliLaji::POISTETTAVA)) {
        veroFiltteri_->setFilterRegularExpression( QRegularExpression( "^(0|12][12]|" + QString::number(verokoodi) + ")" ) );
    } else if( tili.onko(TiliLaji::MENO)) {
        veroFiltteri_->setFilterRegularExpression( QRegularExpression( "^(0|2[1-69]|" + QString::number(verokoodi) + ")" ) );
    } else {
        veroFiltteri_->setFilterRegularExpression( QRegularExpression( "^(0|1[1-79]|" + QString::number(verokoodi) + ")" ) );
    }
    ui->alvCombo->setCurrentIndex( ui->alvCombo->findData(verokoodi, VerotyyppiModel::KoodiRooli) );
}

void TilioteKirjaaja::aliRiviaMuokattu()
{
    emit rivit_->dataChanged(
        rivit_->index(nykyAliRiviIndeksi_, 0),
        rivit_->index(nykyAliRiviIndeksi_, 2));

    if( rivit_->rowCount() > 1) {
        const Euro yhteensa =
            rivit_->plusOnKredit() ? rivit_->summa() : Euro::Zero - rivit_->summa();
        ui->summaLabel->setText(yhteensa);
    } else {
        ui->summaLabel->clear();
    }

    tarkastaTallennus();

}

ApuriRivi *TilioteKirjaaja::rivi() const
{
    return rivit_->rivi(nykyAliRiviIndeksi_);
}

int TilioteKirjaaja::oletustili() const
{
    if( ui->alaTabs->currentIndex() == TULOMENO )
        return menoa_ ? kp()->asetukset()->luku(AsetusModel::OletusMenotili, 4000) : kp()->asetukset()->luku(AsetusModel::OletusMyyntitili, 3000);
    else
        return 0;
}







