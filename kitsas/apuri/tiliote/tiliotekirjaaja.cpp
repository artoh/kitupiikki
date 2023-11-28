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
#include "tiliotekirjaaja.h"
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

TilioteKirjaaja::TilioteKirjaaja(TilioteApuri *apuri) :
    QDialog(apuri),
    ui(new Ui::TilioteKirjaaja),
    maksuProxy_(new QSortFilterProxyModel(this)),
    avoinProxy_( new QSortFilterProxyModel(this)),
    laskut_( new LaskuTauluTilioteProxylla(this, apuri->model())),
    rivi_{apuri->model()}
{
    aliRiviModel_ = new TilioteAliRivitModel(kp(), &rivi_,  this);
    alusta();
    ui->pvmEdit->setDate( apuri->tosite()->data(Tosite::PVM).toDate() );

}

TilioteKirjaaja::TilioteKirjaaja(SiirtoApuri *apuri):
    QDialog(apuri),
    ui(new Ui::TilioteKirjaaja),
    maksuProxy_(new QSortFilterProxyModel(this)),
    avoinProxy_( new QSortFilterProxyModel(this)),
    laskut_( new LaskuTauluModel(this)),
    rivi_{nullptr}
{
    aliRiviModel_ = new TilioteAliRivitModel(kp(), &rivi_,  this);
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
    delete ui;
    delete aliRiviModel_;
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
                siirto->laskuMaksettu(
                    ui->pvmEdit->date(),
                    ui->seliteEdit->toPlainText(),
                    ui->tiliEdit->valittuTilinumero(),
                    ui->eraCombo->eraMap(),
                    ui->asiakastoimittaja->map(),
                    ui->euroEdit->euro(),
                    menoa_
                );
                QDialog::accept();
            }
        } else {
            tallennaRivi();
            tallenna();
            rivi_.paivitaTyyppi();

            if( riviIndeksi_ > -1) {
                apuri()->model()->asetaRivi(riviIndeksi_, rivi_);
                QDialog::accept();
            } else {                
                apuri()->model()->lisaaRivi(rivi_);
            }
            rivi_.paivitaErikoisrivit();
            tyhjenna();
        }
    }
}

void TilioteKirjaaja::kirjaaUusia(const QDate &pvm)
{
    setWindowTitle( tr("Kirjaa tiliotteelle"));
    nykyAliRiviIndeksi_ = -1;
    tyhjenna();
    ui->pvmEdit->setDate(pvm);

    show();
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

    } else if( tab == TULOMENO || tab == HYVITYS) {
        bool menotili = (ui->ylaTab->currentIndex() == TILILTA) ^ (tab == HYVITYS);

        ui->tiliLabel->setText( menotili ? tr("Menotili") : tr("Tulotili"));
        ui->asiakasLabel->setText( menotili ? tr("Toimittaja") : tr("Asiakas"));
        ui->tiliEdit->suodataTyypilla( menotili ? "D.*" : "C.*");
        Tili* valittuna = ui->tiliEdit->tili();
        if( !valittuna || (menotili && !valittuna->onko(TiliLaji::MENO)) || (!menotili && !valittuna->onko(TiliLaji::TULO)) )
            ui->tiliEdit->valitseTiliNumerolla(  menotili ? kp()->asetukset()->luku("OletusMenotili") : kp()->asetukset()->luku("OletusMyyntitili") );    // TODO: Tod. oletukset
    } else if ( tab == SIIRTO ) {
        ui->tiliLabel->setText( menoa_ ? tr("Tilille") : tr("Tililtä")  );
        ui->asiakasLabel->setText( menoa_ ? tr("Saaja") : tr("Maksaja"));
        ui->tiliEdit->suodataTyypilla( "[AB].*");

    } else if( tab == VAKIOVIITE) {
        ui->maksuView->setModel( kp()->vakioViitteet() );
    }

    const bool alv = (tab == TULOMENO || tab == HYVITYS) && kp()->asetukset()->onko(AsetusModel::AlvVelvollinen);
    ui->alvLabel->setVisible(alv);
    ui->alvCombo->setVisible(alv);
    ui->alvProssaCombo->setVisible(alv);
    ui->verotonEdit->setVisible(alv);
    ui->verotonLabel->setVisible(alv);

    tiliMuuttuu();
    paivitaVientiNakyma();

    ui->viennitView->horizontalHeader()->setSectionResizeMode(TilioteViennit::TILI, QHeaderView::Stretch);

}

void TilioteKirjaaja::euroMuuttuu()
{
   const Euro& euro = ui->euroEdit->euro();
   if( euro == aliRivi()->brutto())
    return;

   if( euro && !nykyAliRiviIndeksi_) {
        ui->ylaTab->setCurrentIndex( ui->euroEdit->miinus() ? 1 : 0 );
   }

   aliRivi()->setBrutto( euro );
   ui->verotonEdit->setEuro( aliRivi()->netto());
   ui->verotonEdit->setMiinus( ui->euroEdit->miinus());

   aliRiviaMuokattu();

}

void TilioteKirjaaja::verotonMuuttuu()
{
   const Euro& euro = ui->verotonEdit->euro();
   if( euro == aliRivi()->netto())
        return;

   if( euro && !nykyAliRiviIndeksi_ ) {
        ui->ylaTab->setCurrentIndex( ui->euroEdit->miinus() ? 1 : 0);
   }

   aliRivi()->setNetto( euro );
   ui->euroEdit->setEuro( aliRivi()->brutto() );
   ui->euroEdit->setMiinus( ui->verotonEdit->miinus() );

   aliRiviaMuokattu();
}

void TilioteKirjaaja::alvMuuttuu()
{
   int alvkoodi = ui->alvCombo->currentData( VerotyyppiModel::KoodiRooli ).toInt();
   aliRivi()->setAlvkoodi( alvkoodi );

   bool naytaMaara = aliRivi()->naytaBrutto();
   bool naytaVeroton = aliRivi()->naytaNetto();

   ui->euroLabel->setVisible( naytaMaara );
   ui->euroEdit->setVisible( naytaMaara );

   ui->verotonLabel->setVisible(naytaVeroton);
   ui->verotonEdit->setVisible(naytaVeroton);

   if( !naytaMaara && !aliRivi()->nettoSyotetty()) {
        Euro maara = aliRivi()->brutto();
        ui->verotonEdit->setEuro(maara);
        aliRivi()->setNetto(maara);
   }

   ui->alvProssaCombo->setVisible( !ui->alvCombo->currentData(VerotyyppiModel::NollaLajiRooli).toBool() );
   ui->eiVahennysCheck->setVisible(  aliRivi()->naytaVahennysvalinta() );
   ui->eiVahennysCheck->setChecked( false );

   if( !ui->alvCombo->currentData(VerotyyppiModel::NollaLajiRooli).toBool() && alvProssa() < 0.1 ) {
        ui->alvProssaCombo->setCurrentText("24 %");
   }


   aliRiviaMuokattu();

}

void TilioteKirjaaja::alvProssaMuttuu()
{
   double prossa = alvProssa();
   aliRivi()->setAlvprosentti(prossa);
   ui->euroEdit->setEuro( aliRivi()->brutto() );
   ui->verotonEdit->setEuro( aliRivi()->netto());
   aliRiviaMuokattu();
}

void TilioteKirjaaja::ylaTabMuuttui(int tab)
{
    menoa_ = tab;
    if( menoa_ ) {
        ui->alaTabs->setTabText(MAKSU, tr("Maksettu lasku"));
        ui->alaTabs->setTabIcon(TULOMENO, QIcon(":/pic/poista.png") ) ;
        ui->alaTabs->setTabText(TULOMENO, tr("Meno"));
        if( ui->alaTabs->count() > VAKIOVIITE)
            ui->alaTabs->removeTab(VAKIOVIITE);
    } else {
        ui->alaTabs->setTabText(MAKSU, tr("Saapuva maksu"));
        ui->alaTabs->setTabIcon(TULOMENO, QIcon(":/pic/lisaa.png") ) ;
        ui->alaTabs->setTabText(TULOMENO, tr("Tulo"));
        if( ui->alaTabs->count() == VAKIOVIITE )
            ui->alaTabs->addTab(QIcon(":/pic/viivakoodi.png"), tr("Vakioviite"));
    }
    alaTabMuuttui( ui->alaTabs->currentIndex() );

    paivitaVeroFiltteri( ui->alvCombo->currentData().toInt() );

    ui->euroEdit->setMiinus( tab );
    ui->verotonEdit->setMiinus( tab );
}

void TilioteKirjaaja::tiliMuuttuu()
{
    Tili tili = ui->tiliEdit->valittuTili();
    aliRivi()->setTili( tili.numero() );
    aliRiviaMuokattu();

    bool erat = tili.eritellaankoTase() &&
            ui->alaTabs->currentIndex() != MAKSU;
    ui->eraLabel->setVisible(erat);
    ui->eraCombo->setVisible(erat);
    if( erat ) {
        ui->eraCombo->asetaTili(tili.numero(), ui->asiakastoimittaja->id());
    }

    bool jakso = tili.onko(TiliLaji::TULOS) &&
            (ui->alaTabs->currentIndex() == TULOMENO || ui->alaTabs->currentIndex() == HYVITYS);

    ui->jaksotusLabel->setVisible(jakso);
    ui->jaksoAlkaaEdit->setVisible(jakso);
    ui->jaksoViivaLabel->setVisible(jakso);
    ui->jaksoLoppuuEdit->setVisible(jakso);

    if(tili.luku("kohdennus"))
        ui->kohdennusCombo->valitseKohdennus(tili.luku("kohdennus"));

    const int alvIndeksi = ui->alvCombo->findData(tili.alvlaji());
    ui->alvCombo->setCurrentIndex(alvIndeksi);
    ui->alvProssaCombo->setCurrentText(QString("%1 %").arg(qRound(tili.alvprosentti())));

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
    ui->jaksoLoppuuEdit->setDateRange( pvm, kp()->tilikaudet()->kirjanpitoLoppuu().addYears(10) );
    if( !pvm.isValid())
        ui->jaksoLoppuuEdit->setNull();
}

void TilioteKirjaaja::valitseLasku()
{
    QModelIndex index = ui->maksuView->currentIndex();

    if( index.isValid() && ui->alaTabs->currentIndex() == MAKSU && ui->euroEdit->euro() == Euro::Zero) {
        double avoinna = index.data(LaskuTauluModel::AvoinnaRooli).toDouble();
        ui->euroEdit->setValue( menoa_ ? 0 - avoinna : avoinna  );
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
    ui->asiakastoimittaja->clear();
    ui->maksuView->clearSelection();
    ui->pvmEdit->setFocus();
    ui->merkkausCC->haeMerkkaukset( rivi_.pvm()  );
    ui->euroEdit->clear();
    ui->seliteEdit->clear();

    aliRiviModel_->tyhjenna();
    nykyAliRiviIndeksi_ = 0;

    tiliMuuttuu();

    ui->viennitView->selectRow(0);

    tarkastaTallennus();
    avoinProxy_->setFilterFixedString("€");

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
    tallennaRivi();
    TositeVienti uusi;
    aliRiviModel_->uusiRivi();
    ui->viennitView->selectRow(aliRiviModel_->rowCount()-1);
    ui->tiliEdit->valitseTiliNumerolla(menoa_ ? kp()->asetukset()->luku("OletusMenotili") : kp()->asetukset()->luku("OletusMyyntitili") );

    tiliMuuttuu();

    ui->euroEdit->setMiinus(menoa_);
    ui->verotonEdit->setMiinus(menoa_);
}

void TilioteKirjaaja::poistaVienti()
{
    nykyAliRiviIndeksi_ = -1;
    const int rivi = ui->viennitView->currentIndex().row();
    if( rivi >= 0) {
//        viennit_->poistaVienti(rivi);
        ui->viennitView->selectRow(0);
    }
}

void TilioteKirjaaja::paivitaVientiNakyma()
{
    int alatabu = ui->alaTabs->currentIndex();
    ui->lisaaVientiNappi->setVisible(alatabu == TULOMENO);
//    ui->poistaVientiNappi->setVisible( viennit_->rowCount() > 1 && alatabu == TULOMENO);
//    ui->viennitView->setVisible(viennit_->rowCount() > 1 && alatabu == TULOMENO);
}

void TilioteKirjaaja::alusta()
{
    ui->setupUi(this);
    ui->viennitView->setModel(aliRiviModel_);

    ui->ylaTab->addTab(QIcon(":/pic/lisaa.png"), tr("Tilille"));
    ui->ylaTab->addTab(QIcon(":/pic/poista.png"), tr("Tililtä"));

    ui->alaTabs->addTab(QIcon(":/pic/lasku.png"), tr("Laskun maksu"));
    ui->alaTabs->addTab(QIcon(":/pic/lisaa.png"), tr("Tulo"));
    ui->alaTabs->addTab(QIcon(":/pic/edit-undo.png"), tr("Hyvitys"));
    ui->alaTabs->addTab(QIcon(":/pic/siirra.png"), tr("Siirto"));


    veroFiltteri_ = new QSortFilterProxyModel(this);
    veroFiltteri_->setFilterRole( VerotyyppiModel::KoodiTekstiRooli);
    veroFiltteri_->setSourceModel( kp()->alvTyypit());
    ui->alvCombo->setModel(veroFiltteri_);
    ui->alvProssaCombo->addItems( QStringList() << "24 %" << "14 %" << "10 %");
    ui->alvProssaCombo->setValidator(new QRegularExpressionValidator(QRegularExpression("\\d{1,2}(,\\d{1,2})\\s?%?"),this));

    alaTabMuuttui(0);

    connect( ui->euroEdit, &KpEuroEdit::textChanged, this, &TilioteKirjaaja::euroMuuttuu);
    connect( ui->verotonEdit, &KpEuroEdit::textChanged, this, &TilioteKirjaaja::verotonMuuttuu);
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

    connect( ui->alvProssaCombo, &QComboBox::currentIndexChanged, this, &TilioteKirjaaja::alvProssaMuttuu);
    connect( ui->alvCombo, &QComboBox::currentTextChanged, this, &TilioteKirjaaja::alvMuuttuu );
    connect( ui->tiliEdit, &TilinvalintaLine::textChanged, this, &TilioteKirjaaja::tiliMuuttuu);

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
    rivi_ = rivi;


    lataaNakymaan();
    ui->viennitView->selectRow(0);
    nykyAliRiviIndeksi_ = 0;
    naytaRivi();
}


void TilioteKirjaaja::lataaNakymaan()
{
    ladataan_ = true;

    const int tili = aliRiviModel_->rivi(0).tilinumero();

    ui->pvmEdit->setDate(rivi_.pvm());
    ui->asiakastoimittaja->valitse( rivi_.kumppani() );

    QString saajamaksaja = rivi_.kumppani().value("nimi").toString();
    int valinpaikka = saajamaksaja.indexOf(QRegularExpression("\\W",QRegularExpression::UseUnicodePropertiesOption));
    if( valinpaikka > 2)
        saajamaksaja = saajamaksaja.left(valinpaikka);        

    Euro summa = rivi_.summa();
    ui->ylaTab->setCurrentIndex( summa < Euro::Zero );

    if( rivi_.tyyppi() == TilioteKirjausRivi::SIIRTO)
        ui->alaTabs->setCurrentIndex( SIIRTO );
    else if( rivi_.tyyppi() == TilioteKirjausRivi::SUORITUS )
        ui->alaTabs->setCurrentIndex( MAKSU );
    else if( rivi_.tyyppi() == TilioteKirjausRivi::SIIRTO)
        ui->alaTabs->setCurrentIndex( SIIRTO );
    else {
        if( (rivi_.tyyppi() == TilioteKirjausRivi::OSTO && summa > Euro::Zero) ||
            (rivi_.tyyppi() == TilioteKirjausRivi::MYYNTI  && summa < Euro::Zero)) {
            ui->alaTabs->setCurrentIndex(HYVITYS);
        } else {
            ui->alaTabs->setCurrentIndex(TULOMENO );
        }
    }

    // Etsitään valittava rivi
    avoinProxy_->setFilterFixedString("");

    bool maksu = false;
    const int eraId = aliRiviModel_->rivi(0).eraId();

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

void TilioteKirjaaja::riviVaihtuu(const QModelIndex &/*current*/, const QModelIndex &previous)
{
    if( previous.isValid()) {
        tallennaRivi();
    }
    naytaRivi();
}


void TilioteKirjaaja::naytaRivi()
{
    const int rivi = ui->viennitView->currentIndex().row();
    if( rivi < 0)
        return;
    nykyAliRiviIndeksi_ = rivi;

    const TilioteAliRivi& ar = aliRiviModel_->rivi(rivi);

    ui->euroEdit->setEuro(ar.brutto()); // TODO: Brutto ja netto
    ui->tiliEdit->valitseTiliNumerolla(ar.tilinumero());
    ui->eraCombo->valitse( ar.era());
    ui->kohdennusCombo->valitseKohdennus( ar.kohdennus() );
    ui->merkkausCC->setSelectedItems( ar.merkkaukset() );
//    ui->seliteEdit->setText( vienti.selite() );

    if(ar.jaksoalkaa().isValid())
        ui->jaksoAlkaaEdit->setDate( ar.jaksoalkaa() );
    else
        ui->jaksoAlkaaEdit->setNull();

    if(ar.jaksopaattyy().isValid())
        ui->jaksoLoppuuEdit->setDate( ar.jaksopaattyy() );
    else
        ui->jaksoLoppuuEdit->setNull();

    // Alv-käsittely
    // ui->alvCombo->setCurrentIndex(ui->alvCombo->findData(vienti.alvProsentti()));

    // nykyAliRiviIndeksi_ = rivi;
}


void TilioteKirjaaja::tallennaRivi()
{
    const int rivi = nykyAliRiviIndeksi_;
    if( rivi < 0 || ladataan_)
        return;

    TilioteAliRivi aliRivi = aliRiviModel_->rivi(rivi);

    aliRivi.setTili( ui->tiliEdit->valittuTilinumero());
    aliRivi.setKohdennus( ui->kohdennusCombo->kohdennus());
    aliRivi.setMerkkaukset( ui->merkkausCC->selectedDatas());
    aliRivi.setJaksoalkaa( ui->jaksoAlkaaEdit->date());
    aliRivi.setJaksopaattyy( ui->jaksoLoppuuEdit->date());

    // Tähän bruton ja neton käsittelyt!

    if( aliRivi.naytaBrutto())
        aliRivi.setBrutto( ui->euroEdit->euro() );
    else
        aliRivi.setNetto( ui->euroEdit->euro() );

    aliRiviModel_->korvaa(rivi, aliRivi);
}


void TilioteKirjaaja::tallenna()
{
    rivi_.asetaPvm( ui->pvmEdit->date() );
    rivi_.asetaOtsikko( ui->seliteEdit->toPlainText());
    rivi_.asetaKumppani( ui->asiakastoimittaja->map());

    if ( ui->alaTabs->currentIndex() == MAKSU ) {
        rivi_.asetaTyyppi(TilioteKirjausRivi::SUORITUS);
        QModelIndex index = ui->maksuView->currentIndex();
        const QString& selite = index.data(LaskuTauluModel::SeliteRooli).toString();

        rivi_.asetaOtsikko( selite );
        rivi_.asetaKumppani( index.data(LaskuTauluModel::KumppaniMapRooli).toMap() );

        TilioteAliRivi aliRivi;
        aliRivi.setTili( index.data(LaskuTauluModel::TiliRooli).toInt() );
        aliRivi.setEra(index.data(LaskuTauluModel::EraMapRooli).toMap());
        aliRivi.setBrutto( menoa_ ? ui->euroEdit->euro() : Euro::Zero - ui->euroEdit->euro() );
        rivi_.asetaRivi(aliRivi);

    } else if( ui->alaTabs->currentIndex() == SIIRTO ) {
        rivi_.asetaTyyppi(TilioteKirjausRivi::SIIRTO);

        TilioteAliRivi aliRivi;
        aliRivi.setTili(ui->tiliEdit->valittuTilinumero());
        aliRivi.setEra( ui->eraCombo->eraMap());
        aliRivi.setMerkkaukset( ui->merkkausCC->selectedDatas() );
        aliRivi.setBrutto( menoa_ ? ui->euroEdit->euro() : Euro::Zero - ui->euroEdit->euro() );

        rivi_.asetaRivi(aliRivi);

    } else if( ui->alaTabs->currentIndex() == VAKIOVIITE) {
        rivi_.asetaTyyppi( TilioteKirjausRivi::MYYNTI);


        QModelIndex index = ui->maksuView->currentIndex();
        const QVariantMap& map = index.data(VakioViiteModel::MapRooli).toMap();

        rivi_.asetaOtsikko(map.value("otsikko").toString());
        rivi_.asetaKumppani( ui->asiakastoimittaja->map() );
        rivi_.asetaViite( map.value("viite").toString() );

        TilioteAliRivi aliRivi;
        aliRivi.setTili( map.value("tili").toInt());
        aliRivi.setKohdennus( map.value("kohdennus").toInt());
        aliRivi.setBrutto( ui->euroEdit->euro() );

        rivi_.asetaRivi(aliRivi);

    } else {
        tallennaRivi();

    }


}

void TilioteKirjaaja::paivitaVeroFiltteri(const int verokoodi)
{
    if( menoa_) {
        veroFiltteri_->setFilterRegularExpression( QRegularExpression( "^(0|2[1-69]|927|" + QString::number(verokoodi) + ")" ) );
    } else {
        veroFiltteri_->setFilterRegularExpression( QRegularExpression( "^(0|1[1-79]|" + QString::number(verokoodi) + ")" ) );
    }
    ui->alvCombo->setCurrentIndex( ui->alvCombo->findData(verokoodi, VerotyyppiModel::KoodiRooli) );
}

TilioteAliRivi *TilioteKirjaaja::aliRivi()
{
    if( nykyAliRiviIndeksi_ < 0 || nykyAliRiviIndeksi_ >= aliRiviModel_->rowCount()) {
        nykyAliRiviIndeksi_ = 0;
    }
    return rivi_.pRivi(nykyAliRiviIndeksi_);
}

void TilioteKirjaaja::aliRiviaMuokattu()
{
    emit aliRiviModel_->dataChanged(
        aliRiviModel_->index(nykyAliRiviIndeksi_, 0),
        aliRiviModel_->index(nykyAliRiviIndeksi_, 2));

    tarkastaTallennus();
}







