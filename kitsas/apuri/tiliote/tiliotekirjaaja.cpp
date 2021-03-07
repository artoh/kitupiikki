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
    connect( laskut_, &LaskuTauluTilioteProxylla::modelReset, [this] { this->suodata(this->ui->suodatusEdit->text()); ui->maksuView->resizeColumnToContents(LaskuTauluModel::ASIAKASTOIMITTAJA); });


    ui->alvCombo->addItem(QIcon(":/pic/tyhja.png"), tr("Veroton"), 0);
    ui->alvCombo->addItem(QIcon(":/pic/lihavoi.png"), QString("24 %"), 24.0);
    ui->alvCombo->addItem(QIcon(":/pic/lihavoi.png"), QString("14 %"), 14.0);
    ui->alvCombo->addItem(QIcon(":/pic/lihavoi.png"), QString("10 %"), 10.0);

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
        paivita();
        if( riviIndeksi_ > -1) {
            apuri()->model()->asetaRivi(riviIndeksi_, rivi_);
            QDialog::accept();
        } else {
            apuri()->model()->lisaaRivi(rivi_);
        }
        tyhjenna();
    }
}

void TilioteKirjaaja::kirjaaUusia(const QDate &pvm)
{
    setWindowTitle( tr("Kirjaa tiliotteelle"));
    riviIndeksi_ = -1;
    rivi_ = TilioteKirjausRivi(pvm, apuri()->model());
    show();
}

void TilioteKirjaaja::muokkaaRivia(int riviNro)
{
    setWindowTitle(tr("Muokkaa tiliotekirjausta"));

    riviIndeksi_ = riviNro;
    rivi_ = apuri()->model()->rivi(riviNro);

    lataa();
    show();
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
        Tili* valittuna = ui->tiliEdit->tili();
        if( !valittuna || (menoa_ && !valittuna->onko(TiliLaji::MENO)) || (!menoa_ && !valittuna->onko(TiliLaji::TULO)) )
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

    if(tili.luku("kohdennus"))
        ui->kohdennusCombo->valitseKohdennus(tili.luku("kohdennus"));

    paivitaAlvInfo();
}

void TilioteKirjaaja::paivitaAlvInfo()
{
    Tili tili = ui->tiliEdit->valittuTili();
    bool vero = tili.luku("alvlaji") && ui->alaTabs->currentIndex() == TULOMENO;

    ui->alvLabel->setVisible(false);
    ui->alvCombo->setVisible(false);
    ui->alvVaro->setVisible(vero);

    if ( !menoa_) {
        ui->alvVaro->setText(tr("Tällä toiminnolla voit tehdä vain verottomia kirjauksia.\n"
                                "Kirjaa verolliset tulot tositetyypillä Tulo"));
    } else  {
        ui->alvLabel->setVisible(vero);
        ui->alvCombo->setVisible(vero);
        ui->alvVaro->setText(tr("Tiliotteen yhteydessä voit kirjata alv-vähennyksen vain bruttomenettelyllä.\n"
                                    "Tositteessa on oltava riittävät alv-merkinnät."));

    }
}

void TilioteKirjaaja::eraValittu(int eraId, double avoinna, const QString &selite)
{
    if( !ui->euroEdit->asCents() && avoinna > 1e-5)
        ui->euroEdit->setValue(menoa_ ? 0 - avoinna : avoinna);
    if( ui->seliteEdit->toPlainText().isEmpty())
        ui->seliteEdit->setText(selite);
    haeAlkuperaisTosite(eraId);

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

TilioteApuri *TilioteKirjaaja::apuri() const
{
    return qobject_cast<TilioteApuri*>( parent() );
}

void TilioteKirjaaja::lataa()
{

    const TositeVienti& pankki = rivi_.pankkivienti();
    const TositeVienti& tapatuma = rivi_.viennit().value(1);
    const int tili = tapatuma.tili();

    QString saajamaksaja = pankki.kumppaniNimi();
    int valinpaikka = saajamaksaja.indexOf(QRegularExpression("\\W",QRegularExpression::UseUnicodePropertiesOption));
    if( valinpaikka > 2)
        saajamaksaja = saajamaksaja.left(valinpaikka);

    double euro = pankki.debet() > 1e-5 ? pankki.debet() : pankki.kredit();

    ui->ylaTab->setCurrentIndex( euro < 0 );

    if( tapatuma.eraId() )
        ui->alaTabs->setCurrentIndex( MAKSU );
    else if( QString::number(tili).startsWith('1') ||
             QString::number(tili).startsWith('2'))
        ui->alaTabs->setCurrentIndex( SIIRTO );
    else
        ui->alaTabs->setCurrentIndex(TULOMENO );

    ui->kohdennusCombo->valitseKohdennus( tapatuma.kohdennus() );
    ui->tiliEdit->valitseTiliNumerolla( tili );

    // Etsitään valittava rivi
    avoinProxy_->setFilterFixedString("");

    bool maksu = false;
    if( tapatuma.eraId()) {
        for(int i=0; i < avoinProxy_->rowCount(); i++) {
            if( avoinProxy_->data( avoinProxy_->index(i,0), LaskuTauluModel::EraIdRooli ).toInt() == tapatuma.eraId()) {
                ui->maksuView->selectRow(i);
                maksu = true;
                break;
            }
        }
    }
    if( tapatuma.eraId() && !maksu) {
        ui->alaTabs->setCurrentIndex( SIIRTO );
    }

    ui->eraCombo->valitse( tapatuma.eraId() );

    // MerkkausCC
    ui->merkkausCC->haeMerkkaukset( tapatuma.pvm() );
    ui->merkkausCC->setSelectedItems( tapatuma.merkkaukset() );

    ui->asiakastoimittaja->set( pankki.kumppaniId(),
                                pankki.kumppaniNimi());

    ui->pvmEdit->setDate( pankki.pvm() );
    ui->euroEdit->setValue( euro );
    ui->seliteEdit->setText( pankki.selite() );

    ui->jaksoAlkaaEdit->setDate( tapatuma.jaksoalkaa() );
    ui->jaksoLoppuuEdit->setDate( tapatuma.jaksoloppuu() );

    ui->alvCombo->setCurrentIndex(ui->alvCombo->findData(tapatuma.alvProsentti()));

    ui->suodatusEdit->setText( saajamaksaja );
    if( !saajamaksaja.isEmpty())
        suodata(saajamaksaja);
}

void TilioteKirjaaja::paivita()
{
    TositeVienti pankki = rivi_.pankkivienti();
    TositeVienti tapahtuma = rivi_.viennit().value(1);

    pankki.setPvm(ui->pvmEdit->date());
    tapahtuma.setPvm(ui->pvmEdit->date());

    pankki.setDebet( ui->euroEdit->value());
    tapahtuma.setKredit( ui->euroEdit->value());

    pankki.setSelite( ui->seliteEdit->toPlainText());
    tapahtuma.setSelite( ui->seliteEdit->toPlainText());

    tapahtuma.setKohdennus(ui->kohdennusCombo->kohdennus());
    tapahtuma.setMerkkaukset(ui->merkkausCC->selectedDatas());

    QVariantMap kumppani;


    if( ui->alaTabs->currentIndex() == MAKSU) {
        QModelIndex index = ui->maksuView->currentIndex();

        kumppani.insert("nimi",index.data(LaskuTauluModel::AsiakasToimittajaNimiRooli).toString());
        kumppani.insert("id",index.data(LaskuTauluModel::AsiakasToimittajaIdRooli).toInt());

        tapahtuma.setEra(index.data(LaskuTauluModel::EraMapRooli).toMap());
        tapahtuma.setTili( index.data(LaskuTauluModel::TiliRooli).toInt());
        if( tapahtuma.selite().isEmpty())
                tapahtuma.setSelite(index.data(LaskuTauluModel::SeliteRooli).toString());

    } else {
        kumppani = ui->asiakastoimittaja->map();
        tapahtuma.setEra( ui->eraCombo->eraMap() );
        tapahtuma.setJaksoalkaa( ui->jaksoAlkaaEdit->date() );
        tapahtuma.setJaksoloppuu( ui->jaksoLoppuuEdit->date() );
        tapahtuma.setTili( ui->tiliEdit->valittuTilinumero());

    }

    pankki.setKumppani(kumppani);
    tapahtuma.setKumppani(kumppani);

    double alvprosentti = ui->alvCombo->isVisible() ? ui->alvCombo->currentData().toDouble() : 0.0;
    if( alvprosentti > 1e-5) {
        tapahtuma.setAlvKoodi( menoa_ ? AlvKoodi::OSTOT_BRUTTO : AlvKoodi::MYYNNIT_BRUTTO);
        tapahtuma.setAlvProsentti(alvprosentti);
    } else {
        tapahtuma.setAlvKoodi( AlvKoodi::EIALV);
        tapahtuma.setAlvProsentti(0);
    }

    QList<TositeVienti> rivit;
    rivit << pankki << tapahtuma;
    rivi_.asetaViennit(rivit);
}
