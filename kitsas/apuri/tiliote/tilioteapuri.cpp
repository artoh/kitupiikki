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
#include "tilioteapuri.h"
#include "ui_tilioteapuri.h"

#include "tiliotekirjaaja.h"
#include "model/tosite.h"
#include "model/tositeviennit.h"

#include <QDate>
#include <QSortFilterProxyModel>

#include "kirjaus/kirjauswg.h"

#include "tiliotekirjaaja.h"
#include "db/kirjanpito.h"

#include "tiliotemodel.h"

#include "lisaikkuna.h"
#include "kirjaus/kirjaussivu.h"
#include "kirjaus/kirjauswg.h"
#include "kirjaus/pvmdelegaatti.h"

#include <QDebug>
#include <QSettings>
#include <QTimer>


TilioteApuri::TilioteApuri(QWidget *parent, Tosite *tosite)
    : ApuriWidget (parent,tosite),
      ui( new Ui::TilioteApuri),
      model_(new TilioteModel(this, kp())),
      kirjaaja_(new TilioteKirjaaja(this))
{
    ui->setupUi(this);

    proxy_ = model_->initProxy();

    ui->oteView->setModel(proxy_);
    proxy_->setSortRole(TilioteRivi::LajitteluRooli);
    proxy_->setFilterRole(TilioteRivi::TilaRooli);
    proxy_->setDynamicSortFilter(true);
    proxy_->sort(TilioteRivi::PVM);

    ui->tiliCombo->suodataTyypilla("ARP|BSP");
    laitaPaivat( tosite->data(Tosite::PVM).toDate() );

    connect( ui->lisaaRiviNappi, &QPushButton::clicked, this, [this]  {this->lisaaRivi(true);} );
    connect( ui->lisaaTyhjaBtn, &QPushButton::clicked, this, [this] {this->lisaaRivi(false);} );

    connect( ui->oteView->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
             this, SLOT(riviValittu()));
    connect(ui->muokkaaNappi, &QPushButton::clicked, this, &TilioteApuri::muokkaa);
    connect(ui->poistaNappi, &QPushButton::clicked, this, &TilioteApuri::poista);
    connect(ui->tositeNappi, &QPushButton::clicked, this, &TilioteApuri::naytaTosite);

    connect( model_, &TilioteModel::dataChanged, this, &TilioteApuri::tositteelle);
    connect( model_, &TilioteModel::rowsInserted, this, &TilioteApuri::tositteelle);
    connect( model_, &TilioteModel::rowsRemoved, this, &TilioteApuri::tositteelle);
    connect( model_, &TilioteModel::modelReset, this, &TilioteApuri::tositteelle);

    connect( ui->alkuDate, &KpDateEdit::dateChanged, this, &TilioteApuri::tiliPvmMuutos);
    connect( ui->loppuDate, &KpDateEdit::dateChanged, this, &TilioteApuri::tiliPvmMuutos);
    connect( ui->tiliCombo, &QComboBox::currentTextChanged, this, &TilioteApuri::tiliMuuttui);

    connect( tosite, &Tosite::pvmMuuttui, this, &TilioteApuri::laitaPaivat);

    connect( ui->harmaaNappi, &QPushButton::toggled, this, &TilioteApuri::naytaHarmaat);
    ui->harmaaNappi->setChecked(!kp()->settings()->value("TiliotePiilotaHarmaat",false).toBool());

    connect( ui->oteView, &QTableView::doubleClicked, this, &TilioteApuri::muokkaa);
    connect( kirjaaja_, &TilioteKirjaaja::rejected, this, &TilioteApuri::tositteelle);        

//    QTimer::singleShot(100, this, &TilioteApuri::lataaHarmaat);

}

TilioteApuri::~TilioteApuri()
{
    delete ui;
}

void TilioteApuri::tuo(QVariantMap map)
{
    tuodaan_ = true;

    const QDate alku = map.value("alkupvm").toDate();
    const QDate loppu = map.value("loppupvm").toDate();

    if( alku.isValid())
        ui->alkuDate->setDate(alku);
    if( loppu.isValid())
        ui->loppuDate->setDate(loppu);

    if( map.contains("iban")) {
        QString iban = map.value("iban").toString();
        Tili tili = kp()->tilit()->tiliIbanilla(iban);
        if( tili.onko(TiliLaji::PANKKITILI))
            ui->tiliCombo->valitseTili(tili.numero());
    } else if( map.contains("tili"))
        ui->tiliCombo->valitseTili( map.value("tili").toInt());

    model()->asetaTilinumero(ui->tiliCombo->valittuTilinumero());
    model()->tuo( map.value("tapahtumat").toList() );


    if( map.contains("kausitunnus")) {
        if( map.value("iban").toString().length() > 8) {
            QString tilinimi = kp()->tilit()->tiliIbanilla(map.value("iban").toString()).nimi();
            tosite()->asetaOtsikko(tr("Tiliote %1 %2").arg(map.value("kausitunnus").toString(), tilinimi));
        }
    }

    if( alku.isValid() && loppu.isValid())
        lataaHarmaatAjalta(alku, loppu);

    tuodaan_ = false;
}

QDate TilioteApuri::tiliotteenAlkupaiva() const
{
    return ui->alkuDate->date();
}

QDate TilioteApuri::tiliotteenLoppupaiva() const
{
    return ui->loppuDate->date();
}

void TilioteApuri::salliMuokkaus(bool sallitaanko)
{
    for( QObject* object : children()) {
        QWidget* widget = qobject_cast<QWidget*>(object);
        if(widget && widget != ui->oteView)
          widget->setEnabled(sallitaanko);
    }
    model_->salliMuokkaus(sallitaanko);
    riviValittu();
}

bool TilioteApuri::teeTositteelle()
{
    naytaSummat();

    if( resetoidaanko())
        return false;

    model()->asetaTilinumero(ui->tiliCombo->valittuTilinumero());
    tosite()->viennit()->asetaViennit( model_->viennit() );

    tilioteMap_.insert("alkupvm", ui->alkuDate->date().toString("yyyy-MM-dd"));
    tilioteMap_.insert("loppupvm", ui->loppuDate->date().toString("yyyy-MM-dd"));
    tilioteMap_.insert("tili", ui->tiliCombo->valittuTilinumero());
    tosite()->setData(Tosite::TILIOTE,tilioteMap_);

//    tosite()->asetaLaskupvm(QDate());
    tosite()->asetaKumppani(QVariantMap());
    tosite()->asetaTilioterivi(0);

    if( tosite()->data(Tosite::OTSIKKO).toString().isEmpty())
        tosite()->setData( Tosite::OTSIKKO,
                       tr("Tiliote %1 - %2 %3")
                       .arg( ui->alkuDate->date().toString("dd.MM.yyyy") , ui->loppuDate->date().toString("dd.MM.yyyy"), kp()->tilit()->tiliNumerolla(ui->tiliCombo->valittuTilinumero()).nimi()));

    return true;
}

void TilioteApuri::teeReset()
{
    kirjaaja_->close();
    const auto& viennit = tosite()->viennit()->viennit();
    if( viennit.count() > 1) {
        TositeVienti ekarivi = viennit.first();

        if( kp()->tilit()->tiliNumerolla(ekarivi.tili()).onko(TiliLaji::PANKKITILI)) {
            model_->asetaTilinumero(ekarivi.tili());
            ui->tiliCombo->valitseTili(ekarivi.tili());
        }
    }
    tilioteMap_ = tosite()->data(Tosite::TILIOTE).toMap();
    if( !tilioteMap_.isEmpty()) {
        ui->tiliCombo->valitseTili( tilioteMap_.value("tili").toInt() );
        ui->alkuDate->setDate( tilioteMap_.value("alkupvm").toDate());
        ui->loppuDate->setDate( tilioteMap_.value("loppupvm").toDate() );
    } else {
        ui->loppuDate->setDate(tosite()->pvm());
        ui->alkuDate->setDate( tosite()->pvm().addDays(1).addMonths(-1) );
    }

    model_->asetaTilinumero( ui->tiliCombo->valittuTilinumero() );
    model_->lataa(tosite()->viennit()->tallennettavat() );


    if( tosite()->viennit()->tallennettavat().empty())
        lisaaRivi();

    qApp->processEvents();
    QTimer::singleShot(50, this, &TilioteApuri::lataaHarmaat);
}

void TilioteApuri::lisaaRivi(bool dialogi)
{
    const QModelIndex& index = ui->oteView->currentIndex();
    QDate pvm = index.isValid()
            ? index.sibling(index.row(), TilioteRivi::PVM).data(Qt::EditRole).toDate()
            : ui->alkuDate->date();

    if(dialogi)
        kirjaaja_->kirjaaUusia(pvm);
    else
        model()->lisaaRivi(pvm);        
}

void TilioteApuri::riviValittu()
{
    const QModelIndex& index = ui->oteView->currentIndex();
    bool muokattavaRivi = index.data(TilioteRivi::TilaRooli).toString() == "AA" ;
    ui->muokkaaNappi->setEnabled( muokattavaRivi );
    ui->poistaNappi->setEnabled( muokattavaRivi );
    ui->tositeNappi->setEnabled( index.isValid() &&
                                 !index.sibling(index.row(), TilioteKirjausRivi::EURO).data(Qt::DisplayRole).toString().isEmpty());
}

void TilioteApuri::muokkaa()
{
    const QModelIndex& index = ui->oteView->selectionModel()->currentIndex();
    if(index.data(TilioteRivi::TositeIdRooli).toInt()) {
        naytaTosite();
    } else {
        kirjaaja_->show();
        kirjaaja_->muokkaaRivia( proxy_->mapToSource( index ).row()  );
    }
}

void TilioteApuri::poista()
{
    model_->poistaRivi( proxy_->mapToSource( ui->oteView->currentIndex()).row() );
}

void TilioteApuri::naytaSummat()
{
    QPair<qlonglong, qlonglong> muutos = model()->summat();
    Euro panot = Euro::fromCents(muutos.first);
    Euro otot = Euro::fromCents(muutos.second);

    Euro loppusaldo = alkusaldo_ + panot - otot;

    ui->infoLabel->setText(tr("Alkusaldo %3 \tPanot %1 \tOtot %2 \tLoppusaldo %4")
                               .arg(panot.display(), otot.display(), alkusaldo_.display(), loppusaldo.display()));
}

void TilioteApuri::naytaTosite()
{
    const QModelIndex& index = ui->oteView->selectionModel()->currentIndex();
    if( !index.isValid())
        return;

    LisaIkkuna* ikkuna = new LisaIkkuna;

    if(index.data(TilioteRivi::TositeIdRooli).toInt()) {
        QList<int> selauslista;
        for(int i=0; i < index.model()->rowCount(); i++) {
            int sId = index.sibling(i,0).data(TilioteRivi::TositeIdRooli).toInt();
            if( sId && !selauslista.contains(sId))
                selauslista.append(sId);
        }
        KirjausSivu* sivu = ikkuna->naytaTosite(index.data(TilioteRivi::TositeIdRooli).toInt(), selauslista);
        connect( sivu->kirjausWg()->tosite(), &Tosite::talletettu, this, &TilioteApuri::lataaHarmaat);

    } else {
        const int omaIndeksi = proxy_->mapToSource(index).row();
        const TilioteKirjausRivi& rivi = model()->rivi(omaIndeksi);

        KirjausSivu* sivu = ikkuna->kirjaa(-1, TositeTyyppi::MUU, QList<int>(), KirjausSivu::PALATAAN_AINA);                

        Tosite tosite;
        tosite.asetaPvm(rivi.pvm());
        tosite.asetaKumppani(rivi.kumppani());
        tosite.asetaOtsikko(rivi.otsikko());
        tosite.asetaViite(rivi.viite());
        tosite.asetaTilioterivi( omaIndeksi );

        QVariantList viennit = rivi.viennit(model_->tilinumero());

        if( rivi.tyyppi() == TositeVienti::SUORITUS || rivi.tyyppi() == TositeVienti::SIIRTO) {
            tosite.asetaTyyppi( TositeTyyppi::SIIRTO);
        } else if( rivi.tyyppi() == TositeVienti::MYYNTI) {
            tosite.asetaTyyppi(TositeTyyppi::TULO);
        } else if( rivi.tyyppi() == TositeVienti::OSTO) {
            tosite.asetaTyyppi( TositeTyyppi::MENO);
        } else if(!rivi.tyyppi()){
            if( rivi.summa() ) {
                tosite.asetaTyyppi( rivi.summa() > Euro::Zero ? TositeTyyppi::TULO : TositeTyyppi::MENO );
                for(int i=0; i < viennit.count(); i++) {
                    QVariantMap map = viennit.at(i).toMap();
                    map.insert("tyyppi", map.value("tyyppi").toInt() + tosite.tyyppi());
                    viennit.replace(i, map);
                }
            }
        }

        tosite.viennit()->asetaViennit( viennit );
        sivu->kirjausWg()->tosite()->lataa(tosite.tallennettava());

        // Varmistetaan, että oletustili yms tulee käyttöön
        if( sivu->kirjausWg()->apuri())
            sivu->kirjausWg()->apuri()->tositteelle();

        qApp->processEvents();

        connect( sivu->kirjausWg()->tosite(), &Tosite::talletettu, this, &TilioteApuri::lataaHarmaat);
    }
}


void TilioteApuri::tiliPvmMuutos()
{
    if( tosite()->resetoidaanko() || paivanlaitto_ )
        return;

    // Otsikon päivittäminen
    if( !tuodaan_)
        lataaHarmaat();

    Tili tili = kp()->tilit()->tiliNumerolla( ui->tiliCombo->valittuTilinumero() );    

    QString otsikko = tosite()->otsikko();
    if(( otsikko.isEmpty() || otsikko.contains(QRegularExpression(R"(\d{2}.\d{2}.\d{4} - \d{2}.\d{2}.\d{4})"))) &&
            ui->alkuDate->date().isValid() && ui->loppuDate->date().isValid()) {
        tosite()->setData( Tosite::OTSIKKO,
                       tr("Tiliote %1 - %2 %3")
                       .arg( ui->alkuDate->date().toString("dd.MM.yyyy"), ui->loppuDate->date().toString("dd.MM.yyyy"), tili.tyyppiKoodi() == "ARP" ? tili.nimi() : ""));
    }
    tosite()->asetaPvm( ui->loppuDate->date() );


    // Jos päivämäärää muutetaan, muuttuu myös avoimen kirjauksen päivä
    QModelIndex pvmIndex = ui->oteView->model()->index(0, TilioteRivi::PVM);
    if( pvmIndex.isValid() && Euro(pvmIndex.data(TilioteKirjausRivi::EuroRooli).toString()) == Euro::Zero) {
        ui->oteView->model()->setData(pvmIndex, ui->alkuDate->date());
    }



    tosite()->setData(Tosite::PVM, ui->loppuDate->date());
    kysyAlkusumma();
}

void TilioteApuri::tiliMuuttui()
{
    const int tilinumero = ui->tiliCombo->valittuTilinumero();
    if( tilinumero != model_->tilinumero()) {
        model_->asetaTilinumero(tilinumero);
        lataaHarmaat();
    }
}

void TilioteApuri::lataaHarmaat()
{
    lataaHarmaatAjalta( ui->alkuDate->date(), ui->loppuDate->date());
}

void TilioteApuri::lataaHarmaatAjalta(const QDate &mista, const QDate &mihin)
{

    model_->asetaTositeId(tosite()->id());
    model_->lataaHarmaat( mista, mihin);
    kysyAlkusumma();

//    kysyAlkusumma();
//    proxy_->sort(TilioteRivi::PVM);
}

void TilioteApuri::laitaPaivat(const QDate &pvm)
{
    if( pvm != ui->loppuDate->date() && pvm.isValid()) {
        paivanlaitto_ = true;
        ui->loppuDate->setDate(pvm);
        ui->alkuDate->setDate( pvm.addDays(1).addMonths(-1) );
        paivanlaitto_ = false;
        tiliPvmMuutos();
    }
}

void TilioteApuri::kysyAlkusumma()
{
    int tilinumero = ui->tiliCombo->valittuTilinumero();
    QDate alkupvm = ui->alkuDate->date();
    KpKysely *kysely = kpk("/saldot");
    if( kysely ) {
        kysely->lisaaAttribuutti("tili", tilinumero);
        kysely->lisaaAttribuutti("pvm", alkupvm);
        kysely->lisaaAttribuutti("tase");
        kysely->lisaaAttribuutti("alkusaldot");
        connect(kysely, &KpKysely::vastaus, this, &TilioteApuri::alkusummaSaapuu);
        kysely->kysy();
    }
}

void TilioteApuri::alkusummaSaapuu(QVariant* data)
{
    QVariantMap map = data->toMap();
    Tili tili = kp()->tilit()->tiliNumerolla(ui->tiliCombo->valittuTilinumero());
    Euro saldo = map.value(QString::number(tili.numero())).toString();
    alkusaldo_ = tili.onko(TiliLaji::VASTAAVAA) ? saldo : Euro::Zero - saldo;

    naytaSummat();
}

void TilioteApuri::naytaHarmaat(bool nayta)
{
    proxy_->setFilterFixedString( nayta ? "A" : "AA");
    kp()->settings()->setValue("'TiliotePiilotaHarmaat", !nayta);
}



