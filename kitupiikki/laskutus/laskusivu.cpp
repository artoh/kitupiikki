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

#include "laskudialogi.h"
#include "db/kirjanpito.h"
#include "lisaikkuna.h"
#include "naytin/naytinikkuna.h"
#include "yhteystietowidget.h"

#include "model/laskutaulumodel.h"

#include "kumppanituotewidget.h"
#include "laskulistawidget.h"

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

LaskuSivu::LaskuSivu() :
    KitupiikkiSivu () ,
    kumppaniTuoteWidget_( new KumppaniTuoteWidget(this)),
    laskuWidget_( new LaskulistaWidget(this))

{
    luoUi();
    paaTab_->setCurrentIndex(MYYNTI);

    connect( paaTab_, &QTabBar::currentChanged, this, &LaskuSivu::paaTab );

    connect( asiakasSuodatusEdit_, &QLineEdit::textEdited, kumppaniTuoteWidget_, &KumppaniTuoteWidget::suodata);
    connect( asiakasSuodatusEdit_, &QLineEdit::textEdited, laskuWidget_, &LaskulistaWidget::suodataAsiakas);
    connect( kumppaniTuoteWidget_, &KumppaniTuoteWidget::kumppaniValittu, laskuWidget_, &LaskulistaWidget::suodataAsiakas);

}

LaskuSivu::~LaskuSivu()
{

}

void LaskuSivu::siirrySivulle()
{
    paaTab( paaTab_->currentIndex() );
}

void LaskuSivu::paaTab(int indeksi)
{
    kumppaniTuoteWidget_->setVisible( indeksi >= REKISTERI);
    laskuWidget_->setVisible( indeksi != TUOTTEET && indeksi != REKISTERI);

    if( indeksi >= REKISTERI )
        kumppaniTuoteWidget_->nayta( indeksi - 2);

    if( indeksi != TUOTTEET && indeksi != REKISTERI)
    {
        laskuWidget_->suodataAsiakas( asiakasSuodatusEdit_->text() );
        laskuWidget_->nayta( indeksi );
    }

    if( indeksi == ASIAKAS || indeksi == MYYNTI)
        asiakasSuodatusEdit_->setPlaceholderText(tr("Suodata asiakkaan nimellä"));
    else if( indeksi == TOIMITTAJA || indeksi == OSTO)
        asiakasSuodatusEdit_->setPlaceholderText(tr("Suodata toimittajan nimellä"));
    else
        asiakasSuodatusEdit_->setPlaceholderText(tr("Suodata tuotteen nimellä"));


}
/*
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
*/
void LaskuSivu::luoUi()
{
    paaTab_ = new QTabBar();

    paaTab_->addTab(QIcon(":/pic/lisaa.png"),tr("&Myynnit"));
    paaTab_->addTab(QIcon(":/pic/poista.png"),tr("&Ostot") );
    paaTab_->addTab(QIcon(":/pic/asiakkaat.png"),tr("&Rekisteri"));
    paaTab_->addTab(QIcon(":/pic/mies.png"),("&Asiakkaat"));
    paaTab_->addTab(QIcon(":/pic/yrittaja.png"),tr("&Toimittajat"));    
    paaTab_->addTab(QIcon(":/pic/kirjalaatikko.png"),tr("T&uotteet"));

    asiakasSuodatusEdit_ = new QLineEdit();
    asiakasSuodatusEdit_->setPlaceholderText( tr("Etsi asiakkaan nimellä") );

    QHBoxLayout *ylarivi = new QHBoxLayout;
    ylarivi->addWidget(paaTab_);
    ylarivi->addWidget(asiakasSuodatusEdit_);

    splitter_ = new QSplitter(Qt::Vertical);

    splitter_->addWidget( kumppaniTuoteWidget_ );

    splitter_->addWidget(laskuWidget_);

    QVBoxLayout *paaLeiska = new QVBoxLayout;
    paaLeiska->addLayout(ylarivi);
    paaLeiska->addWidget(splitter_);

    setLayout(paaLeiska);

}
