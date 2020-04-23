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

#include <QPushButton>
#include <QSortFilterProxyModel>
#include "tilioteapuri.h"
#include "model/tosite.h"

#include <QShortcut>

TilioteKirjaaja::TilioteKirjaaja(TilioteApuri *apuri) :
    QDialog(apuri),
    ui(new Ui::TilioteKirjaaja),
    maksuProxy_(new QSortFilterProxyModel(this)),
    avoinProxy_( new QSortFilterProxyModel(this)),
    laskut_( new LaskuTauluTilioteProxylla(this, apuri->model()))
{
    ui->setupUi(this);

    ui->ylaTab->addTab(QIcon(":/pic/lisaa.png"), tr("Tilille"));
    ui->ylaTab->addTab(QIcon(":/pic/poista.png"), tr("Tililtä"));

    ui->alaTabs->addTab(QIcon(":/pic/lasku.png"), tr("Laskun maksu"));
    ui->alaTabs->addTab(QIcon(":/pic/lisaa.png"), tr("Tulo"));
    ui->alaTabs->addTab(QIcon(":/pic/siirra.png"), tr("Siirto"));


    alaTabMuuttui(0);

    connect( ui->euroEdit, &KpEuroEdit::textChanged, this, &TilioteKirjaaja::euroMuuttuu);
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

    ui->pvmEdit->setDate( apuri->tosite()->data(Tosite::PVM).toDate() );
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
    connect( laskut_, &LaskuTauluTilioteProxylla::modelReset, [this] { this->suodata(this->ui->suodatusEdit->text()); });
}

TilioteKirjaaja::~TilioteKirjaaja()
{
    delete ui;
}

void TilioteKirjaaja::asetaPvm(const QDate &pvm)
{
    ui->pvmEdit->setDate(pvm);
}

TilioteModel::Tilioterivi TilioteKirjaaja::rivi()
{
    TilioteModel::Tilioterivi rivi;
    rivi.pvm = ui->pvmEdit->date();
    rivi.euro = ui->euroEdit->value();

    rivi.selite = ui->seliteEdit->text();
    rivi.tili = ui->tiliEdit->valittuTilinumero();
    rivi.kohdennus = ui->kohdennusCombo->kohdennus();
    rivi.merkkaukset = ui->merkkausCC->selectedDatas();


    if( ui->alaTabs->currentIndex() == MAKSU) {
        QModelIndex index = ui->maksuView->currentIndex();

        rivi.saajamaksaja = index.data(LaskuTauluModel::AsiakasToimittajaNimiRooli).toString();
        rivi.saajamaksajaId = index.data(LaskuTauluModel::AsiakasToimittajaIdRooli).toInt();


        rivi.era = index.data(LaskuTauluModel::EraMapRooli).toMap();
        rivi.laskupvm = index.data(LaskuTauluModel::LaskuPvmRooli).toDate();
        rivi.tili = index.data(LaskuTauluModel::TiliRooli).toInt();
        if( rivi.selite.isEmpty())
            rivi.selite = index.data(LaskuTauluModel::AsiakasToimittajaNimiRooli ).toString();
        if( rivi.selite.isEmpty())
            rivi.selite = index.data(LaskuTauluModel::SeliteRooli).toString();

    } else {
        rivi.saajamaksajaId = ui->asiakastoimittaja->id();
        rivi.saajamaksaja = ui->asiakastoimittaja->nimi();
        rivi.era = ui->eraCombo->eraMap();
        rivi.laskupvm = ui->eraCombo->eraMap().value("pvm").toDate();

        if( rivi.selite.isEmpty())
            rivi.selite = rivi.saajamaksaja;
        rivi.jaksoalkaa = ui->jaksoAlkaaEdit->date();
        rivi.jaksoloppuu = ui->jaksoLoppuuEdit->date();
    }
    rivi.alkuperaisetViennit = alkuperaisRivit_;

    return rivi;
}

void TilioteKirjaaja::accept()
{
    if( ui->okNappi->isEnabled() ) {        
        if( muokattavaRivi_ > -1) {
            apuri()->model()->muokkaaRivi( muokattavaRivi_, rivi());
            QDialog::accept();
        } else {
            apuri()->model()->lisaaRivi( rivi());
        }

        tyhjenna();
    }
}

void TilioteKirjaaja::kirjaaUusia()
{
    setWindowTitle( tr("Kirjaa tiliotteelle"));
    muokattavaRivi_ = -1;
    show();
}

void TilioteKirjaaja::muokkaaRivia(int riviNro)
{
    setWindowTitle(tr("Muokkaa tiliotekirjausta"));

    muokattavaRivi_ = riviNro;
    TilioteModel::Tilioterivi rivi = apuri()->model()->rivi(riviNro);

    QString saajamaksaja = rivi.saajamaksaja;
    int valinpaikka = saajamaksaja.indexOf(QRegularExpression("\\s"));
    if( valinpaikka > 2)
        saajamaksaja = saajamaksaja.left(valinpaikka);

    ui->ylaTab->setCurrentIndex( rivi.euro < 0 );

    if( rivi.era.value("id").toInt() )
        ui->alaTabs->setCurrentIndex( MAKSU );
    else if( QString::number(rivi.tili).startsWith('1') ||
             QString::number(rivi.tili).startsWith('2'))
        ui->alaTabs->setCurrentIndex( SIIRTO );
    else
        ui->alaTabs->setCurrentIndex(TULOMENO );

    ui->kohdennusCombo->valitseKohdennus( rivi.kohdennus );
    ui->tiliEdit->valitseTiliNumerolla( rivi.tili );

    // Etsitään valittava rivi
    avoinProxy_->setFilterFixedString("");

    bool maksu = false;
    for(int i=0; i < avoinProxy_->rowCount(); i++) {
        if( avoinProxy_->data( avoinProxy_->index(i,0), LaskuTauluModel::EraIdRooli ).toInt() == rivi.era.value("id").toInt()) {
            ui->maksuView->selectRow(i);
            maksu = true;
            break;
        }        
    }
    if( rivi.era.value("id").toInt() && !maksu) {
        ui->alaTabs->setCurrentIndex( SIIRTO );
    }

    ui->eraCombo->valitse( rivi.era.value("id").toInt() );

    // MerkkausCC
    ui->merkkausCC->haeMerkkaukset( rivi.pvm );
    ui->merkkausCC->setSelectedItems( rivi.merkkaukset );

    ui->asiakastoimittaja->set( rivi.saajamaksajaId,
                                rivi.saajamaksaja);

    ui->pvmEdit->setDate( rivi.pvm );
    ui->euroEdit->setValue( rivi.euro );
    ui->seliteEdit->setText( rivi.selite );

    ui->jaksoAlkaaEdit->setDate( rivi.jaksoalkaa);
    ui->jaksoLoppuuEdit->setDate( rivi.jaksoloppuu);

    ui->suodatusEdit->setText( saajamaksaja );
    suodata(saajamaksaja);
}


void TilioteKirjaaja::alaTabMuuttui(int tab)
{

    ui->suodatusEdit->setVisible( tab == MAKSU  );
    ui->maksuView->setVisible( tab == MAKSU );

    ui->tiliLabel->setVisible( tab != MAKSU && tab != PIILOSSA);
    ui->tiliEdit->setVisible( tab != MAKSU && tab != PIILOSSA);

    ui->kohdennusLabel->setVisible( tab == TULOMENO && kp()->kohdennukset()->kohdennuksia() );
    ui->kohdennusCombo->setVisible( tab == TULOMENO&& kp()->kohdennukset()->kohdennuksia() );    

    ui->merkkausLabel->setVisible(  tab == TULOMENO && kp()->kohdennukset()->merkkauksia() );
    ui->merkkausCC->setVisible(  tab == TULOMENO && kp()->kohdennukset()->merkkauksia() );

    ui->asiakasLabel->setVisible( tab == TULOMENO || tab==SIIRTO);
    ui->asiakastoimittaja->setVisible( tab == TULOMENO || tab==SIIRTO);

    ui->seliteLabel->setVisible(tab != MAKSU);
    ui->seliteEdit->setVisible( tab != MAKSU);

    if( tab == MAKSU ) {
        laskut_->lataaAvoimet( menoa_ );

    } else if( tab == TULOMENO ) {
        ui->tiliLabel->setText( menoa_ ? tr("Menotili") : tr("Tulotili"));
        ui->asiakasLabel->setText( menoa_ ? tr("Toimittaja") : tr("Asiakas"));
        ui->tiliEdit->suodataTyypilla( menoa_ ? "D.*" : "C.*");
        ui->asiakastoimittaja->alusta();
        ui->tiliEdit->valitseTiliNumerolla(  menoa_ ? kp()->asetukset()->luku("OletusMenotili") : kp()->asetukset()->luku("OletusMyyntitili") );    // TODO: Tod. oletukset
    } else if ( tab == SIIRTO ) {
        ui->tiliLabel->setText( menoa_ ? tr("Tilille") : tr("Tililtä")  );
        ui->asiakasLabel->setText( menoa_ ? tr("Saaja") : tr("Maksaja"));
        ui->tiliEdit->suodataTyypilla( "[AB].*");

    }
    tiliMuuttuu();

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
    } else {
        ui->alaTabs->setTabText(MAKSU, tr("Saapuva maksu"));
        ui->alaTabs->setTabIcon(TULOMENO, QIcon(":/pic/lisaa.png") ) ;
        ui->alaTabs->setTabText(TULOMENO, tr("Tulo"));
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
        ui->eraCombo->lataa(tili.numero());
    }

    bool jakso = tili.onko(TiliLaji::TULOS) &&
            ui->alaTabs->currentIndex() == TULOMENO;
    ui->jaksotusLabel->setVisible(jakso);
    ui->jaksoAlkaaEdit->setVisible(jakso);
    ui->jaksoViivaLabel->setVisible(jakso);
    ui->jaksoLoppuuEdit->setVisible(jakso);

    bool vero = tili.luku("alvlaji") && ui->alaTabs->currentIndex() == TULOMENO;
    ui->alvVaro->setVisible(vero);
    if( menoa_ )
        ui->alvVaro->setText(tr("Tällä toiminnolla voit tehdä vain verottomia kirjauksia.\n"
                                "Kirjaa verolliset tulot tositetyypillä Tulo"));
    else
        ui->alvVaro->setText(tr("Tällä toiminnolla voit tehdä vain verottomia kirjauksia.\n"
                                "Kirjaa verolliset menot tositetyypillä Meno"));
}

void TilioteKirjaaja::eraValittu(int eraId, double avoinna, const QString &selite)
{
    if( !ui->euroEdit->asCents() && avoinna > 1e-5)
        ui->euroEdit->setValue(menoa_ ? 0 - avoinna : avoinna);
    if( ui->seliteEdit->text().isEmpty())
        ui->seliteEdit->setText(selite);
    haeAlkuperaisTosite(eraId);

}

void TilioteKirjaaja::jaksomuuttuu(const QDate &pvm)
{
    ui->jaksoLoppuuEdit->setEnabled( pvm.isValid() );
    ui->jaksoLoppuuEdit->setDateRange( pvm, kp()->tilikaudet()->kirjanpitoLoppuu() );
    if( !pvm.isValid())
        ui->jaksoLoppuuEdit->setNull();
}

void TilioteKirjaaja::valitseLasku()
{
    QModelIndex index = ui->maksuView->currentIndex();

    if( index.isValid()) {
        double avoinna = index.data(LaskuTauluModel::AvoinnaRooli).toDouble();
        ui->euroEdit->setValue( menoa_ ? 0 - avoinna : avoinna  );

        haeAlkuperaisTosite( index.data(LaskuTauluModel::EraIdRooli).toInt() );
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
    ui->euroEdit->clear();
    ui->merkkausCC->clear();
    ui->seliteEdit->clear();
    ui->maksuView->clearSelection();
    ui->pvmEdit->setFocus();
    ui->kohdennusCombo->setCurrentIndex(
                ui->kohdennusCombo->findData(0, KohdennusModel::IdRooli));
    ui->eraCombo->valitse(0);
    ui->jaksoAlkaaEdit->setNull();
    ui->jaksoLoppuuEdit->setNull();

    ui->merkkausCC->haeMerkkaukset();

    tarkastaTallennus();
    avoinProxy_->setFilterFixedString("€");
    alkuperaisRivit_.clear();
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
    if( ui->alaTabs->currentIndex() == TULOMENO ) {
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

TilioteApuri *TilioteKirjaaja::apuri()
{
    return qobject_cast<TilioteApuri*>( parent() );
}
