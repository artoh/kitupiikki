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

#include "kirjaus/kohdennusproxymodel.h"
#include "laskutaulutilioteproxylla.h"

#include <QPushButton>
#include <QSortFilterProxyModel>
#include "tilioteapuri.h"
#include "model/tosite.h"

#include <QShortcut>

TilioteKirjaaja::TilioteKirjaaja(TilioteApuri *apuri) :
    QDialog(apuri),
    ui(new Ui::TilioteKirjaaja),
    kohdennusProxy_(new KohdennusProxyModel(this) ),
    maksuProxy_(new QSortFilterProxyModel(this)),
    laskut_( new LaskuTauluTilioteProxylla(this, apuri->model()))
{
    ui->setupUi(this);

    ui->ylaTab->addTab(QIcon(":/pic/lisaa.png"), tr("Tilille"));
    ui->ylaTab->addTab(QIcon(":/pic/poista.png"), tr("Tililtä"));

    ui->alaTabs->addTab(QIcon(":/pic/lasku.png"), tr("Laskun maksu"));
    ui->alaTabs->addTab(QIcon(":/pic/lisaa.png"), tr("Tulo"));
    ui->alaTabs->addTab(QIcon(":/pic/siirra.png"), tr("Siirto"));

    ui->kohdennusCombo->setModel(kohdennusProxy_);
    ui->kohdennusCombo->setModelColumn(KohdennusModel::NIMI);

    alaTabMuuttui(0);

    connect( ui->euroEdit, &KpEuroEdit::textChanged, this, &TilioteKirjaaja::euroMuuttuu);
    connect( ui->alaTabs, &QTabBar::currentChanged, this, &TilioteKirjaaja::alaTabMuuttui);
    connect( ui->ylaTab, &QTabBar::currentChanged, this, &TilioteKirjaaja::ylaTabMuuttui);    

    maksuProxy_->setSourceModel( laskut_ );

    QSortFilterProxyModel* avoinProxy = new QSortFilterProxyModel(this);

    avoinProxy->setSourceModel(maksuProxy_);
    avoinProxy->setFilterRole(Qt::DisplayRole);
    avoinProxy->setFilterKeyColumn(LaskuTauluModel::MAKSAMATTA);
    avoinProxy->setFilterFixedString("€");


    ui->maksuView->setModel(avoinProxy);
    ui->maksuView->setSortingEnabled(true);
    avoinProxy->setDynamicSortFilter(true);
    ui->maksuView->hideColumn( LaskuTauluModel::LAHETYSTAPA );
    connect( ui->maksuView->selectionModel(), &QItemSelectionModel::currentRowChanged , this, &TilioteKirjaaja::valitseLasku);
    connect( ui->suodatusEdit, &QLineEdit::textEdited, this, &TilioteKirjaaja::suodata);

    connect( ui->suljeNappi, &QPushButton::clicked,
             this, &TilioteKirjaaja::tyhjenna);

    ui->pvmEdit->setDate( apuri->tosite()->data(Tosite::PVM).toDate() );
    tyhjenna();

    connect( ui->euroEdit, &KpEuroEdit::textChanged, this, &TilioteKirjaaja::tarkastaTallennus);
    connect( ui->pvmEdit, &KpDateEdit::dateChanged, this, &TilioteKirjaaja::tarkastaTallennus);
    connect( ui->tiliEdit, &TilinvalintaLine::textChanged, this, &TilioteKirjaaja::tarkastaTallennus);
    connect( ui->tiliEdit, &TilinvalintaLine::textChanged, this, &TilioteKirjaaja::tiliMuuttuu);
    connect( ui->maksuView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &TilioteKirjaaja::tarkastaTallennus);
    connect( ui->eraCombo, &EraCombo::valittu, this, &TilioteKirjaaja::eraValittu);
    connect( ui->jaksoAlkaaEdit, &KpDateEdit::dateChanged, this, &TilioteKirjaaja::jaksomuuttuu);

    connect( ui->asiakastoimittaja, &AsiakasToimittajaValinta::valittu, this, &TilioteKirjaaja::kumppaniValittu);

    connect( new QShortcut(QKeySequence("F12"), this), &QShortcut::activated, this, &TilioteKirjaaja::accept);


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
    rivi.kohdennus = ui->kohdennusCombo->currentData(KohdennusModel::IdRooli).toInt();
    rivi.merkkaukset.clear();
    for(auto var : ui->merkkausCC->selectedDatas())
        rivi.merkkaukset.append( var.toInt() );


    if( ui->alaTabs->currentIndex() == MAKSU) {
        QModelIndex index = ui->maksuView->currentIndex();

        rivi.saajamaksaja = index.data(LaskuTauluModel::AsiakasToimittajaNimiRooli).toString();
        rivi.saajamaksajaId = index.data(LaskuTauluModel::AsiakasToimittajaIdRooli).toInt();
        rivi.eraId = index.data(LaskuTauluModel::EraIdRooli).toInt();
        rivi.laskupvm = index.data(LaskuTauluModel::LaskuPvmRooli).toDate();
        rivi.tili = index.data(LaskuTauluModel::TiliRooli).toInt();
        if( rivi.selite.isEmpty())
            rivi.selite = index.data(LaskuTauluModel::AsiakasToimittajaNimiRooli ).toString();

    } else if( ui->alaTabs->currentIndex() == TULOMENO ) {
        rivi.saajamaksajaId = ui->asiakastoimittaja->id();
        rivi.saajamaksaja = ui->asiakastoimittaja->nimi();
        rivi.eraId = ui->eraCombo->valittuEra();

        if( rivi.selite.isEmpty())
            rivi.selite = rivi.saajamaksaja;
        rivi.jaksoalkaa = ui->jaksoAlkaaEdit->date();
        rivi.jaksoloppuu = ui->jaksoLoppuuEdit->date();
    }

    return rivi;
}

void TilioteKirjaaja::accept()
{
    if( muokattavaRivi_) {
        apuri()->model()->muokkaaRivi( muokattavaRivi_, rivi());
        muokattavaRivi_ = 0;
    } else {
        apuri()->model()->lisaaRivi( rivi());
    }

    tyhjenna();
}

void TilioteKirjaaja::muokkaaRivia(int riviNro)
{
    muokattavaRivi_ = riviNro;
    TilioteModel::Tilioterivi rivi = apuri()->model()->rivi(riviNro);

    ui->ylaTab->setCurrentIndex( rivi.euro < 0 );
    if( rivi.eraId )
        ui->alaTabs->setCurrentIndex( MAKSU );
    else if( QString::number(rivi.tili).startsWith('1') ||
             QString::number(rivi.tili).startsWith('2'))
        ui->alaTabs->setCurrentIndex( SIIRTO );
    else
        ui->alaTabs->setCurrentIndex( TULOMENO );

    ui->pvmEdit->setDate( rivi.pvm );
    ui->euroEdit->setValue( rivi.euro );
    ui->seliteEdit->setText( rivi.selite );
    ui->tiliEdit->valitseTiliNumerolla( rivi.tili );
    ui->kohdennusCombo->setCurrentIndex(
                ui->kohdennusCombo->findData( rivi.kohdennus, KohdennusModel::IdRooli));
    ui->eraCombo->valitse( rivi.eraId );

    // Etsitään valittava rivi
    for(int i=0; i < maksuProxy_->rowCount(); i++) {
        if( maksuProxy_->data( maksuProxy_->index(i,0), LaskuTauluModel::EraIdRooli ).toInt() == rivi.eraId) {
            ui->maksuView->selectRow(i);
            break;
        }
    }

    // MerkkausCC
    lataaMerkkaukset( rivi.merkkaukset);
    ui->asiakastoimittaja->set( rivi.saajamaksajaId,
                                rivi.saajamaksaja);
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
        ui->tiliEdit->valitseTiliNumerolla(  menoa_ ? 4000 : 3000 );    // TODO: Tod. oletukset

    } else if ( tab == SIIRTO ) {
        ui->tiliLabel->setText( menoa_ ? tr("Tilille") : tr("Tililtä")  );
        ui->asiakasLabel->setText( menoa_ ? tr("Saaja") : tr("Maksaja"));
        ui->tiliEdit->suodataTyypilla( "[AB].*");
        ui->tiliEdit->clear();
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

    bool erat = tili.eritellaankoTase();
    ui->eraLabel->setVisible(erat);
    ui->eraCombo->setVisible(erat);
    if( erat ) {
        ui->eraCombo->lataa(tili.numero());
    }

    bool jakso = tili.onko(TiliLaji::TULOS) &&
            ui->ylaTab->currentIndex() == TULOMENO;
    ui->jaksotusLabel->setVisible(jakso);
    ui->jaksoAlkaaEdit->setVisible(jakso);
    ui->jaksoViivaLabel->setVisible(jakso);
    ui->jaksoLoppuuEdit->setVisible(jakso);
}

void TilioteKirjaaja::eraValittu(int /* eraId */, double avoinna)
{
    if( !ui->euroEdit->asCents() && avoinna > 1e-5)
        ui->euroEdit->setValue(menoa_ ? 0 - avoinna : avoinna);

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
    lataaMerkkaukset();
    tarkastaTallennus();
}

void TilioteKirjaaja::tarkastaTallennus()
{
    ui->okNappi->setEnabled(
                qAbs(ui->euroEdit->value()) > 1e-5 &&
                ( ui->tiliEdit->valittuTilinumero() ||
                  !ui->maksuView->selectionModel()->selectedRows().isEmpty() ));
}

void TilioteKirjaaja::lataaMerkkaukset(QList<int> merkatut)
{
    // Kohdennukset
    kohdennusProxy_->asetaPaiva(ui->pvmEdit->date());

    KohdennusProxyModel merkkausproxy(this, ui->pvmEdit->date(), -1, KohdennusProxyModel::MERKKKAUKSET );
    ui->merkkausCC->clear();

    for(int i=0; i < merkkausproxy.rowCount(); i++) {
        int koodi = merkkausproxy.data( merkkausproxy.index(i,0), KohdennusModel::IdRooli ).toInt();
        QString nimi = merkkausproxy.data( merkkausproxy.index(i,0), KohdennusModel::NimiRooli ).toString();

            Qt::CheckState state = merkatut.contains( koodi ) ? Qt::Checked : Qt::Unchecked;
            ui->merkkausCC->addItem(nimi, koodi, state);
    }
}

void TilioteKirjaaja::kumppaniValittu(int kumppaniId)
{
    KpKysely *kysely = kpk(QString("/kumppanit/%1").arg(kumppaniId));
    connect(kysely, &KpKysely::vastaus, this, &TilioteKirjaaja::kumppaniTiedot);
    kysely->kysy();
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

TilioteApuri *TilioteKirjaaja::apuri()
{
    return qobject_cast<TilioteApuri*>( parent() );
}
