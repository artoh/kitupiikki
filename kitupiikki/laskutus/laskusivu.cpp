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
#include "laskudialogi.h"
#include "db/kirjanpito.h"
#include "db/tositemodel.h"
#include "lisaikkuna.h"
#include "naytin/naytinikkuna.h"
#include "yhteystietowidget.h"

#include "model/laskutaulumodel.h"

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
    : laskumodel_( new LaskuTauluModel(this)),
      asiakasmodel_(new AsiakkaatModel(this))
{
    luoUi();
    paaTab_->setCurrentIndex(MYYNTI);
    lajiTab_->setCurrentIndex(AVOIMET);

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
    laskuAsiakasProxy_->setFilterKeyColumn(LaskuTauluModel::ASIAKASTOIMITTAJA);
    laskuAsiakasProxy_->setSourceModel(laskumodel_);
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

    if( indeksi ==  ASIAKAS)
        asiakasmodel_->paivita(false);
    else if( indeksi == TOIMITTAJA)
        asiakasmodel_->paivita(true);

    if( indeksi == MYYNTI || indeksi == ASIAKAS) {
        if( lajiTab_->count() < 5) {
            lajiTab_->insertTab(LUONNOKSET,tr("Luonnokset"));
            lajiTab_->insertTab(LAHETETTAVAT,tr("Lähetettävät"));
            lajiTab_->setTabText(KAIKKI, tr("Lähetetyt"));
        }
    } else {
        if( lajiTab_->count() == 5) {
            lajiTab_->setTabText(KAIKKI, tr("Kaikki"));
            lajiTab_->removeTab(LAHETETTAVAT);
            lajiTab_->removeTab(LUONNOKSET);
        }
    }

    paivitaLaskulista();
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
    int laji = lajiTab_->count() == 5 ? lajiTab_->currentIndex() : lajiTab_->currentIndex() + 2;

    laskuView_->setColumnHidden( LaskuTauluModel::NUMERO,  laji == LUONNOKSET);
    laskuView_->setColumnHidden( LaskuTauluModel::MAKSAMATTA, laji < KAIKKI);
    laskuView_->setColumnHidden( LaskuTauluModel::LAHETYSTAPA, laji >= KAIKKI );

    laskumodel_->paivita( paaTab_->currentIndex() == OSTO || paaTab_->currentIndex() == TOIMITTAJA,
                          laji, mistaEdit_->date(), mihinEdit_->date() );
    laskuValintaMuuttuu();
}

void LaskuSivu::asiakasValintaMuuttuu()
{
    laskuAsiakasProxy_->setFilterFixedString( asiakasView_->currentIndex().data(AsiakkaatModel::NimiRooli).toString() );
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

        // Tarkistetaan, onko muokkaaminen sallittu
        TositeModel tositeModel( kp()->tietokanta());
        tositeModel.lataa(tosite);
        bool muokkausSallittu = tositeModel.muokkausSallittu()  &&
                ((index.data(LaskutModel::KirjausPerusteRooli).toInt() != LaskuModel::MAKSUPERUSTE ||
                  (index.data(LaskutModel::SummaRooli).toLongLong() == index.data(LaskutModel::AvoinnaRooli).toLongLong() &&
                   !index.data(LaskutModel::MuistutettuRooli).toBool())) &&
                  index.data(LaskutModel::TyyppiRooli).toInt() != LaskuModel::OSTOLASKU);

        // Vain Kitupiikin laskuja voi muokata
        muokkaaNappi_->setEnabled( muokkausSallittu  && index.data(LaskutModel::KirjausPerusteRooli).toInt() >= 0);
        poistaNappi_->setEnabled( muokkausSallittu );

        hyvitysNappi_->setEnabled( index.data(LaskutModel::TyyppiRooli).toInt() == LaskuModel::LASKU );
        muistutusNappi_->setVisible( index.data(LaskutModel::EraPvmRooli).toDate() < kp()->paivamaara() &&
                                     !index.data(LaskutModel::MuistutettuRooli).toBool() &&
                                     index.data(LaskutModel::EraPvmRooli).toDate().isValid() );

        kopioiNappi_->setEnabled( index.data(LaskutModel::KirjausPerusteRooli).toInt() >= 0 );

    }
    else
    {
        naytaNappi_->setDisabled(true);
        muokkaaNappi_->setDisabled(true);
        poistaNappi_->setDisabled(true);
        hyvitysNappi_->setDisabled(true);
        kopioiNappi_->setDisabled(true);
        muistutusNappi_->hide();
    }
}

void LaskuSivu::uusiLasku()
{
    LaskuDialogi* dlg = new LaskuDialogi();
    dlg->show();
}


void LaskuSivu::naytaLasku()
{
    QModelIndex index = laskuView_->currentIndex();
    NaytinIkkuna::naytaLiite( index.data(LaskutModel::TositeRooli).toInt(),
                           index.data(LaskutModel::LiiteRooli).toInt());


}

void LaskuSivu::hyvityslasku()
{
    if( LaskuDialogi::laskuIkkunoita())
    {
        QMessageBox::information(this, tr("Hyvityslaskua ei voi luoda"),
                             tr("Päällekkäisten viitenumeroiden välttämiseksi voit tehdä vain "
                                "yhden laskun kerrallaan.\n"
                                "Sulje avoinna oleva laskuikkuna ennen uuden laskun luomista."));
        return;
    }

}

void LaskuSivu::kopioiLasku()
{
}

void LaskuSivu::muokkaaLaskua()
{
    // Hakee laskun tiedot ja näyttää dialogin
    int tositeId = laskuView_->currentIndex().data(LaskuTauluModel::TositeIdRooli).toInt();
    int tyyppi = laskuView_->currentIndex().data(LaskuTauluModel::TyyppiRooli).toInt();

    if( tyyppi >= TositeTyyppi::MYYNTILASKU && tyyppi <= TositeTyyppi::MAKSUMUISTUTUS) {
        KpKysely *kysely = kpk( QString("/tositteet/%1").arg(tositeId));
        connect( kysely, &KpKysely::vastaus, this, &LaskuSivu::naytaLaskuDlg);
        kysely->kysy();
    } else {
        LisaIkkuna *lisa = new LisaIkkuna(this);
        lisa->naytaTosite(tositeId);
    }

}

void LaskuSivu::maksumuistutus()
{
    if( LaskuDialogi::laskuIkkunoita())
    {
        QMessageBox::information(this, tr("Maksumuistutusta ei voi luoda"),
                             tr("Päällekkäisten viitenumeroiden välttämiseksi voit tehdä vain "
                                "yhden laskun kerrallaan.\n"
                                "Sulje avoinna oleva laskuikkuna ennen uuden laskun luomista."));
        return;
    }
}


void LaskuSivu::poistaLasku()
{

    TositeModel tosite( kp()->tietokanta() );
    tosite.lataa( laskuView_->currentIndex().data(LaskutModel::TositeRooli).toInt() );


    if( QMessageBox::question(nullptr, tr("Vahvista laskun poistaminen"),
                              tr("Haluatko varmasti poistaa laskun %1 asiakkaalle %2")
                              .arg(laskuView_->currentIndex().data(LaskutModel::ViiteRooli).toString())
                              .arg(laskuView_->currentIndex().data(LaskutModel::AsiakasRooli).toString()),
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No) != QMessageBox::Yes)
        return;

    tosite.poista();
}

void LaskuSivu::naytaLaskuDlg(QVariant *data)
{
    LaskuDialogi* dlg = new LaskuDialogi(data->toMap());
    dlg->show();
}

void LaskuSivu::luoUi()
{
    paaTab_ = new QTabBar();

    paaTab_->addTab(QIcon(":/pic/lisaa.png"),tr("&Myyntilaskut"));
    paaTab_->addTab(QIcon(":/pic/poista.png"),tr("&Ostolaskut") );
    paaTab_->addTab(QIcon(":/pic/asiakkaat.png"),("&Asiakkaat"));
    paaTab_->addTab(QIcon(":/pic/yrittaja.png"),tr("&Toimittajat"));

    asiakasSuodatusEdit_ = new QLineEdit();
    asiakasSuodatusEdit_->setPlaceholderText( tr("Etsi asiakkaan nimellä") );

    QHBoxLayout *ylarivi = new QHBoxLayout;
    ylarivi->addWidget(paaTab_);
    ylarivi->addWidget(asiakasSuodatusEdit_);

    splitter_ = new QSplitter(Qt::Vertical);

    asiakasView_ = new QTableView();
    splitter_->addWidget(asiakasView_);
    asiakasView_->setAlternatingRowColors(true);

    lajiTab_ = new QTabBar();
    lajiTab_->addTab(tr("Luonnokset"));
    lajiTab_->addTab(tr("Lähetettävät"));
    lajiTab_->addTab(QIcon(":/pic/harmaa.png"),tr("&Lähetetyt"));
    lajiTab_->addTab(QIcon(":/pic/keltainen.png"),tr("&Avoimet"));
    lajiTab_->addTab(QIcon(":/pic/punainen.png"),tr("&Erääntyneet"));

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
    laskuView_->setAlternatingRowColors(true);
    alaruutuleiska->addWidget(laskuView_);


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


    nappileiska->addSpacing(64);

    kopioiNappi_ = new QPushButton( QIcon(":/pic/kopioilasku.png"), tr("&Kopioi"));
    nappileiska->addWidget( kopioiNappi_ );
    connect( kopioiNappi_, &QPushButton::clicked, this, &LaskuSivu::kopioiLasku);
    poistaNappi_ = new QPushButton(QIcon(":/pic/roskis.png"), tr("Poista"));
    nappileiska->addWidget(poistaNappi_);
    connect( poistaNappi_, &QPushButton::clicked, this, &LaskuSivu::poistaLasku);

    nappileiska->addSpacing(64);

    hyvitysNappi_ = new QPushButton(QIcon(":/pic/poista.png"), tr("&Hyvityslasku"));
    connect( hyvitysNappi_, &QPushButton::clicked, this, &LaskuSivu::hyvityslasku);
    nappileiska->addWidget(hyvitysNappi_);
    muistutusNappi_ = new QPushButton(QIcon(":/pic/varoitus.png"), tr("Maksumuistutus"));
    connect( muistutusNappi_, &QPushButton::clicked, this, &LaskuSivu::maksumuistutus);
    nappileiska->addWidget(muistutusNappi_);

    nappileiska->addStretch();

    QPushButton *uusiNappi = new QPushButton(QIcon(":/pic/uusitiedosto.png"), tr("&Uusi lasku"));
    nappileiska->addWidget(uusiNappi);
    connect( uusiNappi, &QPushButton::clicked, this, &LaskuSivu::uusiLasku);


    QVBoxLayout *paaLeiska = new QVBoxLayout;
    paaLeiska->addLayout(ylarivi);
    paaLeiska->addWidget(splitter_);
    paaLeiska->addLayout(nappileiska);

    setLayout(paaLeiska);

}
