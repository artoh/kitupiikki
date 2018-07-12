/*
   Copyright (C) 2018 Arto Hyvättinen

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

#include "laskusivu.h"

#include "asiakkaatmodel.h"
#include "laskutmodel.h"
#include "ostolaskutmodel.h"
#include "db/kirjanpito.h"

#include <QTabBar>
#include <QSplitter>
#include <QTableView>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QDateEdit>
#include <QLabel>

#include <QHeaderView>
#include <QSortFilterProxyModel>

LaskuSivu::LaskuSivu()
    : asiakasmodel_(new AsiakkaatModel(this))
{
    luoUi();
    lajiTab_->setCurrentIndex(LaskutModel::AVOIMET);

    connect( paaTab_, &QTabBar::currentChanged, this, &LaskuSivu::paaTab );
    connect( lajiTab_, &QTabBar::currentChanged, this, &LaskuSivu::paivitaLaskulista);

    asiakasProxy_ = new QSortFilterProxyModel(this);
    asiakasProxy_->setSourceModel(asiakasmodel_);
    asiakasProxy_->setFilterKeyColumn(AsiakkaatModel::NIMI);
    asiakasProxy_->setDynamicSortFilter(true);
    asiakasProxy_->setFilterCaseSensitivity(Qt::CaseInsensitive);

    asiakasView_->setModel(asiakasProxy_);
    asiakasView_->horizontalHeader()->setSectionResizeMode(AsiakkaatModel::NIMI, QHeaderView::Stretch);
    asiakasView_->setSelectionBehavior(QTableView::SelectRows);
    asiakasView_->setSelectionMode(QTableView::SingleSelection);
    asiakasView_->setSortingEnabled(true);

    laskuAsiakasProxy_ = new QSortFilterProxyModel(this);
    laskuAsiakasProxy_->setFilterKeyColumn(LaskutModel::ASIAKAS);
    laskuViiteProxy_ = new QSortFilterProxyModel(this);
    laskuAsiakasProxy_->setFilterCaseSensitivity(Qt::CaseInsensitive);
    laskuViiteProxy_->setSourceModel(laskuAsiakasProxy_);
    laskuViiteProxy_->setFilterRole(LaskutModel::ViiteRooli);
    laskuViiteProxy_->setDynamicSortFilter(true);

    laskuView_->setModel(laskuViiteProxy_);
    laskuView_->setSelectionBehavior(QTableView::SelectRows);
    laskuView_->setSelectionMode(QTableView::SingleSelection);
    laskuView_->setSortingEnabled(true);
    laskuView_->horizontalHeader()->setStretchLastSection(true);

    connect( viiteSuodatusEdit_, &QLineEdit::textChanged,
             laskuViiteProxy_, &QSortFilterProxyModel::setFilterFixedString);
    connect( asiakasSuodatusEdit_, &QLineEdit::textChanged,
             this, &LaskuSivu::paivitaAsiakasSuodatus);
    connect( asiakasView_->selectionModel(), &QItemSelectionModel::currentChanged,
             this, &LaskuSivu::asiakasValintaMuuttuu);
    connect( mistaEdit_, &QDateEdit::dateChanged, this, &LaskuSivu::paivitaLaskulista);
    connect( mihinEdit_, &QDateEdit::dateChanged, this, &LaskuSivu::paivitaLaskulista);


    paaTab(MYYNTI);
}

LaskuSivu::~LaskuSivu()
{

}

void LaskuSivu::siirrySivulle()
{
    mistaEdit_->setDate(kp()->tilikaudet()->kirjanpitoAlkaa());
    mihinEdit_->setDate(kp()->tilikaudet()->kirjanpitoLoppuu());
    paaTab( paaTab_->currentIndex() );
}

void LaskuSivu::paaTab(int indeksi)
{
    asiakasView_->setVisible( indeksi >= ASIAKAS );

    if( indeksi >= ASIAKAS && lajiTab_->count() < 4)
        lajiTab_->addTab(tr("&Yhteystiedot"));
    else if(indeksi < ASIAKAS && lajiTab_->count() == 4)
        lajiTab_->removeTab(TIEDOT);

    if( indeksi ==  ASIAKAS)
        asiakasmodel_->paivita(false);
    else if( indeksi == TOIMITTAJA)
        asiakasmodel_->paivita(true);

    if( laskumodel_ )
        delete laskumodel_;

    if( indeksi == MYYNTI || indeksi == ASIAKAS )
        laskumodel_ = new LaskutModel(this);
    else
        laskumodel_ = new OstolaskutModel(this);

    paivitaLaskulista();
    laskuAsiakasProxy_->setSourceModel(laskumodel_);
    paivitaAsiakasSuodatus();

}

void LaskuSivu::paivitaAsiakasSuodatus()
{
    if( paaTab_->currentIndex() >= ASIAKAS)
    {
        asiakasProxy_->setFilterFixedString( asiakasSuodatusEdit_->text() );
    }
    else
    {
        laskuAsiakasProxy_->setFilterFixedString( asiakasSuodatusEdit_->text());
    }
}

void LaskuSivu::paivitaLaskulista()
{
    if( lajiTab_->currentIndex() < TIEDOT && laskumodel_)
    {
        laskumodel_->paivita( lajiTab_->currentIndex(), mistaEdit_->date(), mihinEdit_->date() );
    }
}

void LaskuSivu::asiakasValintaMuuttuu()
{
    laskuAsiakasProxy_->setFilterFixedString( asiakasView_->currentIndex().data(AsiakkaatModel::NimiRooli).toString() );
}

void LaskuSivu::luoUi()
{
    paaTab_ = new QTabBar();

    paaTab_->addTab(tr("&Myyntilaskut"));
    paaTab_->addTab(tr("&Ostolaskut"));
    paaTab_->addTab(tr("&Asiakkaat"));
    paaTab_->addTab(tr("&Toimittajat"));

    asiakasSuodatusEdit_ = new QLineEdit();
    asiakasSuodatusEdit_->setPlaceholderText( tr("Etsi asiakkaan nimellä") );

    QHBoxLayout *ylarivi = new QHBoxLayout;
    ylarivi->addWidget(paaTab_);
    ylarivi->addWidget(asiakasSuodatusEdit_);

    splitter_ = new QSplitter(Qt::Vertical);

    asiakasView_ = new QTableView();
    splitter_->addWidget(asiakasView_);

    lajiTab_ = new QTabBar();
    lajiTab_->addTab(tr("&Kaikki"));
    lajiTab_->addTab(tr("&Avoimet"));
    lajiTab_->addTab(tr("&Erääntyneet"));

    viiteSuodatusEdit_ = new QLineEdit();
    viiteSuodatusEdit_->setPlaceholderText(tr("Etsi viitenumerolla"));
    viiteSuodatusEdit_->setValidator(new QRegularExpressionValidator(QRegularExpression("\\d*")));

    QHBoxLayout *keskirivi = new QHBoxLayout;
    keskirivi->addWidget(lajiTab_);

    mistaEdit_ = new QDateEdit();
    mihinEdit_ = new QDateEdit();
    keskirivi->addWidget(mistaEdit_);
    keskirivi->addWidget(new QLabel(" - "));
    keskirivi->addWidget(mihinEdit_);

    keskirivi->addWidget(viiteSuodatusEdit_);

    QVBoxLayout *alaruutuleiska = new QVBoxLayout;
    alaruutuleiska->addLayout(keskirivi);

    laskuView_ = new QTableView;
    alaruutuleiska->addWidget(laskuView_);

    QWidget *alaWidget = new QWidget();
    alaWidget->setLayout(alaruutuleiska);
    splitter_->addWidget(alaWidget);

    QHBoxLayout *nappileiska = new QHBoxLayout;

    naytaNappi_ = new QPushButton(QIcon(":/pic/print.png"), tr("&Näytä"));
    nappileiska->addWidget(naytaNappi_);

    QVBoxLayout *paaLeiska = new QVBoxLayout;
    paaLeiska->addLayout(ylarivi);
    paaLeiska->addWidget(splitter_);
    paaLeiska->addLayout(nappileiska);

    setLayout(paaLeiska);

}
