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
#include "laskudialogi.h"
#include "db/kirjanpito.h"
#include "db/tositemodel.h"
#include "lisaikkuna.h"
#include "naytin/naytinikkuna.h"
#include "yhteystietowidget.h"

#include <QTabBar>
#include <QSplitter>
#include <QTableView>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QDateEdit>
#include <QLabel>
#include <QSqlQuery>

#include <QHeaderView>
#include <QSortFilterProxyModel>

LaskuSivu::LaskuSivu()
    : asiakasmodel_(new AsiakkaatModel(this))
{
    luoUi();
    paaTab_->setCurrentIndex(MYYNTI);
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
    connect( kp(), &Kirjanpito::kirjanpitoaMuokattu, this, &LaskuSivu::paivitaLaskulista);
    connect( laskuView_->selectionModel(), &QItemSelectionModel::selectionChanged,
             this, &LaskuSivu::laskuValintaMuuttuu);

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

    if( indeksi == ASIAKAS && lajiTab_->count() < 4)
    {
        lajiTab_->addTab(tr("&Yhteystiedot"));
        lajiTab_->setTabEnabled(TIEDOT, false);
    }
    else if(indeksi != ASIAKAS && lajiTab_->count() == 4)
        lajiTab_->removeTab(TIEDOT);
    uusiAsiakasNappi_->setVisible(indeksi == ASIAKAS);

    if( indeksi ==  ASIAKAS)
        asiakasmodel_->paivita(false);
    else if( indeksi == TOIMITTAJA)
        asiakasmodel_->paivita(true);

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
    laskuView_->setVisible( lajiTab_->currentIndex() != TIEDOT);
    yhteystiedot_->setVisible( lajiTab_->currentIndex() == TIEDOT);

    asiakasSuodatusEdit_->setEnabled( lajiTab_->currentIndex() != TIEDOT);
    mistaEdit_->setEnabled( lajiTab_->currentIndex() != TIEDOT );
    mihinEdit_->setEnabled( lajiTab_->currentIndex() != TIEDOT );

    if( lajiTab_->currentIndex() < TIEDOT && laskumodel_)
    {
        laskumodel_->paivita( lajiTab_->currentIndex(), mistaEdit_->date(), mihinEdit_->date() );
        laskuValintaMuuttuu();
    }
    else
    {
        yhteystiedot_->haeTiedot( asiakasView_->currentIndex().data(AsiakkaatModel::NimiRooli).toString() );
    }
}

void LaskuSivu::asiakasValintaMuuttuu()
{
    laskuAsiakasProxy_->setFilterFixedString( asiakasView_->currentIndex().data(AsiakkaatModel::NimiRooli).toString() );
    if( paaTab_->currentIndex() == ASIAKAS )
        lajiTab_->setTabEnabled(TIEDOT, !asiakasView_->currentIndex().data(AsiakkaatModel::NimiRooli).toString().isEmpty() );
    if( lajiTab_->currentIndex() == TIEDOT)
        yhteystiedot_->haeTiedot( asiakasView_->currentIndex().data(AsiakkaatModel::NimiRooli).toString() );

}

void LaskuSivu::laskuValintaMuuttuu()
{
    if( laskuView_->currentIndex().isValid() )
    {
        QModelIndex index = laskuView_->currentIndex();
        int tosite = index.data(LaskutModel::TositeRooli).toInt();
        int liite = index.data(LaskutModel::LiiteRooli).toInt();

        QSqlQuery liitekysely( QString("SELECT id FROM liite WHERE tosite=%1 AND liiteno=%2").arg(tosite).arg(liite));
        naytaNappi_->setEnabled( liitekysely.next());

        tositeNappi_->setEnabled( index.data(LaskutModel::KirjausPerusteRooli).toInt() != LaskuModel::MAKSUPERUSTE );

        // Tarkistetaan, onko muokkaaminen sallittu
        TositeModel tositeModel( kp()->tietokanta());
        tositeModel.lataa(tosite);
        bool muokkausSallittu = tositeModel.muokkausSallittu() &&
                ((index.data(LaskutModel::KirjausPerusteRooli).toInt() != LaskuModel::MAKSUPERUSTE ||
                  index.data(LaskutModel::SummaRooli).toLongLong() == index.data(LaskutModel::AvoinnaRooli).toLongLong()) &&
                  index.data(LaskutModel::TyyppiRooli).toInt() != LaskuModel::OSTOLASKU);

        muokkaaNappi_->setEnabled( muokkausSallittu );
        poistaNappi_->setEnabled( muokkausSallittu );

        hyvitysNappi_->setEnabled( index.data(LaskutModel::TyyppiRooli).toInt() == LaskuModel::LASKU );
        muistutusNappi_->setVisible( index.data(LaskutModel::EraPvmRooli).toDate() < kp()->paivamaara() &&
                                     !index.data(LaskutModel::MuistutettuRooli).toBool());

    }
    else
    {
        naytaNappi_->setDisabled(true);
        muokkaaNappi_->setDisabled(true);
        tositeNappi_->setDisabled(true);
        poistaNappi_->setDisabled(true);
        hyvitysNappi_->setDisabled(true);
        muistutusNappi_->hide();
    }
}

void LaskuSivu::uusiLasku()
{
    LaskuModel *uusi = new LaskuModel();
    if( paaTab_->currentIndex() == ASIAKAS )
        uusi->asetaLaskunsaajannimi( asiakasView_->currentIndex().data(AsiakkaatModel::NimiRooli).toString() );
    LaskuDialogi* dlg = new LaskuDialogi(uusi);
    dlg->exec();
}

void LaskuSivu::uusiAsiakas()
{
    asiakasView_->selectRow(-1);
    lajiTab_->setCurrentIndex(TIEDOT);
    yhteystiedot_->haeTiedot();
}

void LaskuSivu::naytaTosite()
{
    LisaIkkuna *ikkuna = new LisaIkkuna();
    ikkuna->kirjaa( laskuView_->currentIndex().data(LaskutModel::TositeRooli).toInt() );
}

void LaskuSivu::naytaLasku()
{
    QModelIndex index = laskuView_->currentIndex();
    NaytinIkkuna::naytaLiite( index.data(LaskutModel::TositeRooli).toInt(),
                           index.data(LaskutModel::LiiteRooli).toInt());
}

void LaskuSivu::asiakasLisatty(const QString &nimi)
{
    asiakasmodel_->paivita(false);
    for(int i=0; i < asiakasView_->model()->rowCount(QModelIndex()); i++)
    {
        if( asiakasView_->model()->index(i,AsiakkaatModel::NIMI).data().toString() == nimi )
        {
            asiakasView_->selectRow(i);
            return;
        }
    }
}

void LaskuSivu::hyvityslasku()
{
    LaskuDialogi *dlg = new LaskuDialogi( LaskuModel::teeHyvityslasku(  laskuView_->currentIndex().data(LaskutModel::VientiIdRooli).toInt() ));
    dlg->exec();
}

void LaskuSivu::muokkaaLaskua()
{
    LaskuDialogi *dlg = new LaskuDialogi( LaskuModel::haeLasku( laskuView_->currentIndex().data(LaskutModel::VientiIdRooli).toInt() ) );
    dlg->show();
}

void LaskuSivu::maksumuistutus()
{
    LaskuDialogi *dlg = new LaskuDialogi( LaskuModel::teeMaksumuistutus( laskuView_->currentIndex().data(LaskutModel::VientiIdRooli).toInt() ));
    dlg->exec();
}

void LaskuSivu::ryhmaLasku()
{
    auto *dlg = new LaskuDialogi( LaskuModel::ryhmaLasku());
    dlg->exec();
}

void LaskuSivu::poistaLasku()
{

    TositeModel tosite( kp()->tietokanta() );
    tosite.lataa( laskuView_->currentIndex().data(LaskutModel::VientiIdRooli).toInt() );


    if( QMessageBox::question(nullptr, tr("Vahvista laskun poistaminen"),
                              tr("Haluatko varmasti poistaa laskun %1 asiakkaalle %2")
                              .arg(laskuView_->currentIndex().data(LaskutModel::ViiteRooli).toString())
                              .arg(laskuView_->currentIndex().data(LaskutModel::AsiakasRooli).toString()),
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No) != QMessageBox::Yes)
        return;

    tosite.poista();
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
    yhteystiedot_ = new YhteystietoWidget;
    alaruutuleiska->addWidget(yhteystiedot_);
    connect(yhteystiedot_, &YhteystietoWidget::uusiAsiakas, this, &LaskuSivu::asiakasLisatty );


    QWidget *alaWidget = new QWidget();
    alaWidget->setLayout(alaruutuleiska);
    splitter_->addWidget(alaWidget);

    QHBoxLayout *nappileiska = new QHBoxLayout;

    naytaNappi_ = new QPushButton(QIcon(":/pic/print.png"), tr("&Näytä"));
    connect( naytaNappi_, &QPushButton::clicked, this, &LaskuSivu::naytaLasku );
    nappileiska->addWidget(naytaNappi_);
    muokkaaNappi_ = new QPushButton( QIcon(":/pic/muokkaa.png"), tr("&Muokkaa"));
    connect( muokkaaNappi_, &QPushButton::clicked, this, &LaskuSivu::muokkaaLaskua);
    nappileiska->addWidget(muokkaaNappi_);
    tositeNappi_ = new QPushButton(QIcon(":/pic/tekstisivu.png"), tr("&Tosite"));
    connect( tositeNappi_, &QPushButton::clicked, this, &LaskuSivu::naytaTosite );
    nappileiska->addWidget(tositeNappi_);
    poistaNappi_ = new QPushButton(QIcon(":/pic/roskis.png"), tr("Poista"));
    nappileiska->addWidget(poistaNappi_);
    connect( poistaNappi_, &QPushButton::clicked, this, &LaskuSivu::poistaLasku);
    hyvitysNappi_ = new QPushButton(QIcon(":/pic/poista.png"), tr("&Hyvityslasku"));
    connect( hyvitysNappi_, &QPushButton::clicked, this, &LaskuSivu::hyvityslasku);
    nappileiska->addWidget(hyvitysNappi_);
    muistutusNappi_ = new QPushButton(QIcon(":/pic/varoitus.png"), tr("Maksumuistutus"));
    connect( muistutusNappi_, &QPushButton::clicked, this, &LaskuSivu::maksumuistutus);
    nappileiska->addWidget(muistutusNappi_);

    nappileiska->addStretch();
    uusiAsiakasNappi_ = new QPushButton(QIcon(":/pic/yrittaja.png"), tr("Uusi &asiakas"));
    connect( uusiAsiakasNappi_, &QPushButton::clicked, this, &LaskuSivu::uusiAsiakas);
    nappileiska->addWidget(uusiAsiakasNappi_);

    QPushButton *uusiNappi = new QPushButton(QIcon(":/pic/uusitiedosto.png"), tr("&Uusi lasku"));
    nappileiska->addWidget(uusiNappi);
    connect( uusiNappi, &QPushButton::clicked, this, &LaskuSivu::uusiLasku);

    QPushButton *ryhmaNappi = new QPushButton(QIcon(":/pic/dokumentti.png"), tr("&Ryhmälasku"));
    nappileiska->addWidget(ryhmaNappi);
    connect(ryhmaNappi, &QPushButton::clicked, this, &LaskuSivu::ryhmaLasku);


    QVBoxLayout *paaLeiska = new QVBoxLayout;
    paaLeiska->addLayout(ylarivi);
    paaLeiska->addWidget(splitter_);
    paaLeiska->addLayout(nappileiska);

    setLayout(paaLeiska);

}
