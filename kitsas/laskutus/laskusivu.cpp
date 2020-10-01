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

LaskuSivu::LaskuSivu(QWidget * parent) :
    KitupiikkiSivu (parent) ,
    ryhmaWidget_( new KumppaniTuoteWidget()),
    kumppaniTuoteWidget_( new KumppaniTuoteWidget()),
    laskuWidget_( new LaskulistaWidget())

{
    luoUi();
    paaTab_->setCurrentIndex(MYYNTI);

    connect( paaTab_, &QTabBar::currentChanged, this, &LaskuSivu::paaTab );

    connect( asiakasSuodatusEdit_, &QLineEdit::textEdited, kumppaniTuoteWidget_, &KumppaniTuoteWidget::suodata);
    connect( asiakasSuodatusEdit_, &QLineEdit::textEdited, [this] (const QString& teksti) {this->laskuWidget_->suodataAsiakas(teksti,0);});
    connect( kumppaniTuoteWidget_, &KumppaniTuoteWidget::kumppaniValittu, laskuWidget_, &LaskulistaWidget::suodataAsiakas);
    connect( ryhmaWidget_, &KumppaniTuoteWidget::ryhmaValittu, kumppaniTuoteWidget_, &KumppaniTuoteWidget::suodataRyhma);

}

LaskuSivu::~LaskuSivu()
{
    delete ryhmaWidget_;
    delete kumppaniTuoteWidget_;
    delete laskuWidget_;
}

void LaskuSivu::siirrySivulle()
{
    paaTab( paaTab_->currentIndex() );
}

QString LaskuSivu::ohjeSivunNimi()
{
    switch (paaTab_->currentIndex()) {
        case OSTO: case TOIMITTAJA:
            return "laskutus/ostot";
        case REKISTERI:
            return "laskutus/rekisteri";
        case TUOTTEET:
            return "laskutus/tuotteet";
        case VAKIOVIITTEET:
            return "laskutus/vakioviite";
        default:
            return "laskutus/myynnit";
    }

}

void LaskuSivu::outbox()
{
    paaTab_->setCurrentIndex(MYYNTI);
    laskuWidget_->alalehti(LaskuSivu::LAHETETTAVAT);
}

void LaskuSivu::paaTab(int indeksi)
{

    if( indeksi == REKISTERI) {
        if(splitter_->widget(0) != ryhmaWidget_) {
            splitter_->replaceWidget(0, ryhmaWidget_);
        }
        if(splitter_->widget(1) != kumppaniTuoteWidget_) {
            splitter_->replaceWidget(1, kumppaniTuoteWidget_);
        }
        ryhmaWidget_->nayta(KumppaniTuoteWidget::RYHMAT);
        kumppaniTuoteWidget_->nayta(KumppaniTuoteWidget::REKISTERI);
    } else {
        if( splitter_->widget(1) != laskuWidget_)
            splitter_->replaceWidget(1, laskuWidget_);
        if( splitter_->widget(0) != kumppaniTuoteWidget_)
            splitter_->replaceWidget(0, kumppaniTuoteWidget_);
    }

    kumppaniTuoteWidget_->setVisible( indeksi >= REKISTERI);
    laskuWidget_->setVisible( indeksi != TUOTTEET && indeksi != REKISTERI && indeksi != VAKIOVIITTEET);
    ryhmaWidget_->setVisible( indeksi == REKISTERI && indeksi != VAKIOVIITTEET);


    if( indeksi >= REKISTERI )
        kumppaniTuoteWidget_->nayta( indeksi - 2);    
    else if( indeksi < REKISTERI)
        kumppaniTuoteWidget_->nayta( indeksi);    


    if( indeksi != TUOTTEET && indeksi != REKISTERI && indeksi != VAKIOVIITTEET)
    {
        laskuWidget_->suodataAsiakas( asiakasSuodatusEdit_->text() );
        laskuWidget_->nayta( indeksi );
        laskuWidget_->paivita();
    }

    if( indeksi == ASIAKAS || indeksi == MYYNTI)
        asiakasSuodatusEdit_->setPlaceholderText(tr("Suodata asiakkaan nimellä"));
    else if( indeksi == TOIMITTAJA || indeksi == OSTO)
        asiakasSuodatusEdit_->setPlaceholderText(tr("Suodata toimittajan nimellä"));
    else if( indeksi == REKISTERI )
        asiakasSuodatusEdit_->setPlaceholderText(tr("Suodata nimellä"));
    else if( indeksi == VAKIOVIITTEET)
        asiakasSuodatusEdit_->setPlaceholderText("Suodata otsikolla");
    else
        asiakasSuodatusEdit_->setPlaceholderText(tr("Suodata tuotteen nimellä"));


}

void LaskuSivu::luoUi()
{
    paaTab_ = new QTabBar();

    paaTab_->addTab(QIcon(":/pic/lisaa.png"),tr("&Myynnit"));
    paaTab_->addTab(QIcon(":/pic/poista.png"),tr("&Ostot") );
    paaTab_->addTab(QIcon(":/pic/asiakkaat.png"),tr("&Rekisteri"));
    paaTab_->addTab(QIcon(":/pic/mies.png"),("&Asiakkaat"));
    paaTab_->addTab(QIcon(":/pic/yrittaja.png"),tr("&Toimittajat"));    
    paaTab_->addTab(QIcon(":/pic/kirjalaatikko.png"),tr("T&uotteet"));
    paaTab_->addTab(QIcon(":/pic/viivakoodi.png"), tr("&Vakioviitteet"));

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
