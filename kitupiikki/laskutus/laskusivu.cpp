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

LaskuSivu::LaskuSivu() :
    KitupiikkiSivu () ,
    ryhmaWidget_( new KumppaniTuoteWidget(this)),
    kumppaniTuoteWidget_( new KumppaniTuoteWidget(this)),
    laskuWidget_( new LaskulistaWidget(this))

{
    luoUi();
    paaTab_->setCurrentIndex(MYYNTI);

    connect( paaTab_, &QTabBar::currentChanged, this, &LaskuSivu::paaTab );

    connect( asiakasSuodatusEdit_, &QLineEdit::textEdited, kumppaniTuoteWidget_, &KumppaniTuoteWidget::suodata);
    connect( asiakasSuodatusEdit_, &QLineEdit::textEdited, laskuWidget_, &LaskulistaWidget::suodataAsiakas);
    connect( kumppaniTuoteWidget_, &KumppaniTuoteWidget::kumppaniValittu, laskuWidget_, &LaskulistaWidget::suodataAsiakas);
    connect( ryhmaWidget_, &KumppaniTuoteWidget::ryhmaValittu, kumppaniTuoteWidget_, &KumppaniTuoteWidget::suodataRyhma);

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
    laskuWidget_->setVisible( indeksi != TUOTTEET);


    if( indeksi == REKISTERI) {
        splitter_->replaceWidget(0, ryhmaWidget_);
        splitter_->replaceWidget(1, kumppaniTuoteWidget_);
        ryhmaWidget_->nayta(KumppaniTuoteWidget::RYHMAT);
    } else {
        if( splitter_->widget(1) != laskuWidget_)
            splitter_->replaceWidget(1, laskuWidget_);
        if( splitter_->widget(0) != kumppaniTuoteWidget_)
            splitter_->replaceWidget(0, kumppaniTuoteWidget_);
    }

    if( indeksi >= REKISTERI )
        kumppaniTuoteWidget_->nayta( indeksi - 2);    
    else if( indeksi < REKISTERI)
        kumppaniTuoteWidget_->nayta( indeksi);

    if( indeksi != TUOTTEET && indeksi != REKISTERI)
    {
        laskuWidget_->suodataAsiakas( asiakasSuodatusEdit_->text() );
        laskuWidget_->nayta( indeksi );
        laskuWidget_->paivita();
    }

    if( indeksi == ASIAKAS || indeksi == MYYNTI)
        asiakasSuodatusEdit_->setPlaceholderText(tr("Suodata asiakkaan nimellä"));
    else if( indeksi == TOIMITTAJA || indeksi == OSTO)
        asiakasSuodatusEdit_->setPlaceholderText(tr("Suodata toimittajan nimellä"));
    else if( indeksi == REKISTERI ) {
        asiakasSuodatusEdit_->setPlaceholderText(tr("Suodata nimellä"));
    } else
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
