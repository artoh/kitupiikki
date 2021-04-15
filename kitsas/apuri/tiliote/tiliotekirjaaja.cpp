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
    viennit_( new TilioteViennit(kp(), this))
{
    alusta();
    ui->pvmEdit->setDate( apuri->tosite()->data(Tosite::PVM).toDate() );

}

TilioteKirjaaja::TilioteKirjaaja(SiirtoApuri *apuri):
    QDialog(apuri),
    ui(new Ui::TilioteKirjaaja),
    maksuProxy_(new QSortFilterProxyModel(this)),
    avoinProxy_( new QSortFilterProxyModel(this)),
    laskut_( new LaskuTauluModel(this)),
    viennit_( new TilioteViennit(kp(), this))
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
            QDialog::accept();
        } else {
            tallennaRivi();
            if( riviIndeksi_ > -1) {
                apuri()->model()->asetaRivi(riviIndeksi_, tallennettava());
                QDialog::accept();
            } else {
                apuri()->model()->lisaaRivi(tallennettava());
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
}

void TilioteKirjaaja::muokkaaRivia(int riviNro)
{
    setWindowTitle(tr("Muokkaa tiliotekirjausta"));

    riviIndeksi_ = riviNro;
    lataa(apuri()->model()->rivi(riviNro));
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


    tiliMuuttuu();
    paivitaVientiNakyma();

    ui->viennitView->horizontalHeader()->setSectionResizeMode(TilioteViennit::TILI, QHeaderView::Stretch);

}

void TilioteKirjaaja::euroMuuttuu()
{   
   ui->ylaTab->setCurrentIndex( ui->euroEdit->miinus() ? 1 : 0 );
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


    ui->euroEdit->setMiinus( tab );
}

void TilioteKirjaaja::tiliMuuttuu()
{
    Tili tili = ui->tiliEdit->valittuTili();

    bool erat = tili.eritellaankoTase() &&
            ui->alaTabs->currentIndex() != MAKSU;
    ui->eraLabel->setVisible(erat);
    ui->eraCombo->setVisible(erat);
    if( erat ) {
        ui->eraCombo->asetaTili(tili.numero());
    }

    bool jakso = tili.onko(TiliLaji::TULOS) &&
            (ui->alaTabs->currentIndex() == TULOMENO || ui->alaTabs->currentIndex() == HYVITYS);

    ui->jaksotusLabel->setVisible(jakso);
    ui->jaksoAlkaaEdit->setVisible(jakso);
    ui->jaksoViivaLabel->setVisible(jakso);
    ui->jaksoLoppuuEdit->setVisible(jakso);

    if(tili.luku("kohdennus"))
        ui->kohdennusCombo->valitseKohdennus(tili.luku("kohdennus"));

    paivitaAlvInfo();
}

void TilioteKirjaaja::paivitaAlvInfo()
{
    Tili tili = ui->tiliEdit->valittuTili();
    int alvlaji = tili.luku("alvlaji");
    bool vero = (ui->alaTabs->currentIndex() == TULOMENO || ui->alaTabs->currentIndex() == HYVITYS) &&
            ( alvlaji == AlvKoodi::MYYNNIT_NETTO || alvlaji == AlvKoodi::MYYNNIT_BRUTTO ||
              alvlaji == AlvKoodi::OSTOT_NETTO || alvlaji == AlvKoodi::OSTOT_BRUTTO);

    ui->alvLabel->setVisible(vero);
    ui->alvCombo->setVisible(vero);
    ui->alvVaro->setVisible(vero);
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

    if( index.isValid() && ui->alaTabs->currentIndex() == MAKSU) {
        double avoinna = index.data(LaskuTauluModel::AvoinnaRooli).toDouble();
        ui->euroEdit->setValue( menoa_ ? 0 - avoinna : avoinna  );
    }
}

void TilioteKirjaaja::suodata(const QString &teksti)
{
    if( teksti.startsWith("RF") || teksti.contains(  QRegularExpression("^\\d+$")))
    {
        maksuProxy_->setFilterKeyColumn( LaskuTauluModel::NUMERO );
        maksuProxy_->setFilterRegExp("^" + teksti);
    } else {
        maksuProxy_->setFilterKeyColumn( LaskuTauluModel::ASIAKASTOIMITTAJA);
        maksuProxy_->setFilterCaseSensitivity(Qt::CaseInsensitive);
        maksuProxy_->setFilterFixedString( teksti );
    }
    if( ui->maksuView->model()->rowCount())
        ui->alaTabs->setCurrentIndex(MAKSU);
}

void TilioteKirjaaja::tyhjenna()
{       
    ui->asiakastoimittaja->clear();
    ui->maksuView->clearSelection();
    ui->pvmEdit->setFocus();
    ui->merkkausCC->haeMerkkaukset( pankkiVienti_.pvm()  );
    ui->euroEdit->clear();

    viennit_->tyhjenna();
    viennit_->lisaaVienti(TositeVienti());

    lataaNakymaan();
    nykyVientiRivi_ = 0;
    ui->viennitView->selectRow(0);

    tarkastaTallennus();
    avoinProxy_->setFilterFixedString("€");

}

void TilioteKirjaaja::tarkastaTallennus()
{
    ui->okNappi->setEnabled(
                qAbs(ui->euroEdit->value()) > 1e-5 &&
                ( ui->tiliEdit->valittuTilinumero() ||
                  !ui->maksuView->selectionModel()->selectedRows().isEmpty() ));
}



void TilioteKirjaaja::kumppaniValittu(int kumppaniId)
{
    if( ui->alaTabs->currentIndex() == TULOMENO && kumppaniId > 0 ) {
        KpKysely *kysely = kpk(QString("/kumppanit/%1").arg(kumppaniId));
        connect(kysely, &KpKysely::vastaus, this, &TilioteKirjaaja::kumppaniTiedot);
        kysely->kysy();
    }
}

void TilioteKirjaaja::kumppaniTiedot(QVariant *data)
{
    QVariantMap map = data->toMap();
    if( menoa_ ) {
        if( map.contains("menotili"))
            ui->tiliEdit->valitseTiliNumerolla( map.value("menotili").toInt() );
    } else {
        if( map.contains("tulotili"))
            ui->tiliEdit->valitseTiliNumerolla( map.value("tulotili").toInt());
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
    uusi.setTili(menoa_ ? kp()->asetukset()->luku("OletusMenotili") : kp()->asetukset()->luku("OletusMyyntitili")  );
    uusi.setSelite( pankkiVienti_.selite() );
    viennit_->lisaaVienti(uusi);
    ui->viennitView->selectRow(viennit_->rowCount()-1);
}

void TilioteKirjaaja::poistaVienti()
{
    nykyVientiRivi_ = -1;
    const int rivi = ui->viennitView->currentIndex().row();
    if( rivi >= 0) {
        viennit_->poistaVienti(rivi);
        ui->viennitView->selectRow(0);
    }
}

void TilioteKirjaaja::paivitaVientiNakyma()
{
    int alatabu = ui->alaTabs->currentIndex();
    ui->lisaaVientiNappi->setVisible(alatabu == TULOMENO);
    ui->poistaVientiNappi->setVisible( viennit_->rowCount() > 1 && alatabu == TULOMENO);
    ui->viennitView->setVisible(viennit_->rowCount() > 1 && alatabu == TULOMENO);
}

void TilioteKirjaaja::alusta()
{
    ui->setupUi(this);
    ui->viennitView->setModel(viennit_);

    ui->ylaTab->addTab(QIcon(":/pic/lisaa.png"), tr("Tilille"));
    ui->ylaTab->addTab(QIcon(":/pic/poista.png"), tr("Tililtä"));

    ui->alaTabs->addTab(QIcon(":/pic/lasku.png"), tr("Laskun maksu"));
    ui->alaTabs->addTab(QIcon(":/pic/lisaa.png"), tr("Tulo"));
    ui->alaTabs->addTab(QIcon(":/pic/edit-undo.png"), tr("Hyvitys"));
    ui->alaTabs->addTab(QIcon(":/pic/siirra.png"), tr("Siirto"));


    alaTabMuuttui(0);

    connect( ui->euroEdit, &KpEuroEdit::textChanged, this, &TilioteKirjaaja::euroMuuttuu);
    connect( ui->alaTabs, &QTabBar::currentChanged, this, &TilioteKirjaaja::alaTabMuuttui);
    connect( ui->ylaTab, &QTabBar::currentChanged, this, &TilioteKirjaaja::ylaTabMuuttui);
    connect( ui->alvCombo, &QComboBox::currentTextChanged, this, &TilioteKirjaaja::paivitaAlvInfo);

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

    connect( ui->euroEdit, &KpEuroEdit::sntMuuttui, this, &TilioteKirjaaja::tarkastaTallennus);
    connect( ui->pvmEdit, &KpDateEdit::dateChanged, this, &TilioteKirjaaja::tarkastaTallennus);

    connect( ui->tiliEdit, &TilinvalintaLine::textChanged, this, &TilioteKirjaaja::tarkastaTallennus);
    connect( ui->tiliEdit, &TilinvalintaLine::textChanged, this, &TilioteKirjaaja::tiliMuuttuu);
    connect( ui->maksuView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &TilioteKirjaaja::tarkastaTallennus);
    connect( ui->eraCombo, &EraCombo::valittu, this, &TilioteKirjaaja::eraValittu);
    connect( ui->jaksoAlkaaEdit, &KpDateEdit::dateChanged, this, &TilioteKirjaaja::jaksomuuttuu);


    connect( ui->asiakastoimittaja, &AsiakasToimittajaValinta::valittu, this, &TilioteKirjaaja::kumppaniValittu);
    connect( ui->ohjeNappi, &QPushButton::clicked, [] { kp()->ohje("kirjaus/tiliote"); });
    connect( ui->tyhjaaNappi, &QPushButton::clicked, this, &TilioteKirjaaja::tyhjenna);
    connect( laskut_, &LaskuTauluTilioteProxylla::modelReset, [this] { this->suodata(this->ui->suodatusEdit->text()); ui->maksuView->resizeColumnToContents(LaskuTauluModel::ASIAKASTOIMITTAJA); });

    // connect(ui->viennitView, &QTableView::clicked, this, &TilioteKirjaaja::tallennaRivi);
    connect(ui->euroEdit, &KpEuroEdit::textChanged, this, &TilioteKirjaaja::tallennaRivi);
    connect(ui->tiliEdit, &TilinvalintaLine::textChanged, this, &TilioteKirjaaja::tallennaRivi);

    connect( ui->viennitView->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &TilioteKirjaaja::riviVaihtuu);

    connect( ui->lisaaVientiNappi, &QPushButton::clicked, this, &TilioteKirjaaja::lisaaVienti);
    connect( ui->poistaVientiNappi, &QPushButton::clicked, this, &TilioteKirjaaja::poistaVienti);
    connect( viennit_, &TilioteViennit::modelReset, this, &TilioteKirjaaja::paivitaVientiNakyma);
    connect( viennit_, &TilioteViennit::rowsInserted, this, &TilioteKirjaaja::paivitaVientiNakyma);
    connect( viennit_, &TilioteViennit::rowsRemoved, this, &TilioteKirjaaja::paivitaVientiNakyma);

    ui->alvCombo->addItem(QIcon(":/pic/tyhja.png"), tr("Veroton"), 0);
    ui->alvCombo->addItem(QIcon(":/pic/lihavoi.png"), QString("24 %"), 24.0);
    ui->alvCombo->addItem(QIcon(":/pic/lihavoi.png"), QString("14 %"), 14.0);
    ui->alvCombo->addItem(QIcon(":/pic/lihavoi.png"), QString("10 %"), 10.0);

    ylaTabMuuttui( ui->ylaTab->currentIndex() );
}

TilioteApuri *TilioteKirjaaja::apuri() const
{
    return qobject_cast<TilioteApuri*>( parent() );
}

void TilioteKirjaaja::lataa(const TilioteKirjausRivi &rivi)
{
    const QList<TositeVienti>& viennit = rivi.viennit();

    pankkiVienti_ = viennit.value(0);
    viennit_->tyhjenna();
    for(int i=1; i < viennit.count(); i++)
        viennit_->lisaaVienti(viennit.at(i));

    lataaNakymaan();
    ui->viennitView->selectRow(0);
    nykyVientiRivi_ = 0;
    naytaRivi();
}


void TilioteKirjaaja::lataaNakymaan()
{
    const TositeVienti& tapahtuma = viennit_->vienti(0);
    const int tili = tapahtuma.tili();

    ui->pvmEdit->setDate(pankkiVienti_.pvm());
    ui->asiakastoimittaja->set( pankkiVienti_.kumppaniId(), pankkiVienti_.kumppaniNimi() );

    QString saajamaksaja = pankkiVienti_.kumppaniNimi();
    int valinpaikka = saajamaksaja.indexOf(QRegularExpression("\\W",QRegularExpression::UseUnicodePropertiesOption));
    if( valinpaikka > 2)
        saajamaksaja = saajamaksaja.left(valinpaikka);

    double euro = pankkiVienti_.debet() - pankkiVienti_.kredit();
    ui->ylaTab->setCurrentIndex( euro < 0 );

    if( tapahtuma.tyyppi() == TositeVienti::SIIRTO + TositeVienti::KIRJAUS)
        ui->alaTabs->setCurrentIndex( SIIRTO );
    else if( tapahtuma.eraId() )
        ui->alaTabs->setCurrentIndex( MAKSU );
    else if( QString::number(tili).startsWith('1') ||
             QString::number(tili).startsWith('2'))
        ui->alaTabs->setCurrentIndex( SIIRTO );
    else {
        if( (tapahtuma.tyyppi() == TositeVienti::OSTO + TositeVienti::KIRJAUS && euro > 0) ||
            (tapahtuma.tyyppi() == TositeVienti::MYYNTI + TositeVienti::KIRJAUS && euro < 0)) {
            ui->alaTabs->setCurrentIndex(HYVITYS);
        } else {
            ui->alaTabs->setCurrentIndex(TULOMENO );
        }
    }

    // Etsitään valittava rivi
    avoinProxy_->setFilterFixedString("");

    bool maksu = false;
    if( tapahtuma.eraId()) {
        for(int i=0; i < avoinProxy_->rowCount(); i++) {
            if( avoinProxy_->data( avoinProxy_->index(i,0), LaskuTauluModel::EraIdRooli ).toInt() == tapahtuma.eraId()) {
                ui->maksuView->selectRow(i);
                maksu = true;
                break;
            }
        }
    }
    if( tapahtuma.eraId() && !maksu) {
        ui->alaTabs->setCurrentIndex( SIIRTO );
    }

    ui->suodatusEdit->setText( saajamaksaja );
    if( !saajamaksaja.isEmpty())
        suodata(saajamaksaja);
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
    nykyVientiRivi_ = -1;

    const TositeVienti& vienti = viennit_->vienti(rivi);
    double euro = vienti.kredit() - vienti.debet();
    ui->euroEdit->setValue(euro);

    if(vienti.tili())
        ui->tiliEdit->valitseTiliNumerolla( vienti.tili() );

    ui->eraCombo->valitse( vienti.era());
    ui->kohdennusCombo->valitseKohdennus( vienti.kohdennus() );
    ui->merkkausCC->setSelectedItems( vienti.merkkaukset() );
    ui->seliteEdit->setText( vienti.selite() );

    if(vienti.jaksoalkaa().isValid())
        ui->jaksoAlkaaEdit->setDate( vienti.jaksoalkaa() );
    else
        ui->jaksoAlkaaEdit->setNull();

    if(vienti.jaksoloppuu().isValid())
        ui->jaksoLoppuuEdit->setDate( vienti.jaksoloppuu() );
    else
        ui->jaksoLoppuuEdit->setNull();

    ui->alvCombo->setCurrentIndex(ui->alvCombo->findData(vienti.alvProsentti()));

    nykyVientiRivi_ = rivi;
}


void TilioteKirjaaja::tallennaRivi()
{
    const int rivi = nykyVientiRivi_;
    if( rivi < 0)
        return;

    TositeVienti vienti;
    vienti.setTili( ui->tiliEdit->valittuTilinumero());
    vienti.setSelite( ui->seliteEdit->toPlainText());
    vienti.setKohdennus( ui->kohdennusCombo->kohdennus());
    vienti.setMerkkaukset( ui->merkkausCC->selectedDatas());
    vienti.setEra( ui->eraCombo->eraMap());
    vienti.setJaksoalkaa( ui->jaksoAlkaaEdit->date() );
    vienti.setJaksoloppuu( ui->jaksoLoppuuEdit->date() );
    vienti.setKredit( ui->euroEdit->value() );

    Tili tili = ui->tiliEdit->valittuTili();
    int alvlaji = tili.luku("alvlaji");
    double alvprosentti = ui->alvCombo->isVisible() ? ui->alvCombo->currentData().toDouble() : 0.0;
    if( alvprosentti > 1e-5) {
        vienti.setAlvKoodi( alvlaji == AlvKoodi::OSTOT_BRUTTO || alvlaji == AlvKoodi::OSTOT_NETTO ? AlvKoodi::OSTOT_BRUTTO : AlvKoodi::MYYNNIT_BRUTTO);
        vienti.setAlvProsentti(alvprosentti);
    }

    viennit_->asetaVienti(rivi, vienti);
}

QList<TositeVienti> TilioteKirjaaja::viennit() const
{
    QList<TositeVienti> vientiLista;

    TositeVienti pankki(pankkiVienti_);
    pankki.setPvm( ui->pvmEdit->date() );

    if( !apuri() ) // Kun tätä käytetään siirtoapurissa, saadaan pankkitili tästä
        pankki.setTili( ui->tiliEdit->valittuTilinumero() );

    if ( ui->alaTabs->currentIndex() == MAKSU ) {
        TositeVienti suoritus;
        suoritus.setPvm( pankki.pvm() );
        pankki.setTyyppi( TositeVienti::SUORITUS + TositeVienti::VASTAKIRJAUS );
        suoritus.setTyyppi( TositeVienti::SUORITUS + TositeVienti::KIRJAUS);

        QModelIndex index = ui->maksuView->currentIndex();
        const QString& selite = index.data(LaskuTauluModel::SeliteRooli).toString();
        pankki.setSelite(selite);
        suoritus.setSelite(selite);

        QVariantMap kumppani;
        kumppani.insert("nimi",index.data(LaskuTauluModel::AsiakasToimittajaNimiRooli).toString());
        kumppani.insert("id",index.data(LaskuTauluModel::AsiakasToimittajaIdRooli).toInt());
        pankki.setKumppani(kumppani);
        suoritus.setKumppani(kumppani);

        suoritus.setEra(index.data(LaskuTauluModel::EraMapRooli).toMap());
        suoritus.setTili( index.data(LaskuTauluModel::TiliRooli).toInt());

        pankki.setDebet( ui->euroEdit->euro() );
        suoritus.setKredit( ui->euroEdit->euro());

        vientiLista << pankki << suoritus;
    } else if( ui->alaTabs->currentIndex() == SIIRTO ) {
        TositeVienti siirto;
        siirto.setPvm(pankki.pvm());
        pankki.setTyyppi(TositeVienti::SIIRTO + TositeVienti::VASTAKIRJAUS);
        siirto.setTyyppi(TositeVienti::SIIRTO + TositeVienti::KIRJAUS);

        pankki.setSelite( ui->seliteEdit->toPlainText());
        siirto.setSelite( ui->seliteEdit->toPlainText());
        pankki.setKumppani( ui->asiakastoimittaja->map());
        siirto.setKumppani( ui->asiakastoimittaja->map());

        siirto.setEra( ui->eraCombo->eraMap());
        siirto.setMerkkaukset( ui->merkkausCC->selectedDatas());

        pankki.setDebet( ui->euroEdit->euro() );
        siirto.setKredit( ui->euroEdit->euro());

        vientiLista << pankki << siirto;
    } else if( ui->alaTabs->currentIndex() == VAKIOVIITE) {
        TositeVienti maksu;
        maksu.setPvm( pankki.pvm());
        pankki.setTyyppi( TositeVienti::MYYNTI + TositeVienti::VASTAKIRJAUS);
        maksu.setTyyppi( TositeVienti::MYYNTI + TositeVienti::KIRJAUS );

        QModelIndex index = ui->maksuView->currentIndex();
        const QVariantMap& map = index.data(VakioViiteModel::MapRooli).toMap();
        pankki.setSelite(map.value("otsikko").toString());
        maksu.setSelite(map.value("otsikko").toString());
        pankki.setKumppani( ui->asiakastoimittaja->map() );
        maksu.setTili( map.value("tili").toInt());
        maksu.setKohdennus( map.value("kohdennus").toInt());
        maksu.setKumppani( ui->asiakastoimittaja->map() );
        pankki.setViite( map.value("viite").toString() );

        pankki.setDebet( ui->euroEdit->euro() );
        maksu.setKredit( ui->euroEdit->euro());

        vientiLista << pankki << maksu;

    } else {
        int tyyppi = ui->ylaTab->currentIndex() ^ (ui->alaTabs->currentIndex() == HYVITYS) ? TositeVienti::OSTO : TositeVienti::MYYNTI;
        pankki.setTyyppi( tyyppi + TositeVienti::VASTAKIRJAUS );
        pankki.setSelite( viennit_->vienti(0).selite() );
        pankki.setKumppani( ui->asiakastoimittaja->map());
        pankki.setDebet( viennit_->summa() );
        vientiLista << pankki;
        for(const auto& vienti : viennit_->viennit()) {
            TositeVienti tapahtuma(vienti);
            tapahtuma.setPvm(pankki.pvm());
            tapahtuma.setTyyppi(tyyppi + TositeVienti::KIRJAUS);
            if(tapahtuma.debet() > 1e-5 || tapahtuma.kredit() > 1e-5)
                vientiLista << tapahtuma;
        }
    }
    return vientiLista;
}

TilioteKirjausRivi TilioteKirjaaja::tallennettava() const
{
    QList<TositeVienti> vientiLista = viennit();

    TilioteKirjausRivi kirjaus =
            riviIndeksi_ < 0
            ? TilioteKirjausRivi(  vientiLista.value(0).pvm(), apuri()->model() )
            : apuri()->model()->rivi(riviIndeksi_);

    kirjaus.asetaViennit(vientiLista);
    return kirjaus;
}



