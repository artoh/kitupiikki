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
#include "saldodock.h"
#include "saldomodel.h"

#include "db/kirjanpito.h"
#include "db/yhteysmodel.h"
#include <QTableView>
#include <QHeaderView>
#include <QSettings>

#include <QSortFilterProxyModel>
#include <QBoxLayout>
#include <QWidget>
#include <QToolButton>
#include <QLabel>

SaldoDock *SaldoDock::dock()
{
    if( !instanssi__)
        instanssi__ = new SaldoDock();
    return instanssi__;
}

SaldoDock::SaldoDock() :
    QDockWidget(tr("Saldot")),
    view_(new QTableView(this)),
    model_(new SaldoModel(this)),
    proxy_( new QSortFilterProxyModel(this))
{
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);

    connect( kp(), &Kirjanpito::kirjanpitoaMuokattu, this, &SaldoDock::paivita );
    connect( kp(), &Kirjanpito::tietokantaVaihtui, this, &SaldoDock::alusta);

    proxy_->setSourceModel(model_);
    view_->setModel(proxy_);

    view_->horizontalHeader()->setSectionResizeMode(SaldoModel::NIMI, QHeaderView::Stretch);
    view_->horizontalHeader()->setVisible(false);

    QBoxLayout* napit = new QBoxLayout(QBoxLayout::TopToBottom);

    rahavarat_ = new QToolButton();
    rahavarat_->setIcon(QIcon(":/pic/euro.png"));
    rahavarat_->setCheckable(true);
    napit->addWidget(rahavarat_);

    suosikit_ = new QToolButton();
    suosikit_->setIcon(QIcon(":/pic/tahti.png"));
    suosikit_->setCheckable(true);
    napit->addWidget(suosikit_);

    kaikki_ = new QToolButton();
    kaikki_->setIcon(QIcon(":/pic/kaytossa.png"));
    kaikki_->setCheckable(true);
    napit->addWidget(kaikki_);

    napit->addStretch();

    QBoxLayout* leiska = new QBoxLayout(QBoxLayout::LeftToRight);
    leiska->addLayout(napit);
    leiska->addWidget(view_);

    QWidget *widget = new QWidget();
    widget->setLayout(leiska);
    this->setWidget(widget);

//    setTitleBarWidget(new QWidget(this));
    setFilter( kp()->settings()->value("SaldoDockValinta").toInt() );

    connect( rahavarat_, &QToolButton::clicked, this, [this] { this->setFilter(RAHAVARAT);});
    connect( suosikit_, &QToolButton::clicked, this,  [this] { this->setFilter(SUOSIKIT);});
    connect( kaikki_, &QToolButton::clicked, this, [this] { this->setFilter(KAIKKI);});

    view_->setSelectionMode(QTableView::NoSelection);
    view_->setAlternatingRowColors(true);

    alusta();
}

void SaldoDock::paivita()
{
    if( isVisible() )
        model_->paivitaSaldot();
}

void SaldoDock::alusta()
{
    if( kp()->yhteysModel() == nullptr ||
            !kp()->yhteysModel()->onkoOikeutta(YhteysModel::TOSITE_SELAUS + YhteysModel::TOSITE_LUONNOS + YhteysModel::TOSITE_MUOKKAUS
                                                                               + YhteysModel::RAPORTIT + YhteysModel::TILINPAATOS + YhteysModel::ASETUKSET) )
    {
        hide();
    } else {
        setVisible( kp()->settings()->value("SaldoDock").toBool() );
        model_->paivitaSaldot();
    }
}

void SaldoDock::showEvent(QShowEvent *event)
{
    paivita();
    QDockWidget::showEvent(event);
}

void SaldoDock::setFilter(int index)
{
    if( index == RAHAVARAT) {
        proxy_->setFilterRole(SaldoModel::TyyppiRooli);
        proxy_->setFilterRegularExpression("AR.*");
    } else if( index == SUOSIKIT ) {
        proxy_->setFilterRole(SaldoModel::SuosioRooli);
        proxy_->setFilterFixedString("2");
    } else {
        proxy_->setFilterFixedString("");
    }

    rahavarat_->setChecked( index == RAHAVARAT );
    suosikit_->setChecked( index == SUOSIKIT );
    kaikki_->setChecked( index == KAIKKI);

    kp()->settings()->setValue("SaldoDockValinta", index);
}

SaldoDock* SaldoDock::instanssi__ = nullptr;
