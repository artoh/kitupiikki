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

#include <QTabBar>
#include <QSplitter>
#include <QTableView>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>

LaskuSivu::LaskuSivu()
{
    luoUi();
    paaTab(MYYNTI);
    connect( paaTab_, &QTabBar::currentChanged, this, &LaskuSivu::paaTab );

}

void LaskuSivu::siirrySivulle()
{

}

void LaskuSivu::paaTab(int indeksi)
{
    asiakasView_->setVisible( indeksi >= ASIAKAS );

    if( indeksi >= ASIAKAS && lajiTab_->count() < 4)
        lajiTab_->addTab(tr("&Yhteystiedot"));
    else if(indeksi < ASIAKAS && lajiTab_->count() == 4)
        lajiTab_->removeTab(TIEDOT);
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

    QHBoxLayout *keskirivi = new QHBoxLayout;
    keskirivi->addWidget(lajiTab_);
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
