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
#include "tiliotemodel.h"

#include "tiliotekirjaaja.h"
#include "model/tosite.h"
#include "model/tositeviennit.h"

#include <QDate>
#include <QSortFilterProxyModel>

#include "kirjaus/kirjauswg.h"

#include "tiliotekirjaaja.h"
#include "db/kirjanpito.h"

#include <QDebug>
#include <QSettings>

TilioteApuri::TilioteApuri(QWidget *parent, Tosite *tosite)
    : ApuriWidget (parent,tosite),
      ui( new Ui::TilioteApuri),
      model_(new TilioteModel(this)),
      kirjaaja_(new TilioteKirjaaja(this))
{
    ui->setupUi(this);

    proxy_ = new QSortFilterProxyModel(this);
    proxy_->setSourceModel( model_ );

    ui->oteView->setModel(proxy_);
    proxy_->setSortRole(TilioteModel::LajitteluRooli);
    proxy_->setFilterRole(TilioteModel::HarmaaRooli);

    ui->tiliCombo->suodataTyypilla("ARP");
    laitaPaivat( tosite->data(Tosite::PVM).toDate() );

    connect( ui->lisaaRiviNappi, &QPushButton::clicked, this, &TilioteApuri::lisaaRivi);
    connect( ui->lisaaTyhjaBtn, &QPushButton::clicked, this, &TilioteApuri::lisaaTyhjaRivi );
    connect( ui->oteView->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
             this, SLOT(riviValittu()));
    connect(ui->muokkaaNappi, &QPushButton::clicked, this, &TilioteApuri::muokkaa);
    connect(ui->poistaNappi, &QPushButton::clicked, this, &TilioteApuri::poista);

    connect( model_, &TilioteModel::dataChanged, this, &TilioteApuri::tositteelle);
    connect( model_, &TilioteModel::rowsInserted, this, &TilioteApuri::tositteelle);
    connect( model_, &TilioteModel::rowsRemoved, this, &TilioteApuri::tositteelle);
    connect( model_, &TilioteModel::modelReset, this, &TilioteApuri::tositteelle);

    connect( ui->alkuDate, &KpDateEdit::dateChanged, this, &TilioteApuri::tiliPvmMuutos);
    connect( ui->loppuDate, &KpDateEdit::dateChanged, this, &TilioteApuri::tiliPvmMuutos);

    connect( tosite, &Tosite::pvmMuuttui, this, &TilioteApuri::laitaPaivat);

    connect( ui->harmaaNappi, &QPushButton::toggled, this, &TilioteApuri::naytaHarmaat);
    ui->harmaaNappi->setChecked(!kp()->settings()->value("TiliotePiilotaHarmaat",false).toBool());

    connect( ui->tiliCombo, &TiliCombo::currentTextChanged, this, &TilioteApuri::kysyAlkusumma);
    connect( ui->tiliCombo, &TiliCombo::currentTextChanged, this, &TilioteApuri::teeTositteelle);
    connect( ui->tiliCombo, &TiliCombo::currentTextChanged, this, &TilioteApuri::tiliPvmMuutos);

    ui->oteView->horizontalHeader()->setSectionResizeMode( TilioteModel::SELITE, QHeaderView::Stretch );
    connect( ui->oteView, &QTableView::doubleClicked, this, &TilioteApuri::muokkaa);
}

TilioteApuri::~TilioteApuri()
{
    delete ui;
}

void TilioteApuri::tuo(QVariantMap map)
{
    tuodaan_ = true;

    qDebug() << map;

    if( map.contains("iban")) {
        QString iban = map.value("iban").toString();
        Tili tili = kp()->tilit()->tiliIbanilla(iban);
        if( tili.onko(TiliLaji::PANKKITILI))
            ui->tiliCombo->valitseTili(tili.numero());
    } else if( map.contains("tili"))
        ui->tiliCombo->valitseTili( map.value("tili").toInt());

    ui->alkuDate->setDate( map.value("alkupvm").toDate() );
    ui->loppuDate->setDate( map.value("loppupvm").toDate());
    model()->tuo( map.value("tapahtumat").toList() );


    if( map.contains("kausitunnus")) {
        QString tilinimi = kp()->tilit()->tiliIbanilla(map.value("iban").toString()).nimi();
        tosite()->asetaOtsikko(tr("Tiliote %1 %2").arg(map.value("kausitunnus").toString()).arg(tilinimi));
    }

    lataaHarmaat();

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
    tosite()->viennit()->asetaViennit( model_->viennit(  ui->tiliCombo->valittuTilinumero() ) );
    QVariantMap tilioteMap;
    tilioteMap.insert("alkupvm", ui->alkuDate->date());
    tilioteMap.insert("loppupvm", ui->loppuDate->date());
    tilioteMap.insert("tili", ui->tiliCombo->valittuTilinumero());
    tosite()->setData(Tosite::TILIOTE,tilioteMap);
    if( tosite()->data(Tosite::OTSIKKO).toString().isEmpty())
        tosite()->setData( Tosite::OTSIKKO,
                       tr("Tiliote %1 - %2 %3")
                       .arg( ui->alkuDate->date().toString("dd.MM.yyyy") )
                       .arg( ui->loppuDate->date().toString("dd.MM.yyyy"))
                       .arg( kp()->tilit()->tiliNumerolla(ui->tiliCombo->valittuTilinumero()).nimi()));

    naytaSummat();    
    return true;
}

void TilioteApuri::teeReset()
{
    kirjaaja_->close();
    const QVariantList& viennit = tosite()->viennit()->vientilLista();
    if( viennit.count() > 1) {
        TositeVienti ekarivi = viennit.first().toMap();
        ui->tiliCombo->valitseTili(ekarivi.tili());
    }
    QVariantMap tilioteMap = tosite()->data(Tosite::TILIOTE).toMap();
    if( !tilioteMap.isEmpty()) {
        ui->tiliCombo->valitseTili( tilioteMap.value("tili").toInt() );
        ui->alkuDate->setDate( tilioteMap.value("alkupvm").toDate());
        ui->loppuDate->setDate( tilioteMap.value("loppupvm").toDate() );
    }
    model_->lataa(viennit);
    if( kp()->yhteysModel())
        lataaHarmaat();
}

void TilioteApuri::lisaaRivi()
{
    kirjaaja_->kirjaaUusia();
}

void TilioteApuri::lisaaTyhjaRivi()
{
    TilioteModel::Tilioterivi rivi;
    rivi.pvm = tosite()->data(Tosite::PVM).toDate();
    model_->lisaaRivi(rivi);
}

void TilioteApuri::riviValittu()
{
    ui->muokkaaNappi->setEnabled( ui->oteView->currentIndex().isValid() );
    ui->poistaNappi->setEnabled( ui->oteView->currentIndex().isValid());
}

void TilioteApuri::muokkaa()
{
    kirjaaja_->show();
    kirjaaja_->muokkaaRivia( proxy_->mapToSource( ui->oteView->currentIndex()).row()  );
}

void TilioteApuri::poista()
{
    model_->poistaRivi( proxy_->mapToSource( ui->oteView->currentIndex()).row() );
}

void TilioteApuri::naytaSummat()
{
    qlonglong panot = 0l;
    qlonglong otot = 0l;

    for(int i=0; i < model_->rowCount(); i++) {
        qlonglong sentit = qRound64(  model_->data( model_->index(i, TilioteModel::EURO), Qt::EditRole ).toDouble() * 100);
        if( sentit > 0)
            panot += sentit;
        else
            otot += qAbs(sentit);
    }
    double loppusaldo = alkusaldo_ + (panot - otot) / 100.0;

    ui->infoLabel->setText(tr("Alkusaldo %L3 € \tPanot %L1 € \tOtot %L2 € \tLoppusaldo %L4 €")
                           .arg((panot / 100.0), 0, 'f', 2)
                           .arg((otot / 100.0), 0, 'f', 2)
                           .arg(alkusaldo_,0,'f',2)
                           .arg(loppusaldo,0,'f',2));
}

void TilioteApuri::tiliPvmMuutos()
{
    if( tosite()->resetoidaanko() )
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
                       .arg( ui->alkuDate->date().toString("dd.MM.yyyy") )
                       .arg( ui->loppuDate->date().toString("dd.MM.yyyy"))
                       .arg(tili.nimi()));
    }

    tosite()->setData(Tosite::PVM, ui->loppuDate->date());
}

void TilioteApuri::lataaHarmaat()
{
    model_->lataaHarmaat( ui->tiliCombo->valittuTilinumero(),
                          ui->alkuDate->date(),
                          ui->loppuDate->date(),
                          tosite()->id());
    kysyAlkusumma();
}

void TilioteApuri::laitaPaivat(const QDate &pvm)
{
    if( pvm != ui->loppuDate->date() && pvm.isValid()) {
        ui->loppuDate->setDate(pvm);
        ui->alkuDate->setDate( pvm.addDays(1).addMonths(-1) );
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
    int tilinumero = ui->tiliCombo->valittuTilinumero();
    alkusaldo_ = map.value(QString::number(tilinumero)).toDouble();
    naytaSummat();
}

void TilioteApuri::naytaHarmaat(bool nayta)
{
    proxy_->setFilterFixedString( nayta ? "" : "-");
    kp()->settings()->setValue("'TiliotePiilotaHarmaat", !nayta);
}


